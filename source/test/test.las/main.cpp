/// test.las
/// LAS 点云读取测试:
///   1. 创建引擎 + 场景(材质创建需要显卡资源)
///   2. 合成 LAS 样本(格式 0/2/3/6/7、LAS 1.2/1.3/1.4、VLR、extra bytes、
///      扩展点计数、压缩位、坏签名)
///   3. 校验 KD 盒树(叶子点数 <= 10240)、世界坐标复原、RGB565 颜色、共享材质
/// 用法: test.las.exe

#include    <cstdio>
#include    <cstring>
#include    <cmath>
#include    <chrono>
#include    <algorithm>
#include    <map>
#include    <set>
#include    <vector>
#include    <string>
#include    <filesystem>
#include    <fstream>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/FEBuffer.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/mesh/FEMesh.hpp"
#include    "../../inc/mesh/FEAttribyteBuffer.hpp"
#include    "../../inc/node/FENode.hpp"
#include    "../../inc/material/FEMaterialPoint565.hpp"
#include    "../../inc/fileFormat/las/FEFormatLAS.hpp"
#include    "../../inc/fileFormat/las/FEFormatLASReader.hpp"

using   namespace   FE;

/// 注册渲染后端插件,材质创建需要显卡资源
USING_PLUGIN(rs_vulkan);

namespace
{
    int     g_pass  =   0;
    int     g_fail  =   0;

    void    check(bool ok,const char* label)
    {
        if (ok)     {   ++  g_pass;     printf("[PASS] %s\n",label);    }
        else        {   ++  g_fail;     printf("[FAIL] %s\n",label);    }
        fflush(stdout);
    }

    constexpr   uint32  kMaxPointsPerBox    =   10240;

    /// ==================== 合成 LAS 写入 ====================
    struct  SrcPoint
    {
        int32   x   =   0;
        int32   y   =   0;
        int32   z   =   0;
        uint16  intensity   =   0;
        uint8   cls         =   0;
        uint8   r           =   0;
        uint8   g           =   0;
        uint8   b           =   0;
    };

    struct  LasDef
    {
        uint8   major   =   1;
        uint8   minor   =   2;
        uint8   pointFormat =   0;
        uint16  recordLength    =   0;
        uint32  vlrCount        =   0;
        double  scale[3]        =   {0.01,0.01,0.01};
        double  offset[3]       =   {0.0,0.0,0.0};
        bool    legacyZero      =   false;
        bool    compressed      =   false;
        std::vector<SrcPoint>   points;
    };

    inline  void    wrU16(uint8* p,uint16 v)    {   std::memcpy(p,&v,2);    }
    inline  void    wrU32(uint8* p,uint32 v)    {   std::memcpy(p,&v,4);    }
    inline  void    wrI32(uint8* p,int32 v)     {   std::memcpy(p,&v,4);    }
    inline  void    wrU64(uint8* p,uint64 v)    {   std::memcpy(p,&v,8);    }
    inline  void    wrF64(uint8* p,double v)    {   std::memcpy(p,&v,8);    }

    static  uint16  standardLength(uint8 fmt)
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

    static  uint16  rgbOffsetOf(uint8 fmt)
    {
        switch (fmt)
        {
        case 2:     return  20;
        case 3:
        case 5:     return  28;
        case 7:
        case 8:
        case 10:    return  30;
        default:    return  0xFFFF;
        }
    }

    static  uint32  pack565(uint8 r,uint8 g,uint8 b)
    {
        return  uint32((uint32(r >> 3) << 11) | (uint32(g >> 2) << 5) | uint32(b >> 3));
    }

    /// 与读取器一致的 ASPRS 分类色(仅测试用到的类别)
    static  uint32  classColor6()
    {
        return  pack565(220,80,60);
    }

