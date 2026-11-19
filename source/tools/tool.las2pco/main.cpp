/// tool.las2pco
/// 将 .las 点云离线转换为 PCO(Point Cloud Octree) 单文件格式，供引擎按需动态加载。
///
/// PCO v1 布局(小端):
///   [Header 128B]
///     magic "PCO1" / version(u16) / headerSize(u16) / flags(u32) /
///     pointStride(u32) / attributeMask(u32) / reserved0(u32) /
///     pointCount(u64) / nodeCount(u32) / nodeCapacity(u32) / maxDepth(u32) /
///     rootSpacing(f32) / rootMin(f64x3) / rootMax(f64x3) /
///     nodeTableOffset(u64) / nodeTableSize(u64) / payloadOffset(u64) / reserved1(u64)
///   [NodeTable] 每节点 48B(广度优先):
///     depth(u8) / childMask(u8) / reserved(u16) / pointCount(u32) /
///     firstChild(u32，0xFFFFFFFF 表示叶子) / dataOffset(u64) / dataSize(u32) /
///     min(f32x3) / max(f32x3)      —— min/max 相对根中心
///   [Payload] 节点点数据，16 字节对齐，每点 8B:
///     word0 = x(int16) | y(int16) << 16
///     word1 = z(int16) | rgb565(u16) << 16
///     坐标为相对节点中心、按节点半边长量化的整数
///
/// 八叉树采用“加性采样”: 每个节点最多保留 nodeCapacity 个点，
/// 未保留的点下沉给 8 个子节点；节点点数不超过容量时全部保留并终止细分。
/// 这样从根开始逐层加载即可获得由粗到细的完整轮廓。
///
/// 用法:
///   tool.las2pco <input.las> [-o output.pco]
///                [--node-capacity 50000] [--spacing 0]
///                [--max-depth 32] [--help]

#include    <algorithm>
#include    <chrono>
#include    <cmath>
#include    <cstdint>
#include    <cstdio>
#include    <cstdlib>
#include    <cstring>
#include    <deque>
#include    <filesystem>
#include    <limits>
#include    <string>
#include    <unordered_set>
#include    <utility>
#include    <vector>

namespace
{
    using   String  =   std::string;

    constexpr   uint32_t    kInvalidChild   =   0xFFFFFFFFu;
    constexpr   uint64_t    kHeaderSize     =   128;
    constexpr   uint64_t    kNodeEntrySize  =   48;
    constexpr   uint32_t    kMaxPointsCap   =   1u << 20;

    /// <summary>
    /// 解析阶段的点: 原始整型坐标 + RGB565 颜色(与引擎渲染格式一致)
    /// </summary>
    struct  LasPoint
    {
        int32_t     x       =   0;
        int32_t     y       =   0;
        int32_t     z       =   0;
        uint16_t    color   =   0;
    };

    /// <summary>
    /// LAS 头中本工具需要的字段
    /// </summary>
    struct  LasHeader
    {
        uint8_t     versionMajor        =   0;
        uint8_t     versionMinor        =   0;
        uint16_t    headerSize          =   0;
        uint32_t    offsetToPointData   =   0;
        uint8_t     pointFormat         =   0;
        uint16_t    pointRecordLength   =   0;
        uint64_t    pointCount          =   0;
        double      scale[3]            =   {0.0,0.0,0.0};
        double      offset[3]           =   {0.0,0.0,0.0};
        double      min[3]              =   {0.0,0.0,0.0};
        double      max[3]              =   {0.0,0.0,0.0};
        bool        compressed          =   false;
    };

    /// <summary>
    /// 八叉树节点构建期数据
    /// </summary>
    struct  BuildNode
    {
        double                  min[3]      =   {0.0,0.0,0.0};
        double                  max[3]      =   {0.0,0.0,0.0};
        uint32_t                depth       =   0;
        uint8_t                 childMask   =   0;
        uint32_t                firstChild  =   kInvalidChild;
        std::vector<LasPoint>   points;
    };

    /// <summary>
    /// 构建队列项: 节点下标 + 待采样点
    /// </summary>
    struct  BuildItem
    {
        uint32_t                node    =   0;
        uint32_t                depth   =   0;
        std::vector<LasPoint>   points;
    };

    /// <summary>
    /// 转换参数
    /// </summary>
    struct  Options
    {
        String      input;
        String      output;
        uint32_t    nodeCapacity    =   50000;
        uint32_t    maxDepth        =   32;
        double      spacing         =   0.0;    /// 0 表示按节点体积自适应
        bool        showHelp        =   false;
    };

