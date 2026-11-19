#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../node/FENode.hpp"
#include    "../../mesh/FEMeshBuilder.hpp"
#include    "../../material/FEMaterialPoint565.hpp"
#include    "FEFormatLAS.hpp"
#include    <algorithm>
#include    <chrono>
#include    <cmath>
#include    <cstring>
#include    <filesystem>
#include    <limits>
#include    <string>
#include    <utility>

namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatLASReader,"{0C7A2F54-8B31-4D62-A5E8-3F1D9C6B7A20}");

    /// <summary>
    /// LAS 点云读取器(LAS 1.0~1.4，点记录格式 0~10)。
    /// - 支持 scale/offset 换算与 extra bytes(按记录长度跳过)；
    /// - 颜色优先级:RGB -> ASPRS 分类色 -> 强度灰度，最终打包为 RGB565；
    /// - 按 KD 中位二分构建盒树，每个叶子盒最多 10240 个点，每个盒对应一个节点，
    ///   叶子盒网格顶点使用相对盒中心的局部坐标；
    /// - 不支持的压缩格式(LAZ，点格式高位被置位)会明确报错。
    /// </summary>
    /// <remarks>
    /// ==================== LAS 文件格式说明(小端) ====================
    /// 文件由四部分组成:
    ///   [Public Header Block] -> [VLR 列表] -> [Point Data Records] -> [EVLR 列表(1.4)]
    ///
    /// 一、Public Header Block
    ///   偏移  长度  字段
    ///   0     4     "LASF" 文件签名
    ///   24    1     版本主号(必须为 1)
    ///   25    1     版本次号(0~4)
    ///   26    32    系统标识
    ///   58    32    生成软件
    ///   90    2     创建日(年内第几天)
    ///   92    2     创建年
    ///   94    2     Header Size(1.0~1.2=227，1.3=235，1.4=375)
    ///   96    4     Offset To Point Data(点数据起始偏移)
    ///   100   4     Number Of VLRs
    ///   104   1     Point Data Format(低 6 位；高位 0x80/0xC0 表示 LAZ 压缩)
    ///   105   2     Point Record Length(含 extra bytes)
    ///   107   4     Legacy Number Of Point Records
    ///   111   20    Legacy Number Of Points By Return(5 x u32)
    ///   131   24    X/Y/Z Scale Factor(3 x f64)
    ///   155   24    X/Y/Z Offset(3 x f64)
    ///   179   48    Max X / Min X / Max Y / Min Y / Max Z / Min Z(6 x f64)
    ///   227   8     Start Of Waveform Data(1.3+)
    ///   235   8     Start Of First EVLR(1.4)
    ///   243   4     Number Of EVLRs(1.4)
    ///   247   8     Extended Number Of Point Records(1.4)
    ///   255   120   Extended Number Of Points By Return(15 x u64，1.4)
    ///
    /// 二、VLR(可变长度记录)
    ///   每条 = 54 字节头 + 数据:
    ///   0 reserved(u16) / 2 user id(16B) / 18 record id(u16) /
    ///   20 record length(u16) / 22 description(32B) / 54 数据
    ///   本读取器不解析 VLR，仅按 Offset To Point Data 定位点数据。
    ///
    /// 三、Point Data Records(每条长度 = Point Record Length)
    ///   公共字段:
    ///   0   12   X/Y/Z 原始整数坐标(3 x i32)
    ///   12  2    Intensity(u16)
    ///   14  1/2  返回信息(格式 0~5 为 1 字节，6~10 为 2 字节)
    ///   15  1    分类(格式 0~5)
    ///   16  1    分类(格式 6~10)
    ///   点格式  标准长度  GPS Time 偏移  RGB 偏移
    ///   0       20        -              -
    ///   1       28        22             -
    ///   2       26        -              20
    ///   3       34        22             28
    ///   4       57        22             -
    ///   5       63        22             28
    ///   6       30        22             -
    ///   7       36        22             30
    ///   8       38        22             30
    ///   9       59        22             -
    ///   10      67        22             30
    ///   记录长度大于标准长度时，多出的字节为 extra bytes(本读取器直接跳过)。
    ///   RGB 为 3 x u16(0~65535)，本读取器取高 8 位参与打包。
    ///
    /// 四、坐标换算
    ///   world = raw(i32) * scale(f64) + offset(f64)，全程 double 计算。
    ///
    /// 五、EVLR(LAS 1.4，通常位于文件末尾)
    ///   每条 = 60 字节头 + 数据:
    ///   0 reserved(u16) / 2 user id(16B) / 18 record id(u16) /
    ///   20 record length(u64) / 28 description(32B) / 60 数据
    ///   本读取器不解析。
    ///
    /// 本读取器颜色优先级:RGB -> ASPRS 分类色 -> 强度灰度(取强度高 8 位)，
    /// 最终统一打包为 RGB565(R 高 5 位、G 中 6 位、B 低 5 位)。
    /// 点数据按 KD 中位二分构建盒树，叶子盒点数不超过 10240，盒即节点，
    /// 叶子网格顶点为相对盒中心的局部坐标；压缩格式(LAZ)通过点格式高位识别，
    /// 检测到后明确报错并跳过该文件。
    /// </remarks>
    class   FEFormatLASReader :public FEStreamReader
    {
    protected:
        static  constexpr   uint32  kMaxPointsPerBox    =   10240;
        static  constexpr   uint32  kMaxDepth           =   64;
        static  constexpr   uint32  kInvalid            =   0xFFFFFFFFu;
        /// <summary>
        /// 叶子盒各轴最大边长(世界单位)。顶点位置用有符号 16 位量化，
        /// 量化步长 = 边长 / 65534，限制边长即限制量化误差，
        /// 避免稀疏区域出现大盒子导致精度下降
        /// </summary>
        double  _leafMaxExt[3]  =   {0.0,0.0,0.0};
    public:
        /// <summary>
        /// 解析阶段的点:原始整型坐标 + RGB565 颜色，16 字节有效数据
        /// </summary>
        struct  LasPoint
        {
            int32   x   =   0;
            int32   y   =   0;
            int32   z   =   0;
            uint32  color = 0;
        };
        using   LasPoints   =   std::vector<LasPoint>;
    protected:
        /// <summary>
        /// 盒树节点:内部节点只做分组，叶子节点承载一个点云网格
        /// </summary>
        struct  LasBox
        {
            double      min[3]      =   {0.0,0.0,0.0};
            double      max[3]      =   {0.0,0.0,0.0};
            double      center[3]   =   {0.0,0.0,0.0};
            int32       rawMin[3]   =   {0,0,0};
            int32       rawMax[3]   =   {0,0,0};
            uint32      parent      =   kInvalid;
            uint32      child0      =   kInvalid;
            uint32      child1      =   kInvalid;
            uint32      depth       =   0;
            LasPoints   points;
        };
        using   LasBoxes    =   std::vector<LasBox>;
        /// <summary>
        /// LAS 头信息(仅保留本读取器用到的字段)
        /// </summary>
        struct  LasHeader
        {
            uint8   versionMajor        =   0;
            uint8   versionMinor        =   0;
            uint16  headerSize          =   0;
            uint32  offsetToPointData   =   0;
            uint32  vlrCount            =   0;
            uint8   pointFormat         =   0;
            uint16  pointRecordLength   =   0;
            uint32  legacyPointCount    =   0;
            uint64  extendedPointCount  =   0;
            double  scale[3]            =   {0.0,0.0,0.0};
            double  offset[3]           =   {0.0,0.0,0.0};
            double  min[3]              =   {0.0,0.0,0.0};
            double  max[3]              =   {0.0,0.0,0.0};
            bool    compressed          =   false;
        };
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatLASReader)
    public:
        FEFormatLASReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatLASReader(const FEFormatLASReader& other)
            :FEStreamReader(other)
        {}
        /// <summary>
        /// 组件支持的格式数组
        /// </summary>
        virtual Formats formats() const override
        {
            return  formatList();
        }
        /// <summary>
        /// 从文件加载 LAS 点云
        /// </summary>
        virtual Objects readFiles(const Strings& files) override
        {
            Objects result;
            for (auto& file : files)
            {
                if (isCanceled())
                    break;
                FEReaderFile    reader(_ctx,file.c_str());
                if (!reader.isValid())
                {
                    LOG_ERR("LAS: open file failed: %s",file.c_str());
                    continue;
                }
                /// 资源包中的文件没有磁盘路径，整体读入内存后解析
                if (reader.isAssets())
                {
                    FEReaderHelper  helper(reader);
                    Buffer          buffer  =   helper.read(_ctx);
                    reader.close();
                    Node    root    =   parseBuffer(file,buffer);
                    if (root != nullptr)
                        result.emplace_back(root);
                    continue;
                }
                uint64  fileSize    =   reader.length();
                Node    root        =   parseFile(file,reader,fileSize);
                reader.close();
                if (root != nullptr)
                    result.emplace_back(root);
            }
            setProgress(1.0);
            return  result;
        }
        /// <summary>
        /// 从内存缓冲区读取 LAS 点云
        /// </summary>
        virtual Objects readBuffers(const Buffers& buffers) override
        {
            Objects result;
            for (auto& buffer : buffers)
            {
                if (buffer == nullptr || isCanceled())
                    continue;
                Node    root    =   parseBuffer("las",buffer);
                if (root != nullptr)
                    result.emplace_back(root);
            }
            setProgress(1.0);
            return  result;
        }
    public:
        static  Formats formatList()
        {
            return  FELASFormat::formats(UUIDOF(FEFormatLASReader));
        }
    protected:
        /// <summary>
        /// 解析磁盘文件(流式读取点记录，避免整文件缓冲)
        /// </summary>
        Node    parseFile(const String& file,FEReaderFile& reader,uint64 fileSize)
        {
            uint8   header[375] =   {0};
            uint64  readLen     =   std::min<uint64>(fileSize,sizeof(header));
            if (readLen < 227)
            {
                LOG_ERR("LAS: file too small: %s",file.c_str());
                return  nullptr;
            }
            reader.seek(0);
            if (reader.readBuffer(header,readLen) != readLen)
            {
                LOG_ERR("LAS: read header failed: %s",file.c_str());
                return  nullptr;
            }
            LasHeader   hdr;
            if (!parseHeader(header,readLen,hdr))
            {
                LOG_ERR("LAS: invalid header: %s",file.c_str());
                return  nullptr;
            }
            if (hdr.compressed)
            {
                LOG_ERR("LAS: compressed(LAZ) point cloud not supported: %s",file.c_str());
                return  nullptr;
            }
            uint64  count       =   headerPointCount(hdr);
            uint64  available   =   0;
            if (fileSize > hdr.offsetToPointData && hdr.pointRecordLength > 0)
                available   =   (fileSize - hdr.offsetToPointData) / hdr.pointRecordLength;
            if (available == 0)
            {
                LOG_ERR("LAS: no point data: %s",file.c_str());
                return  nullptr;
            }
            if (count == 0 || count > available)
                count   =   available;
            LasPoints   points;
            points.reserve(size_t(count));
            reader.seek(hdr.offsetToPointData);
            const   auto    tRead0  =   std::chrono::steady_clock::now();
            if (!readPoints(reader,hdr,count,points))
                return  nullptr;
            LOG_INF("LAS: read points %lldms",
                    (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - tRead0).count());
            setProgress(0.7);
            return  buildCloud(stemOf(file),hdr,std::move(points));
        }
        /// <summary>
        /// 解析内存缓冲区
        /// </summary>
        Node    parseBuffer(const String& name,const Buffer& buffer)
        {
            if (buffer == nullptr)
                return  nullptr;
            const   uint8*  data    =   buffer->data();
            uint64          size    =   buffer->length();
            LasHeader       hdr;
            if (!parseHeader(data,size,hdr))
            {
                LOG_ERR("LAS: invalid header in buffer");
                return  nullptr;
            }
            if (hdr.compressed)
            {
                LOG_ERR("LAS: compressed(LAZ) point cloud not supported");
                return  nullptr;
            }
            uint64  count       =   headerPointCount(hdr);
            uint64  available   =   0;
            if (size > hdr.offsetToPointData && hdr.pointRecordLength > 0)
                available   =   (size - hdr.offsetToPointData) / hdr.pointRecordLength;
            if (available == 0)
            {
                LOG_ERR("LAS: no point data in buffer");
                return  nullptr;
            }
            if (count == 0 || count > available)
                count   =   available;
            LasPoints   points;
            points.reserve(size_t(count));
            parseRecords(data + hdr.offsetToPointData,hdr,count,points);
            setProgress(0.7);
            return  buildCloud(stemOf(name),hdr,std::move(points));
        }
        /// <summary>
        /// 流式读取点记录
        /// </summary>
        bool    readPoints(FEReaderFile& reader,const LasHeader& hdr,uint64 count,LasPoints& points)
        {
            if (hdr.pointRecordLength == 0)
                return  false;
            uint64  recordsPerChunk =   std::max<uint64>(1,(1ULL << 20) / hdr.pointRecordLength);
            uint8s  chunk(size_t(recordsPerChunk) * hdr.pointRecordLength);
            uint64  done    =   0;
            while (done < count)
            {
                if (isCanceled())
                    return  false;
                uint64  n       =   std::min<uint64>(recordsPerChunk,count - done);
                uint64  want    =   n * hdr.pointRecordLength;
                uint64  got     =   reader.readBuffer(chunk.data(),want);
                if (got < want)
                {
                    LOG_ERR("LAS: truncated point data at %llu/%llu",
                            (unsigned long long)done,(unsigned long long)count);
                    return  false;
                }
                parseRecords(chunk.data(),hdr,n,points);
                done    +=  n;
                setProgress(0.05 + 0.65 * double(done) / double(count));
            }
            return  true;
        }
        /// <summary>
        /// 解析点记录到原始整型坐标 + RGB565 颜色
        /// </summary>
        void    parseRecords(const uint8* data,const LasHeader& hdr,uint64 count,LasPoints& points)
        {
            points.reserve(points.size() + size_t(count));
            for (uint64 i = 0; i < count; ++i)
            {
                const   uint8*  rec =   data + i * hdr.pointRecordLength;
                LasPoint        p;
                p.x     =   rdI32(rec);
                p.y     =   rdI32(rec + 4);
                p.z     =   rdI32(rec + 8);
                p.color =   colorOf(rec,hdr.pointFormat);
                points.push_back(p);
            }
        }
        /// <summary>
        /// 依据点格式计算颜色:RGB -> 分类色 -> 强度灰度
        /// </summary>
        uint32  colorOf(const uint8* rec,uint8 fmt) const
        {
            uint16  rgbOffset   =   rgbOffsetOf(fmt);
            if (rgbOffset != 0xFFFF)
            {
                const   uint16  r16 =   rdU16(rec + rgbOffset);
                const   uint16  g16 =   rdU16(rec + rgbOffset + 2);
                const   uint16  b16 =   rdU16(rec + rgbOffset + 4);
                /// 兼容两种存储:标准 16 位颜色取高 8 位，8 位颜色则直接存放在低字节
                uint8   r   =   uint8(r16 >> 8);
                uint8   g   =   uint8(g16 >> 8);
                uint8   b   =   uint8(b16 >> 8);
                if ((r16 | g16 | b16) <= 0xFFu)
                {
                    r   =   uint8(r16);
                    g   =   uint8(g16);
                    b   =   uint8(b16);
                }
                /// 全部为 0 时视为没有有效颜色，回退到分类/强度
                if ((r | g | b) != 0)
                    return  pack565(r,g,b);
            }
            uint8   cls =   (fmt <= 5) ? uint8(rec[15] & 0x1Fu) : rec[16];
            if (cls > 1)
                return  classificationColor(cls);
            /// 强度同样兼容 8 位值与标准 16 位值
            const   uint16  intensity   =   rdU16(rec + 12);
            uint8   gray    =   intensity <= 0xFFu ? uint8(intensity) : uint8(intensity >> 8);
            return  pack565(gray,gray,gray);
        }
        /// <summary>
        /// 构建盒树并生成场景节点
        /// </summary>
        Node    buildCloud(const String& name,const LasHeader& hdr,LasPoints&& points)
        {
            if (points.empty())
                return  nullptr;
            size_t  totalPoints     =   points.size();
            /// 量化步长 = 叶子盒边长 / 65534。以“平均点间距的 1/8”为步长上限，
            /// 即盒边长上限 ≈ 8192 倍平均点间距:量化误差远小于点间距，
            /// 同时避免线状/面状分布被过度分割成海量节点
            const   double  dx      =   hdr.max[0] - hdr.min[0];
            const   double  dy      =   hdr.max[1] - hdr.min[1];
            const   double  dz      =   hdr.max[2] - hdr.min[2];
            double  spacing         =   std::cbrt((std::max)(dx * dy * dz / double(totalPoints),0.0));
            if (spacing <= 0.0)
                spacing             =   std::sqrt(dx * dx + dy * dy + dz * dz) / 256.0;
            const   double  maxExt  =   spacing * 8192.0;
            for (int i = 0; i < 3; ++i)
                _leafMaxExt[i]      =   maxExt;
            size_t  leafCount       =   0;
            size_t  overflowCount   =   0;
            const   auto    tTree0  =   std::chrono::steady_clock::now();
            LasBoxes    boxes;
            buildTree(boxes,hdr,std::move(points),leafCount,overflowCount);
            const   auto    tTree1  =   std::chrono::steady_clock::now();
            setProgress(0.85);
            Nodes   nodes;
            nodes.resize(boxes.size());
            Material    material    =   new FEMaterialPoint565(_ctx);
            /// 以点云包围盒对角线为基准设置距离衰减，使不同尺度的文件初始观感一致:
            /// 适配整片点云时约 1 像素，拉近后逐渐增大到 8 像素上限
            const   double  diag    =   std::sqrt(dx * dx + dy * dy + dz * dz);
            if (diag > 0.0)
                material->cast<FEMaterialPoint565>()->setPointSizeScale(float(diag * 1.5));
            for (size_t i = 0; i < boxes.size(); ++i)
            {
                LasBox& box     =   boxes[i];
                Node    node    =   new FENode(_ctx);
                if (i == 0)
                {
                    node->setName(name);
                    node->setLocalTranslation(real3(box.center[0],box.center[1],box.center[2]));
                }
                else
                {
                    const   LasBox& parent  =   boxes[box.parent];
                    node->setName(name + "_" + std::to_string(i));
                    node->setLocalTranslation(real3(box.center[0] - parent.center[0],
                                                    box.center[1] - parent.center[1],
                                                    box.center[2] - parent.center[2]));
                    nodes[box.parent]->addChild(node);
                }
                nodes[i]    =   node;
            }
            const   auto    tNode1  =   std::chrono::steady_clock::now();
            for (size_t i = 0; i < boxes.size(); ++i)
            {
                if (isCanceled())
                    break;
                LasBox& box =   boxes[i];
                if (box.child0 == kInvalid)
                    buildLeafMesh(nodes[i],box,hdr,material.get());
                setProgress(0.85 + 0.15 * double(i + 1) / double(boxes.size()));
            }
            LOG_INF("LAS: %s points=%zu boxes=%zu leaves=%zu overflow=%zu",
                    name.c_str(),totalPoints,boxes.size(),leafCount,overflowCount);
            {
                const   auto    tEnd    =   std::chrono::steady_clock::now();
                auto    ms  =   [](std::chrono::steady_clock::time_point a,std::chrono::steady_clock::time_point b)
                {
                    return  (long long)std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
                };
                LOG_INF("LAS: phases tree=%lldms nodes=%lldms mesh=%lldms",
                        ms(tTree0,tTree1),ms(tTree1,tNode1),ms(tNode1,tEnd));
            }
            return  nodes.empty() ? nullptr : nodes[0];
        }
        /// <summary>
        /// 递归构建 KD 盒树(中位二分，叶子点数不超过 kMaxPointsPerBox，
        /// 且叶子各轴边长不超过 _leafMaxExt，以保证 16 位坐标量化精度)
        /// </summary>
        void    buildTree(LasBoxes& boxes,const LasHeader& hdr,LasPoints&& points,size_t& leafCount,size_t& overflowCount)
        {
            LasBox  root;
            root.points =   std::move(points);
            computeBounds(root,hdr);
            boxes.push_back(std::move(root));
            uint32s stack;
            stack.push_back(0);
            /// 组合均匀分割时叶子数约为 2 的幂，用该估计驱动进度条，
            /// 避免 KD 构建期间进度一直停在 70%
            size_t  needLeaves  =   (boxes[0].points.size() + kMaxPointsPerBox - 1) / kMaxPointsPerBox;
            size_t  leafEst     =   1;
            while (leafEst < needLeaves)
                leafEst <<= 1;
            const   double  splitsEst   =   leafEst > 1 ? double(leafEst * 2 - 2) : 1.0;
            size_t  splits      =   0;
            while (!stack.empty())
            {
                if (isCanceled())
                    break;
                uint32  idx =   stack.back();
                stack.pop_back();
                if (!needSplit(boxes[idx]))
                    continue;
                if (!splitBox(boxes,hdr,idx))
                {
                    ++overflowCount;
                    continue;
                }
                ++splits;
                if ((splits & 63u) == 0u)
                    setProgress(0.7 + 0.15 * (std::min)(1.0,double(splits) / splitsEst));
                stack.push_back(boxes[idx].child0);
                stack.push_back(boxes[idx].child1);
            }
            leafCount   =   0;
            for (auto& box : boxes)
            {
                if (box.child0 == kInvalid)
                    ++leafCount;
            }
        }
        /// <summary>
        /// 点数超限，或任一边长超出量化精度允许的盒子尺寸时需要继续分割
        /// </summary>
        bool    needSplit(const LasBox& box) const
        {
            if (box.points.size() > kMaxPointsPerBox)
                return  true;
            if (box.points.size() < 2)
                return  false;
            for (int i = 0; i < 3; ++i)
            {
                if (_leafMaxExt[i] > 0.0 && (box.max[i] - box.min[i]) > _leafMaxExt[i])
                    return  true;
            }
            return  false;
        }
        /// <summary>
        /// 取指定轴上的原始整数坐标
        /// </summary>
        static  int32   axisValue(const LasPoint& p,int axis)
        {
            return  axis == 0 ? p.x : (axis == 1 ? p.y : p.z);
        }
        /// <summary>
        /// 按指定轴就地选出第 nth 个元素(左侧全部不大于它)。
        /// 手写三分取中快选替代 std::nth_element:后者每次比较都要经过
        /// lambda/函数调用，Debug 构建下 KD 树构建的耗时几乎全在这里。
        /// </summary>
        static  void    selectByAxis(LasPoint* first,LasPoint* nth,LasPoint* last,int axis)
        {
            while (last - first > 24)
            {
                LasPoint* mid    =   first + (last - first) / 2;
                const   int32   a    =   axisValue(*first,axis);
                const   int32   b    =   axisValue(*mid,axis);
                const   int32   c    =   axisValue(*(last - 1),axis);
                /// 三数取中，避免有序数据退化
                const   int32   pivot = a < b ? (b < c ? b : (a < c ? c : a))
                                              : (a < c ? a : (b < c ? c : b));
                LasPoint* i =   first;
                LasPoint* j =   last - 1;
                while (i <= j)
                {
                    while (axisValue(*i,axis) < pivot) ++i;
                    while (axisValue(*j,axis) > pivot) --j;
                    if (i <= j)
                    {
                        LasPoint    tmp =   *i;
                        *i  =   *j;
                        *j  =   tmp;
                        ++i;
                        --j;
                    }
                }
                /// [first,j] 不大于 pivot，[i,last) 不小于 pivot
                if (nth <= j)
                    last    =   j + 1;
                else if (nth >= i)
                    first   =   i;
                else
                    return;
            }
            /// 小区间插入排序收尾
            for (LasPoint* it = first + 1; it < last; ++it)
            {
                const   LasPoint    v   =   *it;
                LasPoint*           p   =   it;
                while (p > first && axisValue(*(p - 1),axis) > axisValue(v,axis))
                {
                    *p  =   *(p - 1);
                    --p;
                }
                *p  =   v;
            }
        }
        /// <summary>
        /// 按最长轴中位二分一个盒子
        /// </summary>
        bool    splitBox(LasBoxes& boxes,const LasHeader& hdr,uint32 idx)
        {
            int     axis    =   -1;
            double  best    =   0.0;
            for (int i = 0; i < 3; ++i)
            {
                double  ext =   boxes[idx].max[i] - boxes[idx].min[i];
                if (ext > best + 1e-9)
                {
                    best    =   ext;
                    axis    =   i;
                }
            }
            if (axis < 0 || boxes[idx].depth >= kMaxDepth || boxes[idx].points.size() < 2)
                return  false;
            LasPoints   left    =   std::move(boxes[idx].points);
            boxes[idx].points   =   LasPoints();
            size_t  mid =   left.size() / 2;
            selectByAxis(left.data(),left.data() + mid,left.data() + left.size(),axis);
            LasPoints   right(std::make_move_iterator(left.begin() + mid),
                              std::make_move_iterator(left.end()));
            left.resize(mid);
            uint32  depth   =   boxes[idx].depth + 1;
            LasBox  c0;
            c0.parent   =   idx;
            c0.depth    =   depth;
            c0.points   =   std::move(left);
            computeBounds(c0,hdr);
            LasBox  c1;
            c1.parent   =   idx;
            c1.depth    =   depth;
            c1.points   =   std::move(right);
            computeBounds(c1,hdr);
            uint32  i0  =   uint32(boxes.size());
            boxes.push_back(std::move(c0));
            uint32  i1  =   uint32(boxes.size());
            boxes.push_back(std::move(c1));
            boxes[idx].child0  =   i0;
            boxes[idx].child1  =   i1;
            return  true;
        }
        /// <summary>
        /// 计算盒子的世界包围盒、中心与原始坐标范围
        /// </summary>
        void    computeBounds(LasBox& box,const LasHeader& hdr) const
        {
            if (box.points.empty())
                return;
            int32   rmin[3] =   {std::numeric_limits<int32>::max(),
                                 std::numeric_limits<int32>::max(),
                                 std::numeric_limits<int32>::max()};
            int32   rmax[3] =   {std::numeric_limits<int32>::min(),
                                 std::numeric_limits<int32>::min(),
                                 std::numeric_limits<int32>::min()};
            for (auto& p : box.points)
            {
                if (p.x < rmin[0]) rmin[0] = p.x;
                if (p.x > rmax[0]) rmax[0] = p.x;
                if (p.y < rmin[1]) rmin[1] = p.y;
                if (p.y > rmax[1]) rmax[1] = p.y;
                if (p.z < rmin[2]) rmin[2] = p.z;
                if (p.z > rmax[2]) rmax[2] = p.z;
            }
            for (int i = 0; i < 3; ++i)
            {
                box.rawMin[i]   =   rmin[i];
                box.rawMax[i]   =   rmax[i];
                double  a   =   double(rmin[i]) * hdr.scale[i] + hdr.offset[i];
                double  b   =   double(rmax[i]) * hdr.scale[i] + hdr.offset[i];
                box.min[i]      =   std::min(a,b);
                box.max[i]      =   std::max(a,b);
                box.center[i]   =   (box.min[i] + box.max[i]) * 0.5;
            }
        }
        /// <summary>
        /// 将叶子盒的点转为局部坐标网格并挂到节点上
        /// </summary>
        void    buildLeafMesh(Node& node,LasBox& box,const LasHeader& hdr,FEMaterial* material)
        {
            /// 8 字节/点:word0 = x(int16)|y(int16)<<16, word1 = z(int16)|rgb565<<16
            /// 位置按叶子盒各轴半宽量化，节点缩放负责还原为世界单位
            const   size_t  count   =   box.points.size();
            uints   packed;
            packed.reserve(count * 2);
            double  half[3] =   {0.0,0.0,0.0};
            double  inv[3]  =   {0.0,0.0,0.0};
            for (int i = 0; i < 3; ++i)
            {
                half[i] =   (box.max[i] - box.min[i]) * 0.5;
                inv[i]  =   half[i] > 0.0 ? 32767.0 / half[i] : 0.0;
            }
            for (auto& p : box.points)
            {
                const   double  pos[3]  =   {double(p.x) * hdr.scale[0] + hdr.offset[0] - box.center[0],
                                             double(p.y) * hdr.scale[1] + hdr.offset[1] - box.center[1],
                                             double(p.z) * hdr.scale[2] + hdr.offset[2] - box.center[2]};
                int     q[3]            =   {0,0,0};
                for (int i = 0; i < 3; ++i)
                {
                    const   long    v   =   std::lround(pos[i] * inv[i]);
                    q[i]                =   int((std::max)(-32767L,(std::min)(32767L,v)));
                }
                packed.push_back(uint32(uint16(int16(q[0]))) | (uint32(uint16(int16(q[1]))) << 16));
                packed.push_back(uint32(uint16(int16(q[2]))) | (uint32(p.color & 0xFFFFu) << 16));
            }
            Mesh    mesh    =   FEMeshBuilder::makePointMesh565(_ctx,packed);
            if (mesh != nullptr)
            {
                /// 退化轴(所有点同值)用 1.0 缩放，避免零缩放矩阵;此时量化值恒为 0。
                /// 顶点位置已归一化到 [-1,1]，节点缩放直接取叶子盒半宽即还原世界单位
                node->setLocalScaling(real3(half[0] > 0.0 ? half[0] : 1.0,
                                            half[1] > 0.0 ? half[1] : 1.0,
                                            half[2] > 0.0 ? half[2] : 1.0));
                node->setMesh(mesh.get());
                node->setMaterial(material);
            }
            LasPoints().swap(box.points);
        }
    protected:
        /// <summary>
        /// 解析 LAS 头
        /// </summary>
        static  bool    parseHeader(const uint8* data,uint64 size,LasHeader& hdr)
        {
            if (data == nullptr || size < 227)
                return  false;
            if (std::memcmp(data,"LASF",4) != 0)
                return  false;
            hdr.versionMajor        =   data[24];
            hdr.versionMinor        =   data[25];
            if (hdr.versionMajor != 1)
                return  false;
            hdr.headerSize          =   rdU16(data + 94);
            hdr.offsetToPointData   =   rdU32(data + 96);
            hdr.vlrCount            =   rdU32(data + 100);
            uint8   rawFormat       =   data[104];
            hdr.compressed          =   (rawFormat & 0xC0) != 0;
            hdr.pointFormat         =   rawFormat & 0x3F;
            hdr.pointRecordLength   =   rdU16(data + 105);
            hdr.legacyPointCount    =   rdU32(data + 107);
            for (int i = 0; i < 3; ++i)
            {
                hdr.scale[i]    =   rdF64(data + 131 + i * 8);
                hdr.offset[i]   =   rdF64(data + 155 + i * 8);
            }
            hdr.max[0]  =   rdF64(data + 179);
            hdr.min[0]  =   rdF64(data + 187);
            hdr.max[1]  =   rdF64(data + 195);
            hdr.min[1]  =   rdF64(data + 203);
            hdr.max[2]  =   rdF64(data + 211);
            hdr.min[2]  =   rdF64(data + 219);
            if (hdr.versionMinor >= 4)
            {
                if (size < 375)
                    return  false;
                hdr.extendedPointCount  =   rdU64(data + 247);
            }
            if (hdr.headerSize < 227 || hdr.offsetToPointData < hdr.headerSize)
                return  false;
            uint16  standard    =   standardLength(hdr.pointFormat);
            if (standard == 0 || hdr.pointRecordLength < standard)
                return  false;
            return  true;
        }
        /// <summary>
        /// 点数量(LAS 1.4 优先使用扩展计数)
        /// </summary>
        static  uint64  headerPointCount(const LasHeader& hdr)
        {
            if (hdr.versionMinor >= 4 && hdr.extendedPointCount > 0)
                return  hdr.extendedPointCount;
            return  hdr.legacyPointCount;
        }
        /// <summary>
        /// 颜色属性偏移，0xFFFF 表示该格式没有 RGB
        /// </summary>
        static  uint16  rgbOffsetOf(uint8 fmt)
        {
            switch (fmt)
            {
            case 2:
                return  20;
            case 3:
            case 5:
                /// 格式 3/5 在 RGB 之前有 8 字节 GPS Time
                return  28;
            case 7:
            case 8:
            case 10:
                return  30;
            default:
                return  0xFFFF;
            }
        }
        /// <summary>
        /// 各点格式的标准记录长度
        /// </summary>
        static  uint16  standardLength(uint8 fmt)
        {
            switch (fmt)
            {
            case 0:
                return  20;
            case 1:
                return  28;
            case 2:
                return  26;
            case 3:
                return  34;
            case 4:
                return  57;
            case 5:
                return  63;
            case 6:
                return  30;
            case 7:
                return  36;
            case 8:
                return  38;
            case 9:
                return  59;
            case 10:
                return  67;
            default:
                return  0;
            }
        }
        /// <summary>
        /// 打包 RGB565(R 在高 5 位)
        /// </summary>
        static  uint32  pack565(uint8 r,uint8 g,uint8 b)
        {
            return  uint32((uint32(r >> 3) << 11) | (uint32(g >> 2) << 5) | uint32(b >> 3));
        }
        /// <summary>
        /// ASPRS 分类色板
        /// </summary>
        static  uint32  classificationColor(uint8 cls)
        {
            uint8   r   =   180;
            uint8   g   =   180;
            uint8   b   =   180;
            switch (cls)
            {
            case 0:
            case 1:
                r   =   180;
                g   =   180;
                b   =   180;
                break;
            case 2:
                r   =   140;
                g   =   100;
                b   =   60;
                break;
            case 3:
                r   =   120;
                g   =   200;
                b   =   100;
                break;
            case 4:
                r   =   60;
                g   =   160;
                b   =   60;
                break;
            case 5:
                r   =   20;
                g   =   110;
                b   =   20;
                break;
            case 6:
                r   =   220;
                g   =   80;
                b   =   60;
                break;
            case 7:
                r   =   255;
                g   =   0;
                b   =   255;
                break;
            case 8:
                r   =   160;
                g   =   80;
                b   =   220;
                break;
            case 9:
                r   =   60;
                g   =   130;
                b   =   220;
                break;
            case 10:
                r   =   150;
                g   =   110;
                b   =   70;
                break;
            case 11:
                r   =   170;
                g   =   170;
                b   =   170;
                break;
            case 12:
                r   =   230;
                g   =   220;
                b   =   90;
                break;
            case 13:
                r   =   110;
                g   =   110;
                b   =   110;
                break;
            case 14:
                r   =   200;
                g   =   200;
                b   =   200;
                break;
            case 15:
                r   =   240;
                g   =   150;
                b   =   40;
                break;
            case 16:
                r   =   200;
                g   =   110;
                b   =   20;
                break;
            case 17:
                r   =   190;
                g   =   160;
                b   =   120;
                break;
            case 18:
                r   =   150;
                g   =   0;
                b   =   150;
                break;
            default:
                r   =   180;
                g   =   180;
                b   =   180;
                break;
            }
            return  pack565(r,g,b);
        }
        /// <summary>
        /// 文件名(不含扩展名)
        /// </summary>
        static  String  stemOf(const String& file)
        {
            std::filesystem::path   path(file);
            String  stem    =   path.stem().string();
            return  stem.empty() ? String("las") : stem;
        }
        /// <summary>
        /// 小端读取辅助
        /// </summary>
        static  uint16  rdU16(const uint8* p)
        {
            uint16  v   =   0;
            std::memcpy(&v,p,2);
            return  v;
        }
        static  uint32  rdU32(const uint8* p)
        {
            uint32  v   =   0;
            std::memcpy(&v,p,4);
            return  v;
        }
        static  int32   rdI32(const uint8* p)
        {
            int32   v   =   0;
            std::memcpy(&v,p,4);
            return  v;
        }
        static  uint64  rdU64(const uint8* p)
        {
            uint64  v   =   0;
            std::memcpy(&v,p,8);
            return  v;
        }
        static  double  rdF64(const uint8* p)
        {
            double  v   =   0.0;
            std::memcpy(&v,p,8);
            return  v;
        }
    };
}