    static  std::vector<uint8>  buildLas(const LasDef& def)
    {
        const   uint16  headerSize  =   def.minor >= 4 ? 375 : (def.minor >= 3 ? 235 : 227);
        const   uint16  recLen      =   def.recordLength != 0
                                        ? def.recordLength
                                        : standardLength(def.pointFormat);
        const   uint32  offset      =   uint32(headerSize) + 54u * def.vlrCount;
        std::vector<uint8>  buf(size_t(offset) + def.points.size() * recLen,0);
        uint8*  h   =   buf.data();
        std::memcpy(h,"LASF",4);
        h[24]   =   def.major;
        h[25]   =   def.minor;
        wrU16(h + 94,headerSize);
        wrU32(h + 96,offset);
        wrU32(h + 100,def.vlrCount);
        h[104]  =   uint8(def.pointFormat | (def.compressed ? 0x80 : 0x00));
        wrU16(h + 105,recLen);
        if (!def.legacyZero)
        {
            wrU32(h + 107,uint32(def.points.size()));
            wrU32(h + 111,uint32(def.points.size()));
        }
        double  mn[3]   =   {1e300,1e300,1e300};
        double  mx[3]   =   {-1e300,-1e300,-1e300};
        for (auto& p : def.points)
        {
            const   int32   raw[3]  =   {p.x,p.y,p.z};
            for (int k = 0; k < 3; ++k)
            {
                const   double  w   =   double(raw[k]) * def.scale[k] + def.offset[k];
                mn[k]   =   (std::min)(mn[k],w);
                mx[k]   =   (std::max)(mx[k],w);
            }
        }
        for (int k = 0; k < 3; ++k)
        {
            wrF64(h + 131 + k * 8,def.scale[k]);
            wrF64(h + 155 + k * 8,def.offset[k]);
        }
        wrF64(h + 179,mx[0]);
        wrF64(h + 187,mn[0]);
        wrF64(h + 195,mx[1]);
        wrF64(h + 203,mn[1]);
        wrF64(h + 211,mx[2]);
        wrF64(h + 219,mn[2]);
        if (def.minor >= 4)
            wrU64(h + 247,uint64(def.points.size()));

        const   uint16  rgbOff  =   rgbOffsetOf(def.pointFormat);
        for (size_t i = 0; i < def.points.size(); ++i)
        {
            const   SrcPoint&   p   =   def.points[i];
            uint8*  rec =   buf.data() + offset + i * recLen;
            wrI32(rec     ,p.x);
            wrI32(rec + 4 ,p.y);
            wrI32(rec + 8 ,p.z);
            wrU16(rec + 12,p.intensity);
            if (def.pointFormat <= 5)
                rec[15] =   p.cls;
            else
                rec[16] =   p.cls;
            if (rgbOff != 0xFFFF)
            {
                wrU16(rec + rgbOff    ,uint16(p.r) * 257u);
                wrU16(rec + rgbOff + 2,uint16(p.g) * 257u);
                wrU16(rec + rgbOff + 4,uint16(p.b) * 257u);
            }
        }
        return  buf;
    }

    static  String  writeTemp(const String& name,const std::vector<uint8>& bytes)
    {
        const   String  path    =   (std::filesystem::temp_directory_path() / name).string();
        std::ofstream   out(path,std::ios::binary);
        if (!bytes.empty())
            out.write((const char*)bytes.data(),std::streamsize(bytes.size()));
        out.close();
        return  path;
    }

    /// ==================== 场景遍历统计 ====================
    struct  Stat
    {
        size_t                  nodes   =   0;
        size_t                  meshes  =   0;
        size_t                  leaves  =   0;
        size_t                  points  =   0;
        size_t                  maxLeaf =   0;
        size_t                  badAttr =   0;
        size_t                  badPrim =   0;
        std::set<FEMaterial*>   materials;
        std::map<uint32,int>    colors;
        double                  mn[3]   =   {1e300,1e300,1e300};
        double                  mx[3]   =   {-1e300,-1e300,-1e300};
        double                  sum[3]  =   {0.0,0.0,0.0};
        double                  sum2[3] =   {0.0,0.0,0.0};
    };