    // ==================== 小端读取 ====================

    inline  uint16_t    rdU16(const uint8_t* p)
    {
        uint16_t v = 0;
        std::memcpy(&v,p,2);
        return v;
    }
    inline  uint32_t    rdU32(const uint8_t* p)
    {
        uint32_t v = 0;
        std::memcpy(&v,p,4);
        return v;
    }
    inline  int32_t     rdI32(const uint8_t* p)
    {
        int32_t v = 0;
        std::memcpy(&v,p,4);
        return v;
    }
    inline  uint64_t    rdU64(const uint8_t* p)
    {
        uint64_t v = 0;
        std::memcpy(&v,p,8);
        return v;
    }
    inline  double      rdF64(const uint8_t* p)
    {
        double v = 0.0;
        std::memcpy(&v,p,8);
        return v;
    }

    // ==================== 小端写出 ====================

    inline  void    wrU16(uint8_t* p,uint16_t v)    {   std::memcpy(p,&v,2);    }
    inline  void    wrU32(uint8_t* p,uint32_t v)    {   std::memcpy(p,&v,4);    }
    inline  void    wrU64(uint8_t* p,uint64_t v)    {   std::memcpy(p,&v,8);    }
    inline  void    wrF32(uint8_t* p,float v)       {   std::memcpy(p,&v,4);    }
    inline  void    wrF64(uint8_t* p,double v)      {   std::memcpy(p,&v,8);    }

    // ==================== LAS 解析 ====================

    inline  uint32_t    pack565(uint8_t r,uint8_t g,uint8_t b)
    {
        return  (uint32_t(r >> 3) << 11) | (uint32_t(g >> 2) << 5) | uint32_t(b >> 3);
    }

    /// <summary>
    /// ASPRS 分类色板(与引擎 LAS 读取器保持一致)
    /// </summary>
    uint32_t    classificationColor(uint8_t cls)
    {
        uint8_t r = 180,g = 180,b = 180;
        switch (cls)
        {
        case 2:     r = 140;    g = 100;    b = 60;     break;
        case 3:     r = 120;    g = 200;    b = 100;    break;
        case 4:     r = 60;     g = 160;    b = 60;     break;
        case 5:     r = 20;     g = 110;    b = 20;     break;
        case 6:     r = 220;    g = 80;     b = 60;     break;
        case 7:     r = 255;    g = 0;      b = 255;    break;
        case 8:     r = 160;    g = 80;     b = 220;    break;
        case 9:     r = 60;     g = 130;    b = 220;    break;
        case 10:    r = 150;    g = 110;    b = 70;     break;
        case 12:    r = 230;    g = 220;    b = 90;     break;
        case 15:    r = 240;    g = 150;    b = 40;     break;
        case 16:    r = 200;    g = 110;    b = 20;     break;
        case 17:    r = 190;    g = 160;    b = 120;    break;
        case 18:    r = 150;    g = 0;      b = 150;    break;
        default:    break;
        }
        return  pack565(r,g,b);
    }

    uint16_t    rgbOffsetOf(uint8_t fmt)
    {
        switch (fmt)
        {
        case 2:             return  20;
        case 3:
        case 5:             return  28;
        case 7:
        case 8:
        case 10:            return  30;
        default:            return  0xFFFF;
        }
    }

    uint16_t    standardLength(uint8_t fmt)
    {
        switch (fmt)
        {
        case 0:     return  20;
        case 1:     return  28;
        case 2:     return  26;
        case 3:     return  34;
        case 4:     return  57;
        case 5:     return  63;
        case 6:     return  30;
        case 7:     return  36;
        case 8:     return  38;
        case 9:     return  59;
        case 10:    return  67;
        default:    return  0;
        }
    }

