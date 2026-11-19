#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../node/FENode.hpp"
#include    "../../material/FEMaterialPoint565.hpp"
#include    "FEFormatPCO.hpp"
#include    "FEPointCloudLod.hpp"
#include    <cmath>
#include    <cstring>
#include    <filesystem>
#include    <vector>

namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatPCOReader,"{4B8E2A63-5D17-4C90-8F26-A7E3D9B15C48}");

    /// <summary>
    /// PCO 点云读取器(自研八叉树格式 v1，由 tool.las2pco 生成)。
    /// - 只解析 128 字节文件头与节点表，点数据由 LOD 调度器按需读取；
    /// - 按节点表还原 FENode 树：根节点平移为点云包围盒中心，
    ///   子节点平移为相对父节点中心的偏移；
    /// - 节点网格顶点为相对节点中心的 16 位量化坐标，节点局部缩放
    ///   由 relMax-relMin 还原(与转换工具量化完全一致)；
    /// - 根节点同步加载，保证加载完成即有粗略轮廓与有效包围盒；
    /// - 创建 FEPointCloudLod 负责后续的视锥/屏幕误差调度与增删节点。
    /// </summary>
    /// <remarks>
    /// ==================== PCO v1 文件布局(小端) ====================
    /// 一、Header(128B)
    ///   0   4   magic "PCO1"
    ///   4   2   version(1)
    ///   6   2   headerSize(128)
    ///   8   4   flags
    ///   12  4   pointStride(8)
    ///   16  4   attributeMask(bit0 = RGB565)
    ///   20  4   reserved
    ///   24  8   pointCount
    ///   32  4   nodeCount
    ///   36  4   nodeCapacity
    ///   40  4   maxDepth
    ///   44  4   rootSpacing(f32)
    ///   48  24  rootMin(f64 x3)
    ///   72  24  rootMax(f64 x3)
    ///   96  8   nodeTableOffset
    ///   104 8   nodeTableSize
    ///   112 8   payloadOffset
    ///   120 8   reserved
    ///
    /// 二、NodeTable(每节点 48B)
    ///   0   1   depth
    ///   1   1   childMask(位序即子节点槽位顺序)
    ///   2   2   reserved
    ///   4   4   pointCount
    ///   8   4   firstChild(0xFFFFFFFF 表示叶)
    ///   12  8   dataOffset(文件绝对偏移)
    ///   20  4   dataSize
    ///   24  12  min(f32 x3，相对根中心)
    ///   36  12  max(f32 x3，相对根中心)
    ///
    /// 三、Payload(每点 8B，节点间 16B 对齐)
    ///   word0 = x(int16) | y(int16) &lt;&lt; 16
    ///   word1 = z(int16) | rgb565(u16) &lt;&lt; 16
    ///   位置为相对节点中心、以 half/32767 为步长的量化整型。
    /// </remarks>
    class   FEFormatPCOReader :public FEStreamReader
    {
    protected:
        static  constexpr   uint32  kHeaderSize        =   128;
        static  constexpr   uint32  kNodeEntrySize     =   48;
        static  constexpr   uint32  kInvalidIndex      =   0xFFFFFFFFu;
        /// <summary>
        /// 节点数上限，防止损坏文件申请超大内存
        /// </summary>
        static  constexpr   uint32  kMaxNodeCount      =   8u * 1024u * 1024u;
        /// <summary>
        /// 文件头(仅保留本读取器使用的字段)
        /// </summary>
        struct  PcoHeader
        {
            uint32  version         =   0;
            uint32  headerSize      =   0;
            uint32  pointStride     =   0;
            uint32  attributeMask   =   0;
            uint64  pointCount      =   0;
            uint32  nodeCount       =   0;
            uint32  nodeCapacity    =   0;
            uint32  maxDepth        =   0;
            double  rootMin[3]      =   {0.0,0.0,0.0};
            double  rootMax[3]      =   {0.0,0.0,0.0};
            uint64  nodeTableOffset =   0;
            uint64  nodeTableSize   =   0;
        };
        /// <summary>
        /// 节点表项(48B)
        /// </summary>
        struct  PcoNode
        {
            uint32  pointCount  =   0;
            uint32  firstChild  =   kInvalidIndex;
            uint64  dataOffset  =   0;
            uint32  dataSize    =   0;
            float   relMin[3]   =   {0.0f,0.0f,0.0f};
            float   relMax[3]   =   {0.0f,0.0f,0.0f};
            uint8   depth       =   0;
            uint8   childMask   =   0;
        };
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatPCOReader)
    public:
        FEFormatPCOReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatPCOReader(const FEFormatPCOReader& other)
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
        /// 从文件加载 PCO 点云
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
                    LOG_ERR("PCO: open file failed: %s",file.c_str());
                    continue;
                }
                const   uint64  fileSize    =   reader.length();
                /// 资源包中的文件没有磁盘路径:整体读入内存后解析
                if (reader.isAssets())
                {
                    FEReaderHelper  helper(reader);
                    Buffer          buffer  =   helper.read(_ctx);
                    reader.close();
                    if (buffer == nullptr)
                    {
                        LOG_ERR("PCO: read asset failed: %s",file.c_str());
                        continue;
                    }
                    Node    root    =   parseBuffer(stemOf(file),buffer,file);
                    if (root != nullptr)
                        result.emplace_back(root);
                    continue;
                }
                /// 磁盘文件只读头部与节点表，点数据由 LOD 线程按需读取
                uint8   header[kHeaderSize] =   {0};
                reader.seek(0);
                if (fileSize < kHeaderSize || reader.readBuffer(header,kHeaderSize) != kHeaderSize)
                {
                    LOG_ERR("PCO: read header failed: %s",file.c_str());
                    reader.close();
                    continue;
                }
                PcoHeader   hdr;
                if (!parseHeader(header,hdr))
                {
                    LOG_ERR("PCO: invalid header: %s",file.c_str());
                    reader.close();
                    continue;
                }
                if (hdr.nodeTableSize != uint64(hdr.nodeCount) * kNodeEntrySize
                    || hdr.nodeTableOffset > fileSize
                    || hdr.nodeTableOffset + hdr.nodeTableSize > fileSize)
                {
                    LOG_ERR("PCO: node table out of range: %s",file.c_str());
                    reader.close();
                    continue;
                }
                std::vector<uint8>  table(size_t(hdr.nodeTableSize));
                reader.seek(hdr.nodeTableOffset);
                if (reader.readBuffer(table.data(),hdr.nodeTableSize) != hdr.nodeTableSize)
                {
                    LOG_ERR("PCO: read node table failed: %s",file.c_str());
                    reader.close();
                    continue;
                }
                reader.close();
                Node    root    =   buildCloud(stemOf(file),hdr,table.data(),fileSize,file,Buffer());
                if (root != nullptr)
                    result.emplace_back(root);
            }
            setProgress(1.0);
            return  result;
        }
        /// <summary>
        /// 从内存缓冲区读取 PCO 点云
        /// </summary>
        virtual Objects readBuffers(const Buffers& buffers) override
        {
            Objects result;
            for (auto& buffer : buffers)
            {
                if (buffer == nullptr || isCanceled())
                    continue;
                Node    root    =   parseBuffer("pco",buffer,String());
                if (root != nullptr)
                    result.emplace_back(root);
            }
            setProgress(1.0);
            return  result;
        }
    public:
        static  Formats formatList()
        {
            return  FEPCOFormat::formats(UUIDOF(FEFormatPCOReader));
        }
    protected:
        /// <summary>
        /// 内存模式:整个文件都在 Buffer 中
        /// </summary>
        Node    parseBuffer(const String& name,const Buffer& buffer,const String& file)
        {
            const   uint64  size    =   buffer->length();
            if (size < kHeaderSize)
            {
                LOG_ERR("PCO: buffer too small");
                return  nullptr;
            }
            const   uint8*  data    =   buffer->data();
            PcoHeader       hdr;
            if (!parseHeader(data,hdr))
            {
                LOG_ERR("PCO: invalid header");
                return  nullptr;
            }
            if (hdr.nodeTableSize != uint64(hdr.nodeCount) * kNodeEntrySize
                || hdr.nodeTableOffset > size
                || hdr.nodeTableOffset + hdr.nodeTableSize > size)
            {
                LOG_ERR("PCO: node table out of range");
                return  nullptr;
            }
            return  buildCloud(name,hdr,data + hdr.nodeTableOffset,size,file,buffer);
        }
        /// <summary>
        /// 解析并校验文件头
        /// </summary>
        bool    parseHeader(const uint8* data,PcoHeader& hdr) const
        {
            if (std::memcmp(data,"PCO1",4) != 0)
                return  false;
            hdr.version         =   rdU16(data + 4);
            hdr.headerSize      =   rdU16(data + 6);
            hdr.pointStride     =   rdU32(data + 12);
            hdr.attributeMask   =   rdU32(data + 16);
            hdr.pointCount      =   rdU64(data + 24);
            hdr.nodeCount       =   rdU32(data + 32);
            hdr.nodeCapacity    =   rdU32(data + 36);
            hdr.maxDepth        =   rdU32(data + 40);
            for (int k = 0; k < 3; ++k)
                hdr.rootMin[k]  =   rdF64(data + 48 + k * 8);
            for (int k = 0; k < 3; ++k)
                hdr.rootMax[k]  =   rdF64(data + 72 + k * 8);
            hdr.nodeTableOffset =   rdU64(data + 96);
            hdr.nodeTableSize   =   rdU64(data + 104);
            if (hdr.version != 1 || hdr.headerSize < kHeaderSize)
                return  false;
            /// 当前版本只支持 8 字节/点(位置 + RGB565)
            if (hdr.pointStride != 8)
                return  false;
            return  hdr.nodeCount > 0 && hdr.nodeCount <= kMaxNodeCount;
        }
        /// <summary>
        /// 解析节点表项
        /// </summary>
        static  void    parseNode(const uint8* data,PcoNode& node)
        {
            node.depth          =   data[0];
            node.childMask      =   data[1];
            node.pointCount     =   rdU32(data + 4);
            node.firstChild     =   rdU32(data + 8);
            node.dataOffset     =   rdU64(data + 12);
            node.dataSize       =   rdU32(data + 20);
            for (int k = 0; k < 3; ++k)
                node.relMin[k]  =   rdF32(data + 24 + k * 4);
            for (int k = 0; k < 3; ++k)
                node.relMax[k]  =   rdF32(data + 36 + k * 4);
        }
        /// <summary>
        /// 根据节点表构建节点树并创建 LOD 调度器
        /// </summary>
        Node    buildCloud(const String& name,const PcoHeader& hdr,const uint8* table,
                           uint64 dataLimit,const String& file,const Buffer& memory)
        {
            const   uint32  count   =   hdr.nodeCount;
            std::vector<PcoNode>    entries(count);
            std::vector<uint32>     parents(count,kInvalidIndex);
            for (uint32 i = 0; i < count; ++i)
            {
                PcoNode&    e   =   entries[i];
                parseNode(table + size_t(i) * kNodeEntrySize,e);
                /// 点数据必须完整落在文件(或内存)内
                if (uint64(e.pointCount) * 8 > e.dataSize
                    || e.dataOffset + e.dataSize < e.dataOffset
                    || e.dataOffset + e.dataSize > dataLimit)
                {
                    LOG_ERR("PCO: node %u data out of range",i);
                    return  nullptr;
                }
                if (e.childMask == 0)
                    continue;
                uint32  slots   =   0;
                for (uint32 bit = 0; bit < 8; ++bit)
                {
                    if ((e.childMask & (1u << bit)) != 0)
                        ++slots;
                }
                if (uint64(e.firstChild) + slots > count)
                {
                    LOG_ERR("PCO: node %u children out of range",i);
                    return  nullptr;
                }
                for (uint32 bit = 0, slot = 0; bit < 8; ++bit)
                {
                    if ((e.childMask & (1u << bit)) == 0)
                        continue;
                    const   uint32  child   =   e.firstChild + slot++;
                    /// 广度优先顺序保证父下标一定小于子下标
                    if (child <= i)
                    {
                        LOG_ERR("PCO: node %u child %u order invalid",i,child);
                        return  nullptr;
                    }
                    parents[child]  =   i;
                }
            }
            const   double  rootCenter[3]   =   {(hdr.rootMin[0] + hdr.rootMax[0]) * 0.5,
                                                 (hdr.rootMin[1] + hdr.rootMax[1]) * 0.5,
                                                 (hdr.rootMin[2] + hdr.rootMax[2]) * 0.5};
            /// 创建共享材质(与 LAS 读取器一致的距离衰减基准)
            Material    material    =   new FEMaterialPoint565(_ctx);
            {
                const   double  dx  =   hdr.rootMax[0] - hdr.rootMin[0];
                const   double  dy  =   hdr.rootMax[1] - hdr.rootMin[1];
                const   double  dz  =   hdr.rootMax[2] - hdr.rootMin[2];
                const   double  diag=   std::sqrt(dx * dx + dy * dy + dz * dz);
                if (diag > 0.0)
                    material->cast<FEMaterialPoint565>()->setPointSizeScale(float(diag * 1.5));
            }
            SharedPtr<FEPointCloudLod>  lod =   new FEPointCloudLod(_ctx);
            lod->setMaterial(material);
            std::vector<Node>   nodes(count);
            for (uint32 i = 0; i < count; ++i)
            {
                const   PcoNode&    e       =   entries[i];
                double              center[3];
                double              halfLen[3];
                /// 世界缩放:量化顶点(±32767)还原为世界单位；退化轴用 1 避免零缩放
                double              worldScale[3];
                for (int k = 0; k < 3; ++k)
                {
                    center[k]   =   (double(e.relMin[k]) + double(e.relMax[k])) * 0.5;
                    halfLen[k]  =   (double(e.relMax[k]) - double(e.relMin[k])) * 0.5;
                    worldScale[k]   =   halfLen[k] > 0.0 ? halfLen[k] / 32767.0 : 1.0;
                }
                Node    node    =   new FENode(_ctx);
                node->setName(i == 0 ? name : name + "_" + std::to_string(i));
                const   uint32  parent  =   parents[i];
                FEPointCloudLod::NodeInfo   info;
                if (i == 0 || parent == kInvalidIndex)
                {
                    node->setLocalTranslation(real3(rootCenter[0],rootCenter[1],rootCenter[2]));
                    for (int k = 0; k < 3; ++k)
                        info.localScale[k]  =   worldScale[k];
                }
                else
                {
                    const   PcoNode&    pe  =   entries[parent];
                    double              pc[3];
                    double              parentScale[3];
                    for (int k = 0; k < 3; ++k)
                    {
                        pc[k]           =   (double(pe.relMin[k]) + double(pe.relMax[k])) * 0.5;
                        const   double  parentHalf  =   (double(pe.relMax[k]) - double(pe.relMin[k])) * 0.5;
                        parentScale[k]  =   parentHalf > 0.0 ? parentHalf / 32767.0 : 1.0;
                    }
                    /// 父节点缩放会被子节点继承:本地平移先除以父世界缩放，
                    /// 本地缩放取"本节点世界缩放/父世界缩放"，父子合并后正好还原世界坐标
                    node->setLocalTranslation(real3((center[0] - pc[0]) / parentScale[0],
                                                    (center[1] - pc[1]) / parentScale[1],
                                                    (center[2] - pc[2]) / parentScale[2]));
                    for (int k = 0; k < 3; ++k)
                        info.localScale[k]  =   worldScale[k] / parentScale[k];
                    nodes[parent]->addChild(node);
                }
                nodes[i]    =   node;

                for (int k = 0; k < 3; ++k)
                {
                    info.min[k]         =   rootCenter[k] + double(e.relMin[k]);
                    info.max[k]         =   rootCenter[k] + double(e.relMax[k]);
                    info.center[k]      =   rootCenter[k] + center[k];
                    info.relMin[k]      =   e.relMin[k];
                    info.relMax[k]      =   e.relMax[k];
                }
                info.radius         =   std::sqrt(halfLen[0] * halfLen[0]
                                                + halfLen[1] * halfLen[1]
                                                + halfLen[2] * halfLen[2]);
                info.dataOffset     =   e.dataOffset;
                info.dataSize       =   e.dataSize;
                info.pointCount     =   e.pointCount;
                info.firstChild     =   e.firstChild;
                info.childMask      =   e.childMask;
                info.depth          =   e.depth;
                lod->addNode(info,node);
            }
            lod->setSource(file,memory);
            /// 根节点同步加载:保证返回的节点有有效包围盒(供相机取景)
            if (!lod->loadRootSync())
            {
                LOG_ERR("PCO: load root node failed: %s",name.c_str());
                return  nullptr;
            }
            lod->start();
            /// LOD 必须挂在场景更新队列上才能每帧调度
            FEScene*    scene   =   _ctx.scene();
            if (scene == nullptr)
            {
                LOG_ERR("PCO: no scene, LOD disabled: %s",name.c_str());
            }
            else
            {
                QueueItems  items;
                QueueItem   item    =   new FEQueueItem(_ctx);
                item->_runable  +=  [scene,lod]()
                {
                    FEPointCloudLod::attach(*scene,lod);
                };
                items.emplace_back(item);
                scene->postRunable(std::move(items));
            }
            LOG_INF("PCO: %s nodes=%u points=%llu",name.c_str(),
                    count,(unsigned long long)hdr.pointCount);
            return  nodes.empty() ? nullptr : nodes[0];
        }
        /// <summary>
        /// 文件名(不含扩展名)
        /// </summary>
        static  String  stemOf(const String& file)
        {
            std::filesystem::path   path(file);
            String  stem    =   path.stem().string();
            return  stem.empty() ? String("pco") : stem;
        }
        /// <summary>
        /// 小端读取辅助(仅在小端平台使用)
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
        static  uint64  rdU64(const uint8* p)
        {
            uint64  v   =   0;
            std::memcpy(&v,p,8);
            return  v;
        }
        static  float   rdF32(const uint8* p)
        {
            float   v   =   0.0f;
            std::memcpy(&v,p,4);
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