    void    collect(const Node& node,const real3& parent,Stat& stat)
    {
        if (node == nullptr)
            return;
        ++  stat.nodes;
        const   real3   local   =   node->localTranslation();
        const   real3   world   =   parent + local;
        Mesh    mesh            =   node->mesh();
        if (mesh != nullptr)
        {
            ++  stat.meshes;
            auto*   pos =   mesh->get(FEAttribute(IS_VERTEX_POS));
            auto*   clr =   mesh->get(FEAttribute(IS_VERTEX_COLOR0));
            if (pos == nullptr || pos->count() == 0
                || pos->attr().format() != FMT_R16G16B16_SNORM
                || clr == nullptr || clr->attr().format() != FMT_R16_UINT
                || clr->count() != pos->count())
            {
                ++  stat.badAttr;
            }
            else
            {
                ++  stat.leaves;
                stat.points +=  pos->count();
                stat.maxLeaf =  (std::max)(stat.maxLeaf,size_t(pos->count()));
                /// 位置为 SNORM 归一化坐标(int16/32767)，乘节点缩放还原世界单位；
                /// 颜色独立存在 R16_UINT 属性里(RGB565)
                const   int16*  data    =   (const int16*)pos->data();
                const   uint16* cdata   =   (const uint16*)clr->data();
                const   real3   scl     =   node->localScaling();
                for (uint i = 0; i < pos->count(); ++i)
                {
                    const   double  p[3]    =   {double(data[i * 3 + 0]) / 32767.0 * scl.x + world.x,
                                                 double(data[i * 3 + 1]) / 32767.0 * scl.y + world.y,
                                                 double(data[i * 3 + 2]) / 32767.0 * scl.z + world.z};
                    for (int k = 0; k < 3; ++k)
                    {
                        stat.mn[k]  =   (std::min)(stat.mn[k],p[k]);
                        stat.mx[k]  =   (std::max)(stat.mx[k],p[k]);
                        stat.sum[k] +=  p[k];
                        stat.sum2[k]+=  p[k] * p[k];
                    }
                    ++  stat.colors[uint32(cdata[i])];
                }
            }
            if (mesh->primitives().empty()
                || mesh->primitives().front()->primitive() != PRI_POINTS)
            {
                ++  stat.badPrim;
            }
            if (node->material() != nullptr)
                stat.materials.insert(node->material().get());
        }
        for (const auto& child : node->children())
            collect(child,world,stat);
    }

    /// ==================== 期望值计算 ====================
    void    expectedBounds(const std::vector<SrcPoint>& pts,const LasDef& def,
                           double mn[3],double mx[3],double sum[3],double sum2[3])
    {
        for (int k = 0; k < 3; ++k)
        {
            mn[k]   =   1e300;
            mx[k]   =   -1e300;
            sum[k]  =   0.0;
            sum2[k] =   0.0;
        }
        for (auto& p : pts)
        {
            const   int32   raw[3]  =   {p.x,p.y,p.z};
            for (int k = 0; k < 3; ++k)
            {
                const   double  w   =   double(raw[k]) * def.scale[k] + def.offset[k];
                mn[k]   =   (std::min)(mn[k],w);
                mx[k]   =   (std::max)(mx[k],w);
                sum[k]  +=  w;
                sum2[k] +=  w * w;
            }
        }
    }

    bool    approx(double a,double b,double eps)
    {
        return  std::fabs(a - b) <= eps * (1.0 + std::fabs(b));
    }