    /// <summary>
    /// 解析 LAS Public Header Block
    /// </summary>
    bool    parseHeader(const uint8_t* data,uint64_t size,LasHeader& hdr)
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
        const   uint8_t rawFormat   =   data[104];
        hdr.compressed          =   (rawFormat & 0xC0) != 0;
        hdr.pointFormat         =   rawFormat & 0x3F;
        hdr.pointRecordLength   =   rdU16(data + 105);
        const   uint32_t    legacyCount =   rdU32(data + 107);
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
        hdr.pointCount  =   legacyCount;
        if (hdr.versionMinor >= 4)
        {
            if (size < 375)
                return  false;
            const   uint64_t    extCount    =   rdU64(data + 247);
            if (extCount > 0)
                hdr.pointCount  =   extCount;
        }
        if (hdr.headerSize < 227 || hdr.offsetToPointData < hdr.headerSize)
            return  false;
        const   uint16_t    standard    =   standardLength(hdr.pointFormat);
        if (standard == 0 || hdr.pointRecordLength < standard)
            return  false;
        return  true;
    }

    /// <summary>
    /// 依据点格式解析颜色: RGB -> ASPRS 分类 -> 强度灰度
    /// </summary>
    uint16_t    colorOf(const LasHeader& hdr,const uint8_t* rec)
    {
        const   uint16_t    rgbOffset   =   rgbOffsetOf(hdr.pointFormat);
        if (rgbOffset != 0xFFFF)
        {
            const   uint16_t    r16 =   rdU16(rec + rgbOffset);
            const   uint16_t    g16 =   rdU16(rec + rgbOffset + 2);
            const   uint16_t    b16 =   rdU16(rec + rgbOffset + 4);
            uint8_t r   =   uint8_t(r16 >> 8);
            uint8_t g   =   uint8_t(g16 >> 8);
            uint8_t b   =   uint8_t(b16 >> 8);
            if ((r16 | g16 | b16) <= 0xFFu)
            {
                r   =   uint8_t(r16);
                g   =   uint8_t(g16);
                b   =   uint8_t(b16);
            }
            if ((r | g | b) != 0)
                return  uint16_t(pack565(r,g,b));
        }
        const   uint8_t cls =   (hdr.pointFormat <= 5) ? uint8_t(rec[15] & 0x1Fu) : rec[16];
        if (cls > 1)
            return  uint16_t(classificationColor(cls));
        const   uint16_t    intensity   =   rdU16(rec + 12);
        const   uint8_t     gray        =   intensity <= 0xFFu ? uint8_t(intensity)
                                                              : uint8_t(intensity >> 8);
        return  uint16_t(pack565(gray,gray,gray));
    }

    /// <summary>
    /// 流式读取全部点记录
    /// </summary>
    bool    readPoints(FILE* file,const LasHeader& hdr,std::vector<LasPoint>& points)
    {
        const   uint64_t    fileSize    =   [file]() -> uint64_t
        {
            const   int64_t cur =   _ftelli64(file);
            _fseeki64(file,0,SEEK_END);
            const   int64_t end =   _ftelli64(file);
            _fseeki64(file,cur,SEEK_SET);
            return  end > 0 ? uint64_t(end) : 0;
        }();
        if (fileSize <= hdr.offsetToPointData || hdr.pointRecordLength == 0)
            return  false;
        uint64_t    count   =   (fileSize - hdr.offsetToPointData) / hdr.pointRecordLength;
        if (hdr.pointCount > 0 && hdr.pointCount < count)
            count   =   hdr.pointCount;
        if (count == 0)
            return  false;
        points.clear();
        points.reserve(size_t(count));
        if (_fseeki64(file,int64_t(hdr.offsetToPointData),SEEK_SET) != 0)
            return  false;
        const   uint64_t    perChunk    =   std::max<uint64_t>(1,(1ULL << 20) / hdr.pointRecordLength);
        std::vector<uint8_t>    chunk;
        uint64_t    done    =   0;
        while (done < count)
        {
            const   uint64_t    n   =   std::min<uint64_t>(perChunk,count - done);
            const   uint64_t    want=   n * hdr.pointRecordLength;
            chunk.resize(size_t(want));
            if (std::fread(chunk.data(),1,size_t(want),file) != size_t(want))
            {
                std::fprintf(stderr,"[ERROR] LAS point data truncated at %llu/%llu\n",
                             (unsigned long long)done,(unsigned long long)count);
                return  false;
            }
            for (uint64_t i = 0; i < n; ++i)
            {
                const   uint8_t*    rec =   chunk.data() + i * hdr.pointRecordLength;
                LasPoint    p;
                p.x     =   rdI32(rec);
                p.y     =   rdI32(rec + 4);
                p.z     =   rdI32(rec + 8);
                p.color =   colorOf(hdr,rec);
                points.push_back(p);
            }
            done    +=  n;
            std::printf("\r  reading LAS ... %.1f%%",100.0 * double(done) / double(count));
            std::fflush(stdout);
        }
        std::printf("\r  reading LAS ... 100.0%%\n");
        return  true;
    }

    // ==================== 八叉树构建 ====================

    /// <summary>
    /// 计算点集的世界包围盒
    /// </summary>
    void    computeBounds(const std::vector<LasPoint>& points,const LasHeader& hdr,double mn[3],double mx[3])
    {
        int32_t rmin[3] =   {std::numeric_limits<int32_t>::max(),
                             std::numeric_limits<int32_t>::max(),
                             std::numeric_limits<int32_t>::max()};
        int32_t rmax[3] =   {std::numeric_limits<int32_t>::min(),
                             std::numeric_limits<int32_t>::min(),
                             std::numeric_limits<int32_t>::min()};
        for (const auto& p : points)
        {
            const   int32_t v[3]    =   {p.x,p.y,p.z};
            for (int k = 0; k < 3; ++k)
            {
                rmin[k] =   std::min(rmin[k],v[k]);
                rmax[k] =   std::max(rmax[k],v[k]);
            }
        }
        for (int k = 0; k < 3; ++k)
        {
            const   double  a   =   double(rmin[k]) * hdr.scale[k] + hdr.offset[k];
            const   double  b   =   double(rmax[k]) * hdr.scale[k] + hdr.offset[k];
            mn[k]   =   std::min(a,b);
            mx[k]   =   std::max(a,b);
        }
    }

    /// <summary>
    /// 把世界坐标换算为相对节点最小角的网格单元键。
    /// 每轴 21 位(约 200 万格)，足够覆盖单节点的采样分辨率
    /// </summary>
    uint64_t    cellKey(const LasPoint& p,const LasHeader& hdr,const double mn[3],double spacing)
    {
        const   int32_t raw[3]  =   {p.x,p.y,p.z};
        uint64_t    key =   0;
        for (int k = 0; k < 3; ++k)
        {
            const   double  w   =   double(raw[k]) * hdr.scale[k] + hdr.offset[k];
            int64_t cell    =   int64_t(std::floor((w - mn[k]) / spacing));
            cell    =   std::max<int64_t>(0,std::min<int64_t>(cell,0x1FFFFF));
            key     |=  (uint64_t(cell) << (k * 21));
        }
        return  key;
    }

    /// <summary>
    /// 在 [0,kept) 留下采样点，其余点移到尾部。
    /// 先按体积估算间距，若采样数超过容量则放大间距重采，
    /// 保证单节点点数不超过 nodeCapacity
    /// </summary>
    uint64_t    samplePrefix(std::vector<LasPoint>& points,const LasHeader& hdr,
                             const double mn[3],const double mx[3],
                             double spacing,uint32_t capacity)
    {
        const   double  ext[3]  =   {std::max(1e-6,mx[0] - mn[0]),
                                     std::max(1e-6,mx[1] - mn[1]),
                                     std::max(1e-6,mx[2] - mn[2])};
        if (spacing <= 0.0)
            spacing =   std::cbrt(ext[0] * ext[1] * ext[2] / double(capacity));
        if (spacing <= 0.0)
            spacing =   1e-6;
        const   double  maxExt  =   std::max(ext[0],std::max(ext[1],ext[2]));
        for (int iter = 0; iter < 64; ++iter)
        {
            std::unordered_set<uint64_t>    grid;
            grid.reserve(size_t(std::min<uint64_t>(points.size(),uint64_t(capacity) * 2)));
            size_t  kept    =   0;
            for (size_t i = 0; i < points.size(); ++i)
            {
                if (grid.insert(cellKey(points[i],hdr,mn,spacing)).second)
                {
                    if (kept != i)
                        std::swap(points[kept],points[i]);
                    ++kept;
                }
            }
            if (kept <= capacity || spacing > maxExt * 2.0)
                return  uint64_t(kept);
            /// 放大间距后重采样: 3D 体密度按立方根收敛
            const   double  grow    =   std::max(1.25,std::cbrt(double(kept) / double(capacity)));
            spacing *=  grow;
        }
        return  uint64_t(points.size());
    }

    /// <summary>
    /// 广度优先构建八叉树
    /// </summary>
    void    buildOctree(std::vector<BuildNode>& nodes,const LasHeader& hdr,
                        std::vector<LasPoint>&& allPoints,const Options& opt)
    {
        nodes.clear();
        nodes.reserve(size_t(std::max<uint64_t>(1,allPoints.size() / opt.nodeCapacity)) * 2);
        std::deque<BuildItem>   queue;
        nodes.emplace_back();
        queue.push_back({0,0,std::move(allPoints)});
        /// 每层统计: 节点数 / 点数
        std::vector<std::pair<uint64_t,uint64_t>>   levelStat;
        uint64_t    processed   =   0;
        const   uint64_t    total   =   queue.front().points.size();
        while (!queue.empty())
        {
            BuildItem   item    =   std::move(queue.front());
            queue.pop_front();
            /// 注意: 后面对 nodes 的 emplace_back 会使引用失效，
            /// 因此节点字段一律先写入局部变量，最后按下标回填
            const   uint32_t    nodeIndex   =   item.node;
            double  nodeMin[3],nodeMax[3];
            computeBounds(item.points,hdr,nodeMin,nodeMax);
            if (item.depth >= levelStat.size())
                levelStat.resize(item.depth + 1);
            levelStat[item.depth].first  +=  1;
            levelStat[item.depth].second +=  item.points.size();
            /// 点数不超容量，或到达最大深度: 全部保留，不再细分
            if (item.points.size() > opt.nodeCapacity && item.depth < opt.maxDepth)
            {
                const   double  levelSpacing    =   opt.spacing > 0.0
                                                    ?   opt.spacing / double(uint64_t(1) << std::min<uint32_t>(item.depth,52))
                                                    :   0.0;
                const   uint64_t    kept    =   samplePrefix(item.points,hdr,nodeMin,nodeMax,
                                                             levelSpacing,opt.nodeCapacity);
                const   double  center[3]   =   {(nodeMin[0] + nodeMax[0]) * 0.5,
                                                 (nodeMin[1] + nodeMax[1]) * 0.5,
                                                 (nodeMin[2] + nodeMax[2]) * 0.5};
                std::vector<LasPoint>   children[8];
                for (size_t i = size_t(kept); i < item.points.size(); ++i)
                {
                    const   LasPoint&   p   =   item.points[i];
                    const   double  w[3]    =   {double(p.x) * hdr.scale[0] + hdr.offset[0],
                                                 double(p.y) * hdr.scale[1] + hdr.offset[1],
                                                 double(p.z) * hdr.scale[2] + hdr.offset[2]};
                    const   uint32_t    bit =   uint32_t(w[0] >= center[0] ? 1 : 0)
                                            |   uint32_t(w[1] >= center[1] ? 2 : 0)
                                            |   uint32_t(w[2] >= center[2] ? 4 : 0);
                    children[bit].push_back(p);
                }
                item.points.resize(size_t(kept));
                uint32_t    firstChild  =   kInvalidChild;
                uint8_t     childMask   =   0;
                const   uint32_t    depth   =   item.depth + 1;
                for (uint32_t bit = 0; bit < 8; ++bit)
                {
                    if (children[bit].empty())
                        continue;
                    if (firstChild == kInvalidChild)
                        firstChild  =   uint32_t(nodes.size());
                    childMask  |=  uint8_t(1u << bit);
                    queue.push_back({uint32_t(nodes.size()),depth,std::move(children[bit])});
                    nodes.emplace_back();
                }
                BuildNode&  node    =   nodes[nodeIndex];
                node.depth  =   item.depth;
                node.min[0] =   nodeMin[0]; node.min[1] =   nodeMin[1]; node.min[2] =   nodeMin[2];
                node.max[0] =   nodeMax[0]; node.max[1] =   nodeMax[1]; node.max[2] =   nodeMax[2];
                node.firstChild =   firstChild;
                node.childMask  =   childMask;
                node.points     =   std::move(item.points);
            }
            else
            {
                BuildNode&  node    =   nodes[nodeIndex];
                node.depth  =   item.depth;
                node.min[0] =   nodeMin[0]; node.min[1] =   nodeMin[1]; node.min[2] =   nodeMin[2];
                node.max[0] =   nodeMax[0]; node.max[1] =   nodeMax[1]; node.max[2] =   nodeMax[2];
                node.points =   std::move(item.points);
            }
            processed   +=  nodes[nodeIndex].points.size();
            if ((queue.size() & 1023u) == 0u)
            {
                std::printf("\r  building octree ... nodes=%zu queued=%zu",
                            nodes.size(),queue.size());
                std::fflush(stdout);
            }
        }
        std::printf("\r  building octree ... nodes=%zu done      \n",nodes.size());
        std::printf("  hierarchy:\n");
        for (size_t d = 0; d < levelStat.size(); ++d)
        {
            std::printf("    level %-2zu nodes=%-8llu points=%llu\n",d,
                        (unsigned long long)levelStat[d].first,
                        (unsigned long long)levelStat[d].second);
        }
        std::printf("  total points in tree = %llu (input %llu)%s\n",
                    (unsigned long long)processed,(unsigned long long)total,
                    processed == total ? "" : "  [WARN] point count mismatch");
    }

    // ==================== PCO 写出 ====================

    /// <summary>
    /// 节点表项(48B)
    /// </summary>
    struct  NodeEntry
    {
        uint32_t    pointCount  =   0;
        uint32_t    firstChild  =   kInvalidChild;
        uint64_t    dataOffset  =   0;
        uint32_t    dataSize    =   0;
        float       min[3]      =   {0.0f,0.0f,0.0f};
        float       max[3]      =   {0.0f,0.0f,0.0f};
        uint8_t     depth       =   0;
        uint8_t     childMask   =   0;
    };

    void    writeNodeEntry(uint8_t* dst,const NodeEntry& e)
    {
        dst[0]  =   e.depth;
        dst[1]  =   e.childMask;
        wrU16(dst + 2,0);
        wrU32(dst + 4,e.pointCount);
        wrU32(dst + 8,e.firstChild);
        wrU64(dst + 12,e.dataOffset);
        wrU32(dst + 20,e.dataSize);
        for (int k = 0; k < 3; ++k)
            wrF32(dst + 24 + k * 4,e.min[k]);
        for (int k = 0; k < 3; ++k)
            wrF32(dst + 36 + k * 4,e.max[k]);
    }

    /// <summary>
    /// 计算单个节点的量化参数: 中心/半边长均先转 float，
    /// 保证写端量化与运行端反量化使用完全相同的数值
    /// </summary>
    void    quantParams(const BuildNode& node,const double rootCenter[3],
                        double center[3],double half[3],
                        float relMin[3],float relMax[3])
    {
        for (int k = 0; k < 3; ++k)
        {
            const   float   lo  =   float(node.min[k] - rootCenter[k]);
            const   float   hi  =   float(node.max[k] - rootCenter[k]);
            relMin[k]   =   lo;
            relMax[k]   =   hi;
            center[k]   =   (double(lo) + double(hi)) * 0.5;
            half[k]     =   (double(hi) - double(lo)) * 0.5;
        }
    }

    /// <summary>
    /// 写出整个 PCO 文件
    /// </summary>
    bool    writePco(const String& path,std::vector<BuildNode>& nodes,const LasHeader& hdr,
                     const Options& opt)
    {
        const   double  rootCenter[3]   =   {(hdr.min[0] + hdr.max[0]) * 0.5,
                                             (hdr.min[1] + hdr.max[1]) * 0.5,
                                             (hdr.min[2] + hdr.max[2]) * 0.5};
        const   uint64_t    nodeCount   =   nodes.size();
        /// 先算出各节点数据偏移(16 字节对齐)
        std::vector<NodeEntry>  entries(nodes.size());
        uint64_t    cursor  =   kHeaderSize + nodeCount * kNodeEntrySize;
        uint64_t    totalPoints =   0;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const   BuildNode&  node    =   nodes[i];
            NodeEntry&          e       =   entries[i];
            double  center[3],half[3];
            quantParams(node,rootCenter,center,half,e.min,e.max);
            (void)center;
            (void)half;
            e.depth         =   uint8_t(node.depth);
            e.childMask     =   node.childMask;
            e.pointCount    =   uint32_t(node.points.size());
            e.firstChild    =   node.firstChild;
            e.dataSize      =   uint32_t(node.points.size() * 8);
            cursor          =   (cursor + 15) & ~uint64_t(15);
            e.dataOffset    =   cursor;
            cursor          +=  e.dataSize;
            totalPoints     +=  node.points.size();
        }

        FILE*   file    =   std::fopen(path.c_str(),"wb");
        if (file == nullptr)
        {
            std::fprintf(stderr,"[ERROR] cannot create output file: %s\n",path.c_str());
            return  false;
        }
        std::vector<uint8_t>    header(kHeaderSize,0);
        std::memcpy(header.data(),"PCO1",4);
        wrU16(header.data() + 4,1);                                     /// version
        wrU16(header.data() + 6,uint16_t(kHeaderSize));                 /// header size
        wrU32(header.data() + 8,0);                                     /// flags
        wrU32(header.data() + 12,8);                                    /// point stride
        wrU32(header.data() + 16,1);                                    /// attributeMask: RGB565
        wrU32(header.data() + 20,0);                                    /// reserved0
        wrU64(header.data() + 24,totalPoints);
        wrU32(header.data() + 32,uint32_t(nodeCount));
        wrU32(header.data() + 36,opt.nodeCapacity);
        wrU32(header.data() + 40,opt.maxDepth);
        {
            const   double  ext[3]  =   {std::max(1e-6,hdr.max[0] - hdr.min[0]),
                                         std::max(1e-6,hdr.max[1] - hdr.min[1]),
                                         std::max(1e-6,hdr.max[2] - hdr.min[2])};
            const   float   rootSpacing =   opt.spacing > 0.0
                                            ?   float(opt.spacing)
                                            :   float(std::cbrt(ext[0] * ext[1] * ext[2]
                                                                / double(opt.nodeCapacity)));
            wrF32(header.data() + 44,rootSpacing);
        }
        for (int k = 0; k < 3; ++k)
            wrF64(header.data() + 48 + k * 8,hdr.min[k]);
        for (int k = 0; k < 3; ++k)
            wrF64(header.data() + 72 + k * 8,hdr.max[k]);
        wrU64(header.data() + 96,kHeaderSize);
        wrU64(header.data() + 104,nodeCount * kNodeEntrySize);
        wrU64(header.data() + 112,(kHeaderSize + nodeCount * kNodeEntrySize + 15) & ~uint64_t(15));
        wrU64(header.data() + 120,0);

        bool    ok  =   std::fwrite(header.data(),1,header.size(),file) == header.size();
        std::vector<uint8_t>    table(nodeCount * kNodeEntrySize,0);
        for (size_t i = 0; ok && i < entries.size(); ++i)
            writeNodeEntry(table.data() + i * kNodeEntrySize,entries[i]);
        if (ok)
            ok  =   std::fwrite(table.data(),1,table.size(),file) == table.size();

        /// 严格按顺序写节点数据，写入后立即释放点内存
        uint64_t    written =   0;
        for (size_t i = 0; ok && i < nodes.size(); ++i)
        {
            const   BuildNode&  node    =   nodes[i];
            const   NodeEntry&  e       =   entries[i];
            const   int64_t     pos     =   int64_t(e.dataOffset);
            if (_fseeki64(file,pos,SEEK_SET) != 0)
            {
                ok  =   false;
                break;
            }
            double  center[3],half[3];
            float   relMin[3],relMax[3];
            quantParams(node,rootCenter,center,half,relMin,relMax);
            double  inv[3]  =   {0.0,0.0,0.0};
            for (int k = 0; k < 3; ++k)
                inv[k]  =   half[k] > 0.0 ? 32767.0 / half[k] : 0.0;
            std::vector<uint32_t>   packed;
            packed.reserve(node.points.size() * 2);
            for (const auto& p : node.points)
            {
                double  w[3]    =   {double(p.x) * hdr.scale[0] + hdr.offset[0] - rootCenter[0],
                                     double(p.y) * hdr.scale[1] + hdr.offset[1] - rootCenter[1],
                                     double(p.z) * hdr.scale[2] + hdr.offset[2] - rootCenter[2]};
                int     q[3]    =   {0,0,0};
                for (int k = 0; k < 3; ++k)
                {
                    const   long    v   =   std::lround((w[k] - center[k]) * inv[k]);
                    q[k]    =   int(std::max(-32767L,std::min(32767L,v)));
                }
                packed.push_back(uint32_t(uint16_t(int16_t(q[0])))
                                 | (uint32_t(uint16_t(int16_t(q[1]))) << 16));
                packed.push_back(uint32_t(uint16_t(int16_t(q[2])))
                                 | (uint32_t(p.color) << 16));
            }
            if (!packed.empty())
                ok  =   std::fwrite(packed.data(),1,packed.size() * sizeof(uint32_t),file)
                        == packed.size() * sizeof(uint32_t);
            written +=  node.points.size();
            std::vector<LasPoint>().swap(nodes[i].points);
            std::printf("\r  writing PCO ... nodes=%zu/%zu",i + 1,nodes.size());
            std::fflush(stdout);
        }
        std::printf("\r  writing PCO ... done              \n");
        std::fclose(file);
        if (ok)
        {
            std::printf("  PCO bytes = %llu, points = %llu\n",
                        (unsigned long long)cursor,(unsigned long long)written);
        }
        return  ok;
    }

    // ==================== 命令行 ====================

    void    printUsage()
    {
        std::printf(
            "tool.las2pco - convert LAS point cloud to PCO octree file\n"
            "\n"
            "Usage:\n"
            "  tool.las2pco <input.las> [options]\n"
            "\n"
            "Options:\n"
            "  -o, --output <file>       output .pco path (default: <input>.pco)\n"
            "      --node-capacity <n>   max points per node (default: 50000)\n"
            "      --spacing <meters>    root voxel spacing (default: auto)\n"
            "      --max-depth <n>       max octree depth (default: 32)\n"
            "  -h, --help                show this help\n");
    }

    bool    parseU32(const char* text,uint32_t& value)
    {
        char*   end =   nullptr;
        const   unsigned long   v   =   std::strtoul(text,&end,10);
        if (end == text || *end != '\0' || v == 0)
            return  false;
        value   =   uint32_t(v);
        return  true;
    }

    bool    parseOptions(int argc,char** argv,Options& opt)
    {
        for (int i = 1; i < argc; ++i)
        {
            const   String  arg =   argv[i];
            if (arg == "-h" || arg == "--help")
            {
                opt.showHelp    =   true;
                printUsage();
                return  false;
            }
            if ((arg == "-o" || arg == "--output") && i + 1 < argc)
            {
                opt.output  =   argv[++i];
            }
            else if (arg == "--node-capacity" && i + 1 < argc)
            {
                if (!parseU32(argv[++i],opt.nodeCapacity) || opt.nodeCapacity > kMaxPointsCap)
                {
                    std::fprintf(stderr,"[ERROR] invalid --node-capacity\n");
                    return  false;
                }
            }
            else if (arg == "--max-depth" && i + 1 < argc)
            {
                if (!parseU32(argv[++i],opt.maxDepth) || opt.maxDepth > 32)
                {
                    std::fprintf(stderr,"[ERROR] invalid --max-depth (1..32)\n");
                    return  false;
                }
            }
            else if (arg == "--spacing" && i + 1 < argc)
            {
                char*   end =   nullptr;
                const   double  v   =   std::strtod(argv[++i],&end);
                if (end == argv[i] || *end != '\0' || v < 0.0)
                {
                    std::fprintf(stderr,"[ERROR] invalid --spacing\n");
                    return  false;
                }
                opt.spacing =   v;
            }
            else if (!arg.empty() && arg[0] == '-' && arg.size() > 1)
            {
                std::fprintf(stderr,"[ERROR] unknown option: %s\n",arg.c_str());
                printUsage();
                return  false;
            }
            else if (opt.input.empty())
            {
                opt.input   =   arg;
            }
            else
            {
                std::fprintf(stderr,"[ERROR] unexpected argument: %s\n",arg.c_str());
                return  false;
            }
        }
        if (opt.input.empty())
        {
            printUsage();
            return  false;
        }
        if (opt.output.empty())
        {
            std::filesystem::path   p(opt.input);
            p.replace_extension(".pco");
            opt.output  =   p.string();
        }
        return  true;
    }
}