    /// ==================== 各测试 ====================
    void    testFormat0(FEContext& ctx)
    {
        printf("--- format 0 / LAS 1.2 / KD boxes ---\n");
        LasDef  def;
        def.minor   =   2;
        def.pointFormat =   0;
        def.scale[0]    =   0.01;
        def.scale[1]    =   0.01;
        def.scale[2]    =   0.01;
        def.offset[0]   =   500000.0;
        def.offset[1]   =   4200000.0;
        def.offset[2]   =   100.0;
        const   size_t  count   =   25000;
        def.points.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i * 2) - 1000;
            p.y =   int32((i * 37) % 5000) - 2500;
            p.z =   int32((i * 11) % 2000) - 1000;
            p.intensity =   uint16((i * 257) % 65536);
            p.cls       =   (i % 10 == 0) ? 6 : 1;
            def.points.push_back(p);
        }
        const   String  path    =   writeTemp("las_test_0.las",buildLas(def));

        FEFileFormat    fmt(".las","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        check(reader != nullptr,"format0: query .las reader");
        if (reader == nullptr)
            return;
        Objects objects =   reader->readFiles({path});
        check(objects.size() == 1,"format0: readFiles object count = 1");
        check(reader->progress() == real(1),"format0: progress reaches 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"format0: root is node");
        if (root == nullptr)
            return;

        Stat    stat;
        collect(root,real3(0.0),stat);
        printf("       nodes=%zu meshes=%zu points=%zu maxLeaf=%zu materials=%zu\n",
               stat.nodes,stat.meshes,stat.points,stat.maxLeaf,stat.materials.size());
        check(std::strcmp(root->name(),"las_test_0") == 0,"format0: root name");
        check(stat.nodes == 7,"format0: node count = 7 (1+2+4)");
        check(stat.meshes == 4,"format0: leaf mesh count = 4");
        check(stat.leaves == 4,"format0: leaf count = 4");
        check(stat.points == count,"format0: total points = 25000");
        check(stat.maxLeaf <= kMaxPointsPerBox,"format0: leaf points <= 10240");
        check(stat.badAttr == 0,"format0: pos/RGB565 attributes valid");
        check(stat.badPrim == 0,"format0: primitives are PRI_POINTS");
        check(stat.materials.size() == 1,"format0: one shared material");
        if (stat.materials.size() == 1)
        {
            FEMaterial* mat =   *stat.materials.begin();
            check(mat->cast<FEMaterialPoint565>() != nullptr,"format0: material is MaterialPoint565");
        }

        /// 世界坐标:包围盒/均值/二阶矩都应与源点一致
        double  mn[3],mx[3],sum[3],sum2[3];
        expectedBounds(def.points,def,mn,mx,sum,sum2);
        bool    coordOk =   true;
        for (int k = 0; k < 3; ++k)
        {
            if (!approx(stat.mn[k],mn[k],1e-5) || !approx(stat.mx[k],mx[k],1e-5)
                || !approx(stat.sum[k],sum[k],1e-5) || !approx(stat.sum2[k],sum2[k],1e-5))
            {
                coordOk =   false;
            }
        }
        check(coordOk,"format0: world coordinates restored");
        check(approx(root->localTranslation().x,(mn[0] + mx[0]) * 0.5,1e-9)
           && approx(root->localTranslation().y,(mn[1] + mx[1]) * 0.5,1e-9)
           && approx(root->localTranslation().z,(mn[2] + mx[2]) * 0.5,1e-9),
              "format0: root translation = box center");

        /// 颜色:分类 6 使用分类色,其余走强度灰度
        std::set<uint32>    grays;
        int                 redExpected =   0;
        for (auto& p : def.points)
        {
            if (p.cls == 6)
            {
                ++  redExpected;
            }
            else
            {
                const   uint8   gray    =   uint8(p.intensity >> 8);
                grays.insert(pack565(gray,gray,gray));
            }
        }
        int     redActual   =   0;
        bool    colorOk     =   true;
        for (auto& kv : stat.colors)
        {
            if (kv.first == classColor6())
                redActual   +=  kv.second;
            else if (grays.find(kv.first) == grays.end())
                colorOk =   false;
        }
        check(colorOk,"format0: colors are classification/gray only");
        check(redActual == redExpected,"format0: classification color count");

        /// 拾取:顶点是量化整数、节点带缩放，世界单位容差必须换算到局部空间
        root->updateTransform(real(0),true);
        root->updateAabb(true);
        /// 取 Z 最高的点作为目标:拾取按"射线上最近的候选点"取(与深度一致),
        /// 最高点沿向下的射线不会被其它点抢在前头
        const   SrcPoint*   sp  =   &def.points.front();
        for (auto& spItem : def.points)
        {
            if (spItem.z > sp->z)
                sp  =   &spItem;
        }
        const   real3   target  =   real3(real(double(sp->x) * def.scale[0] + def.offset[0]),
                                          real(double(sp->y) * def.scale[1] + def.offset[1]),
                                          real(double(sp->z) * def.scale[2] + def.offset[2]));
        const   Ray     rayPick =   Ray(target + real3(0,0,10),real3(0,0,-1));
        Pickups picks;
        root->intersect(rayPick,picks);
        bool    hitTarget   =   false;
        for (auto& pick : picks)
        {
            if (pick.pickup == FEPickup::PR_Point
                && approx(double(pick.point.x),double(target.x),1e-5)
                && approx(double(pick.point.y),double(target.y),1e-5)
                && approx(double(pick.point.z),double(target.z),1e-5))
            {
                hitTarget   =   true;
                break;
            }
        }
        check(hitTarget,"format0: quantized vertex pickup hits target point");

        std::filesystem::remove(path);
    }

    void    testBufferAndUpper(FEContext& ctx)
    {
        printf("--- .LAS uppercase / readBuffers ---\n");
        LasDef  def;
        def.pointFormat =   2;
        def.scale[0]    =   0.001;
        def.scale[1]    =   0.001;
        def.scale[2]    =   0.001;
        for (size_t i = 0; i < 5000; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i);
            p.y =   int32(i * 3);
            p.z =   int32(i * 7);
            p.r =   uint8(i % 251) + 1;
            p.g =   uint8((i * 13) % 241) + 1;
            p.b =   uint8((i * 29) % 239) + 1;
            def.points.push_back(p);
        }
        const   std::vector<uint8>  bytes   =   buildLas(def);
        const   String  path    =   writeTemp("las_test_2.las",bytes);

        FEFileFormat    fmt(".LAS","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        check(reader != nullptr,"buffer: query .LAS reader (uppercase)");
        if (reader == nullptr)
            return;

        Buffer  buffer  =   new FEBuffer(ctx);
        buffer->cInfo()._buffer =   bytes;
        Objects objects =   reader->readBuffers({buffer});
        check(objects.size() == 1,"buffer: readBuffers object count = 1");
        check(reader->progress() == real(1),"buffer: progress reaches 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"buffer: root is node");
        if (root != nullptr)
        {
            Stat    stat;
            collect(root,real3(0.0),stat);
            check(stat.nodes == 1 && stat.meshes == 1,"buffer: 5000 points -> 1 box");
            check(stat.points == 5000,"buffer: total points = 5000");
            check(stat.maxLeaf <= kMaxPointsPerBox,"buffer: leaf points <= 10240");

            std::map<uint32,int>    expected;
            for (auto& p : def.points)
                ++  expected[pack565(p.r,p.g,p.b)];
            check(stat.colors == expected,"buffer: RGB565 colors packed correctly");
        }
        std::filesystem::remove(path);
    }

    void    testFormat3Extras(FEContext& ctx)
    {
        printf("--- format 3 / LAS 1.3 / VLR + extra bytes ---\n");
        LasDef  def;
        def.minor       =   3;
        def.pointFormat =   3;
        def.recordLength=   34 + 6;     // 标准长度 + 6 字节 extra bytes
        def.vlrCount    =   1;
        for (size_t i = 0; i < 6000; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i * 5) - 3000;
            p.y =   int32(i % 97);
            p.z =   int32(i % 53);
            p.r =   uint8(i % 200) + 20;
            p.g =   uint8((i * 3) % 200) + 20;
            p.b =   uint8((i * 5) % 200) + 20;
            def.points.push_back(p);
        }
        const   String  path    =   writeTemp("las_test_3.las",buildLas(def));

        FEFileFormat    fmt(".las","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        if (reader == nullptr)
        {
            check(false,"format3: query .las reader");
            return;
        }
        Objects objects =   reader->readFiles({path});
        check(objects.size() == 1,"format3: readFiles object count = 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"format3: root is node");
        if (root != nullptr)
        {
            Stat    stat;
            collect(root,real3(0.0),stat);
            check(stat.nodes == 1 && stat.points == 6000,"format3: extra bytes skipped, 6000 points");
            std::map<uint32,int>    expected;
            for (auto& p : def.points)
                ++  expected[pack565(p.r,p.g,p.b)];
            check(stat.colors == expected,"format3: RGB565 colors from offset 20");
        }
        std::filesystem::remove(path);
    }

    void    testFormat6Las14(FEContext& ctx)
    {
        printf("--- format 6 / LAS 1.4 / extended count only ---\n");
        LasDef  def;
        def.minor       =   4;
        def.pointFormat =   6;
        def.legacyZero  =   true;
        def.scale[0]    =   0.001;
        def.scale[1]    =   0.001;
        def.scale[2]    =   0.001;
        for (size_t i = 0; i < 12000; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i);
            p.y =   int32(i % 500);
            p.z =   int32(i % 300);
            p.cls   =   6;
            def.points.push_back(p);
        }
        const   String  path    =   writeTemp("las_test_6.las",buildLas(def));

        FEFileFormat    fmt(".las","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        if (reader == nullptr)
        {
            check(false,"format6: query .las reader");
            return;
        }
        Objects objects =   reader->readFiles({path});
        check(objects.size() == 1,"format6: readFiles object count = 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"format6: root is node");
        if (root != nullptr)
        {
            Stat    stat;
            collect(root,real3(0.0),stat);
            check(stat.nodes == 3 && stat.leaves == 2,"format6: 12000 points -> 2 leaves");
            check(stat.points == 12000,"format6: extended point count = 12000");
            check(stat.maxLeaf <= kMaxPointsPerBox,"format6: leaf points <= 10240");
            const   int red =   stat.colors.size() == 1
                                ? stat.colors.begin()->second : 0;
            check(red == 12000 && stat.colors.begin()->first == classColor6(),
                  "format6: classification 6 color for all points");
        }
        std::filesystem::remove(path);
    }

    void    testFormat7(FEContext& ctx)
    {
        printf("--- format 7 / RGB at offset 30 ---\n");
        LasDef  def;
        def.pointFormat =   7;
        for (size_t i = 0; i < 3000; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i);
            p.y =   int32(i % 31);
            p.z =   int32(i % 17);
            p.r =   uint8(i % 256);
            p.g =   uint8((i * 7) % 256);
            p.b =   uint8((i * 11) % 256);
            p.cls   =   2;
            def.points.push_back(p);
        }
        const   String  path    =   writeTemp("las_test_7.las",buildLas(def));

        FEFileFormat    fmt(".las","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        if (reader == nullptr)
        {
            check(false,"format7: query .las reader");
            return;
        }
        Objects objects =   reader->readFiles({path});
        check(objects.size() == 1,"format7: readFiles object count = 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"format7: root is node");
        if (root != nullptr)
        {
            Stat    stat;
            collect(root,real3(0.0),stat);
            check(stat.nodes == 1 && stat.points == 3000,"format7: 3000 points -> 1 box");
            std::map<uint32,int>    expected;
            for (auto& p : def.points)
            {
                /// RGB 全 0 时回退到分类色(测试数据分类为 2)
                if ((p.r | p.g | p.b) == 0)
                    ++  expected[pack565(140,100,60)];
                else
                    ++  expected[pack565(p.r,p.g,p.b)];
            }
            check(stat.colors == expected,"format7: RGB565 colors from offset 30");
        }
        std::filesystem::remove(path);
    }

    void    testRejects(FEContext& ctx)
    {
        printf("--- compressed / broken files ---\n");
        FEFileFormat    fmt(".las","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        check(reader != nullptr,"reject: query .las reader");
        if (reader == nullptr)
            return;

        LasDef  def;
        def.pointFormat =   2;
        def.recordLength=   26;
        def.compressed  =   true;
        for (size_t i = 0; i < 100; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i);
            def.points.push_back(p);
        }
        const   String  pathLaz =   writeTemp("las_test_laz.las",buildLas(def));
        Objects laz =   reader->readFiles({pathLaz});
        check(laz.empty(),"reject: compressed(LAZ) returns no object");
        std::filesystem::remove(pathLaz);

        LasDef  good;
        good.pointFormat    =   0;
        for (size_t i = 0; i < 10; ++i)
        {
            SrcPoint    p;
            p.x =   int32(i);
            good.points.push_back(p);
        }
        std::vector<uint8>  bad =   buildLas(good);
        bad[0]  =   'X';
        const   String  pathBad =   writeTemp("las_test_bad.las",bad);
        Objects broken  =   reader->readFiles({pathBad});
        check(broken.empty(),"reject: bad signature returns no object");
        std::filesystem::remove(pathBad);
    }

    /// 真实 LAS 文件:读取并输出规模/包围盒/耗时，校验盒树与属性完整性
    void    testRealFile(FEContext& ctx,const String& file)
    {
        printf("--- real LAS: %s ---\n",file.c_str());
        FEFileFormat    fmt(".las","1.0.0.0","LAS Point Cloud!");
        auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
        check(reader != nullptr,"real: query .las reader");
        if (reader == nullptr)
            return;

        const   auto    begin   =   std::chrono::steady_clock::now();
        Objects objects =   reader->readFiles({file});
        const   auto    end     =   std::chrono::steady_clock::now();
        const   double  seconds =   std::chrono::duration<double>(end - begin).count();

        check(objects.size() == 1,"real: readFiles object count = 1");
        check(reader->progress() == real(1),"real: progress reaches 1");
        Node    root    =   objects.size() == 1 ? objects[0]->cast<FENode>() : nullptr;
        check(root != nullptr,"real: root is node");
        if (root == nullptr)
            return;

        Stat    stat;
        collect(root,real3(0.0),stat);
        printf("       nodes=%zu leaves=%zu points=%zu maxLeaf=%zu materials=%zu\n",
               stat.nodes,stat.leaves,stat.points,stat.maxLeaf,stat.materials.size());
        printf("       bbox = (%.3f,%.3f,%.3f) - (%.3f,%.3f,%.3f)\n",
               stat.mn[0],stat.mn[1],stat.mn[2],stat.mx[0],stat.mx[1],stat.mx[2]);
        printf("       read+parse+mesh: %.2f s\n",seconds);

        check(stat.points > 0,"real: points > 0");
        check(stat.leaves == stat.meshes,"real: every leaf has a mesh");
        check(stat.maxLeaf <= kMaxPointsPerBox,"real: leaf points <= 10240");
        check(stat.badAttr == 0,"real: pos/RGB565 attributes valid");
        check(stat.badPrim == 0,"real: primitives are PRI_POINTS");
        check(stat.materials.size() == 1,"real: one shared material");
        check(stat.mx[0] > stat.mn[0] && stat.mx[1] > stat.mn[1] && stat.mx[2] > stat.mn[2],
              "real: bbox has positive size");
    }
};

int     main(int argc,char** argv)
{
    setvbuf(stdout,nullptr,_IONBF,0);

    FEContext&  ctx =   FEContext::instance();
    FEScene*    pScene  =   nullptr;

    FEApp::CreateInfo   info    =   {};
    info._width     =   640;
    info._height    =   480;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx,info);
    check(app != nullptr,"engine: create app");
    if (app == nullptr)
        return  1;

    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    check(scene->setup(app,RS_VULKAN),"engine: scene setup");

    /// 带参数时读取真实 LAS 文件，否则运行合成样本测试
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
            testRealFile(ctx,argv[i]);
    }
    else
    {
        testFormat0(ctx);
        testBufferAndUpper(ctx);
        testFormat3Extras(ctx);
        testFormat6Las14(ctx);
        testFormat7(ctx);
        testRejects(ctx);
    }

    scene->destroy();
    scene   =   nullptr;
    pScene  =   nullptr;
    app->destroy();

    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);
    return  g_fail == 0 ? 0 : 1;
}