int main(int argc,char** argv)
{
    Options opt;
    if (!parseOptions(argc,argv,opt))
        return  opt.showHelp ? 0 : 1;

    const   auto    t0  =   std::chrono::steady_clock::now();
    FILE*   file    =   std::fopen(opt.input.c_str(),"rb");
    if (file == nullptr)
    {
        std::fprintf(stderr,"[ERROR] cannot open input: %s\n",opt.input.c_str());
        return  2;
    }
    uint8_t headerBytes[375]    =   {0};
    const   size_t  readLen     =   std::fread(headerBytes,1,sizeof(headerBytes),file);
    LasHeader   hdr;
    if (readLen < 227 || !parseHeader(headerBytes,readLen,hdr))
    {
        std::fprintf(stderr,"[ERROR] invalid LAS header: %s\n",opt.input.c_str());
        std::fclose(file);
        return  3;
    }
    if (hdr.compressed)
    {
        std::fprintf(stderr,"[ERROR] LAZ(compressed) is not supported: %s\n",opt.input.c_str());
        std::fclose(file);
        return  4;
    }
    std::printf("input : %s\n",opt.input.c_str());
    std::printf("  LAS %u.%u, point format %u, record %u bytes\n",
                unsigned(hdr.versionMajor),unsigned(hdr.versionMinor),
                unsigned(hdr.pointFormat),unsigned(hdr.pointRecordLength));
    std::printf("  bounds: (%.3f, %.3f, %.3f) - (%.3f, %.3f, %.3f)\n",
                hdr.min[0],hdr.min[1],hdr.min[2],hdr.max[0],hdr.max[1],hdr.max[2]);

    std::vector<LasPoint>   points;
    if (!readPoints(file,hdr,points))
    {
        std::fprintf(stderr,"[ERROR] no point data: %s\n",opt.input.c_str());
        std::fclose(file);
        return  5;
    }
    std::fclose(file);
    std::printf("  points read: %zu\n",points.size());

    std::vector<BuildNode>  nodes;
    buildOctree(nodes,hdr,std::move(points),opt);

    if (!writePco(opt.output,nodes,hdr,opt))
        return  6;

    const   double  seconds =   std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    std::printf("output: %s\n",opt.output.c_str());
    std::printf("  nodes=%zu, elapsed=%.2fs\n",nodes.size(),seconds);
    return  0;
}
