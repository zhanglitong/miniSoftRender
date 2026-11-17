#pragma     once
#include    "../../FEStreamReader.hpp"
#include    "../../FEReaderHelper.hpp"
#include    "../../node/FENode.hpp"
#include    "../../mesh/FEMeshBuilder.hpp"
#include    "../../material/FEMaterialPBR.hpp"
#include    "../../geometry/FEGeometryBox.hpp"
#include    "../../geometry/FEGeometryCylinder.hpp"
#include    "../../geometry/FEGeometrySphere.hpp"
#include    "../../geometry/FEGeometryTorus.hpp"
#include    "../../geometry/FEGeometryPRPY.hpp"
#include    "../../geometry/FEGeometryPRRT.hpp"
#include    "../../geometry/FEGeometryPRCN.hpp"
#include    "../../geometry/FEGeometryPRCP.hpp"
#include    "../../geometry/FEGeometryPRDH.hpp"
#include    "../../geometry/detail/FEEarcut.hpp"
#include    "FEFormatRVM.hpp"
#include    <map>
#include    <array>
#include    <cmath>
#include    <cstring>
#include    <algorithm>

namespace   FE
{
    DEFINE_CLASS_UUID(FEFormatRVMReader,"{7D0D3A9B-4C21-4F5E-9B7A-2E6C1F0A5D33}");

    /// <summary>
    /// AVEVA RVM(PDMS/Review)模型读取器。
    /// RVM 采用大端序存储,矩阵为 3x4(x/y/z 轴 + 原点),位置单位为米,
    /// 基本体尺寸参数单位为毫米;引擎为 Z-up,因此各基本体按 Y-up 建模后
    /// 在节点旋转上左乘 Y-up -> Z-up 的 -90 度 X 轴旋转。
    /// </summary>
    class   FEFormatRVMReader :public FEStreamReader
    {
    public:
        using   MaterialIndex   =   std::map<uint32,Material>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEFormatRVMReader)
    public:
        FEFormatRVMReader(FEContext& ctx)
            :FEStreamReader(ctx)
        {}
        FEFormatRVMReader(const FEFormatRVMReader& other)
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
        /// 从文件加载 RVM 模型
        /// </summary>
        virtual Objects readFiles(const Strings& files) override
        {
            Objects result;
            for (auto& file : files)
            {
                FEReaderFile    reader(_ctx,file.c_str());
                if (!reader.isValid())
                {
                    LOG_ERR("RVM: open file failed: %s",file.c_str());
                    continue;
                }
                FEReaderHelper  helper(reader);
                Buffer          buffer  =   helper.read(_ctx);
                reader.close();
                auto    temp    =   readBuffers({buffer});
                result.insert(result.end(),temp.begin(),temp.end());
            }
            setProgress(real(1));
            return  result;
        }
        /// <summary>
        /// 从内存加载 RVM 模型
        /// </summary>
        virtual Objects readBuffers(const Buffers& buffers) override
        {
            Objects result;
            for (auto& buffer : buffers)
            {
                if (buffer == nullptr || buffer->length() == 0)
                    continue;
                Node    root    =   parse(buffer->data(),buffer->length());
                if (root != nullptr)
                    result.emplace_back(root);
            }
            /// 解析完成后立即释放内部缓存引用:读取器可能比场景/设备存活更久
            /// (如 FEViewer 的加载任务持有读取器),不能让它拖延节点与材质的释放
            _root   =   nullptr;
            _materials.clear();
            setProgress(real(1));
            return  result;
        }
    public:
        static  Formats formatList()
        {
            return  FERVMFomat::formats(UUIDOF(FEFormatRVMReader));
        }
    protected:
        /// <summary>
        /// 大端字节读取游标
        /// </summary>
        class   Cursor
        {
        public:
            Cursor(const uint8* data,uint64 size)
                :_data(data)
                ,_size(size)
            {}
            inline  bool    ok() const
            {
                return  _ok;
            }
            inline  uint64  pos() const
            {
                return  _pos;
            }
            inline  uint64  size() const
            {
                return  _size;
            }
            inline  void    fail()
            {
                _ok =   false;
            }
            inline  bool    need(uint64 n)
            {
                if (!_ok || n > _size || _pos > _size - n)
                {
                    _ok =   false;
                    return  false;
                }
                return  true;
            }
            inline  uint32  u32()
            {
                if (!need(4))
                    return  0;
                const   uint8*  p   =   _data + _pos;
                _pos    +=  4;
                return  (uint32(p[0]) << 24) | (uint32(p[1]) << 16) | (uint32(p[2]) << 8) | uint32(p[3]);
            }
            inline  float   f32()
            {
                uint32  v   =   u32();
                float   f   =   0.0f;
                std::memcpy(&f,&v,sizeof(f));
                return  f;
            }
            inline  bool    skip(uint64 n)
            {
                if (!need(n))
                    return  false;
                _pos    +=  n;
                return  true;
            }
            inline  bool    seek(uint64 n)
            {
                if (!_ok || n > _size)
                {
                    _ok =   false;
                    return  false;
                }
                _pos    =   n;
                return  true;
            }
            /// <summary>
            /// 读取标识:普通 4 字符;END 只有 3 字符且不消费后续的版本字段
            /// </summary>
            inline  bool    identifier(char out[5])
            {
                out[0]  =   out[1]  =   out[2]  =   out[3]  =   0;
                for (int i = 0; i < 3; ++i)
                {
                    uint32  v   =   u32();
                    if (!_ok)
                        return  false;
                    out[i]  =   char(v & 0xFF);
                }
                if (out[0] == 'E' && out[1] == 'N' && out[2] == 'D')
                    return  true;
                uint32  v   =   u32();
                if (!_ok)
                    return  false;
                out[3]  =   char(v & 0xFF);
                return  true;
            }
            /// <summary>
            /// 读取字符串:uint32 为 4 字节字数量,字符以 0 补齐到 4 字节边界
            /// </summary>
            inline  String  string()
            {
                uint32  words   =   u32();
                uint64  bytes   =   uint64(words) * 4;
                if (!need(bytes))
                    return  {};
                const   uint8*  p   =   _data + _pos;
                _pos    +=  bytes;
                uint64  len     =   0;
                while (len < bytes && p[len] != 0)
                    ++  len;
                return  String((const char*)p,size_t(len));
            }
            /// <summary>
            /// 跳过未知块:块首 uint32 为下一块的绝对偏移
            /// </summary>
            inline  bool    readOther()
            {
                uint32  next    =   u32();
                if (!_ok || !skip(8))
                    return  false;
                if (next == 0 || next <= _pos)
                    return  false;
                return  seek(next);
            }
        protected:
            const   uint8*  _data   =   nullptr;
            uint64          _size   =   0;
            uint64          _pos    =   0;
            bool            _ok     =   true;
        };
        /// <summary>
        /// 节点变换(分解后的 TRS)
        /// </summary>
        struct  PrimXform
        {
            real3   translation =   real3(0.0);
            real3   scale       =   real3(1.0);
            quatr   rotation    =   quatr(1.0,0.0,0.0,0.0);
        };
        /// <summary>
        /// 组上下文:节点延迟创建,空组不生成节点
        /// </summary>
        struct  GroupCtx
        {
            Node    node        =   nullptr;
            String  name;
            float4  color       =   float4(0.8f,0.8f,0.8f,1.0f);
            size_t  primCount   =   0;
        };
        using   FacetVertex =   std::pair<float3,float3>;
        using   FacetContour=   std::vector<FacetVertex>;
        using   FacetPolygon=   std::vector<FacetContour>;
    protected:
        MaterialIndex   _materials;
        uint8           _palette[256 * 3]   =   {0};
        /// <summary>
        /// 当前解析的 RVM 字节数,用于按游标位置上报加载进度
        /// </summary>
        uint64          _totalSize          =   0;
    protected:
        /// <summary>
        /// 按已消费字节数上报进度(进度条订阅读取器的 progress)
        /// </summary>
        inline  void    reportProgress(const Cursor& cur)
        {
            setProgress(real(cur.pos()) / real(_totalSize > 0 ? _totalSize : 1));
        }
        /// <summary>
        /// 重置本次加载状态并解析一个 RVM
        /// </summary>
        Node    parse(const uint8* data,uint64 size)
        {
            Cursor  cur(data,size);
            _totalSize  =   size;
            setProgress(real(0));
            resetPalette();
            _materials.clear();
            _stack.clear();

            /// HEAD 头:魔数 + 5 个字符串(第 5 个为编码,可选)
            char    id[5]   =   {0};
            if (!cur.identifier(id) || std::strcmp(id,"HEAD") != 0)
            {
                LOG_ERR("RVM: invalid header(only binary rvm is supported)");
                return  nullptr;
            }
            const   uint64  headNext    =   cur.u32();
            cur.u32();
            cur.u32();
            cur.string();
            cur.string();
            cur.string();
            cur.string();
            if (cur.pos() < headNext)
                cur.string();
            if (cur.pos() < headNext)
                cur.seek(headNext);

            /// MODL 块
            if (!cur.identifier(id) || std::strcmp(id,"MODL") != 0)
            {
                LOG_ERR("RVM: MODL chunk not found");
                return  nullptr;
            }
            cur.u32();
            cur.u32();
            cur.u32();
            const   String  projectName =   cur.string();
            cur.string();
            if (!cur.ok())
                return  nullptr;

            Node    root    =   new FENode(_ctx);
            root->setName(projectName);
            _root           =   root;

            /// 模型上下文入栈:顶层基本体直接挂到根节点
            GroupCtx    modelCtx;
            modelCtx.node   =   root;
            modelCtx.name   =   projectName;
            _stack.emplace_back(modelCtx);

            while (cur.ok())
            {
                if (isCanceled())
                    break;
                if (!cur.identifier(id))
                    break;
                const   String  chunk(id);
                if (chunk == "END")
                    break;
                if (chunk == "CNTB")
                {
                    if (!parseGroup(cur))
                        break;
                }
                else if (chunk == "PRIM")
                {
                    if (!parsePrimitive(cur))
                        break;
                }
                else if (chunk == "COLR")
                {
                    if (!parseColor(cur))
                        break;
                }
                else if (!cur.readOther())
                {
                    break;
                }
                reportProgress(cur);
            }
            _stack.clear();
            if (!cur.ok())
                LOG_WAR("RVM: parse truncated at %llu/%llu",(unsigned long long)cur.pos(),(unsigned long long)size);
            return  root;
        }
        /// <summary>
        /// 解析 CNTB 组,递归读取 PRIM/CNTB/COLR 直到 CNTE
        /// </summary>
        bool    parseGroup(Cursor& cur)
        {
            const   uint32  next    =   cur.u32();
            cur.u32();
            cur.u32();
            GroupCtx    ctx;
            ctx.name    =   cur.string();
            for (uint i = 0; i < 3; ++i)
                cur.f32();
            const   uint32  colorIndex  =   cur.u32();
            uint32          alpha       =   255;
            if (cur.pos() < next && cur.ok())
            {
                const   uint32  value   =   cur.u32();
                alpha   =   (value >> 24) & 0xFF;
            }
            ctx.color   =   paletteColor(colorIndex,uint8(alpha));
            _stack.emplace_back(ctx);

            bool    closed  =   false;
            while (cur.ok())
            {
                char    id[5]   =   {0};
                if (!cur.identifier(id))
                    break;
                const   String  chunk(id);
                if (chunk == "CNTB")
                {
                    if (!parseGroup(cur))
                        break;
                }
                else if (chunk == "PRIM")
                {
                    if (!parsePrimitive(cur))
                        break;
                }
                else if (chunk == "COLR")
                {
                    if (!parseColor(cur))
                        break;
                }
                else if (chunk == "CNTE")
                {
                    cur.skip(12);
                    closed  =   true;
                    break;
                }
                else if (!cur.readOther())
                {
                    break;
                }
                /// 组内每个块都上报一次:顶层可能只有一个 CNTB,
                /// 只在顶层循环上报会让进度从 0 直接跳到 100%
                reportProgress(cur);
            }
            _stack.pop_back();
            return  closed && cur.ok();
        }
        /// <summary>
        /// 解析 PRIM 基本体:矩阵 + 包围盒 + 参数,转换并挂接为节点
        /// </summary>
        bool    parsePrimitive(Cursor& cur)
        {
            const   uint64  start   =   cur.pos();
            const   uint32  next    =   cur.u32();
            cur.u32();
            cur.u32();
            const   uint32  kind    =   cur.u32();
            float   matrix[12]      =   {0};
            for (auto& v : matrix)
                v   =   cur.f32();
            float   bbox[6]         =   {0};
            for (auto& v : bbox)
                v   =   cur.f32();
            if (!cur.ok())
                return  false;

            if (kind == 11)
            {
                std::vector<FacetPolygon>   polygons;
                if (!readFacetGroup(cur,polygons))
                    return  false;
                buildFacet(polygons,matrix,bbox);
            }
            else
            {
                static  const   uint    argCounts[10]  =   {7,3,4,3,2,2,9,2,1,2};
                if (kind < 1 || kind > 10)
                {
                    /// 未知基本体:按块偏移跳过
                    if (next > start && next <= cur.size())
                        return  cur.seek(next);
                    return  false;
                }
                float   args[9] =   {0};
                const   uint    count   =   argCounts[kind - 1];
                for (uint i = 0; i < count; ++i)
                    args[i] =   cur.f32();
                if (!cur.ok())
                    return  false;
                buildParametric(kind,args,matrix);
            }
            /// 跳过块内多余数据(兼容更高版本追加字段)
            if (next > start && next <= cur.size() && next > cur.pos())
                cur.seek(next);
            return  cur.ok();
        }
        /// <summary>
        /// 读取面片组:多边形 -> 轮廓 -> 顶点(位置 + 法线)
        /// </summary>
        bool    readFacetGroup(Cursor& cur,std::vector<FacetPolygon>& polygons)
        {
            const   uint32  count   =   cur.u32();
            if (!cur.ok() || count > 10000000)
                return  false;
            polygons.reserve(size_t(count));
            for (uint32 i = 0; i < count; ++i)
            {
                FacetPolygon    polygon;
                const   uint32  contours    =   cur.u32();
                if (!cur.ok() || contours > 100000)
                    return  false;
                polygon.resize(size_t(contours));
                for (auto& contour : polygon)
                {
                    const   uint32  vertices    =   cur.u32();
                    if (!cur.ok() || vertices > 1000000)
                        return  false;
                    contour.resize(size_t(vertices));
                    for (auto& vertex : contour)
                    {
                        vertex.first.x  =   cur.f32();
                        vertex.first.y  =   cur.f32();
                        vertex.first.z  =   cur.f32();
                        vertex.second.x =   cur.f32();
                        vertex.second.y =   cur.f32();
                        vertex.second.z =   cur.f32();
                    }
                    if (!cur.ok())
                        return  false;
                }
                polygons.emplace_back(std::move(polygon));
                /// 面片组的顶点数据量占整体比例最大,按多边形上报进度
                reportProgress(cur);
            }
            return  cur.ok();
        }
        /// <summary>
        /// 解析 COLR 调色板覆盖块
        /// </summary>
        bool    parseColor(Cursor& cur)
        {
            cur.skip(12);
            const   uint32  index   =   cur.u32();
            const   uint32  color   =   cur.u32();
            if (!cur.ok())
                return  false;
            if (index < 256)
            {
                _palette[index * 3 + 0] =   uint8((color >> 24) & 0xFF);
                _palette[index * 3 + 1] =   uint8((color >> 16) & 0xFF);
                _palette[index * 3 + 2] =   uint8((color >> 8) & 0xFF);
            }
            return  true;
        }
        /// <summary>
        /// 调色板索引 -> 线性颜色
        /// </summary>
        float4  paletteColor(uint32 index,uint8 alpha) const
        {
            if (index > 255)
                index   =   0;
            const   float   r   =   float(_palette[index * 3 + 0]) / 100.0f;
            const   float   g   =   float(_palette[index * 3 + 1]) / 100.0f;
            const   float   b   =   float(_palette[index * 3 + 2]) / 100.0f;
            return  float4(r,g,b,float(alpha) / 255.0f);
        }
        /// <summary>
        /// RVM 矩阵 -> 引擎节点变换。
        /// 矩阵按列存储 x/y/z 轴与原点的 12 个浮点数;
        /// 基本体以 Y-up 建模,左乘 -90 度 X 轴旋转对齐 RVM/引擎的 Z-up。
        /// </summary>
        static  PrimXform   makeXform(const float matrix[12],uint32 kind)
        {
            mat4r   mat(real(1.0));
            real*   p   =   &mat[0][0];
            p[0]    =   real(matrix[0]);    p[1]    =   real(matrix[1]);    p[2]    =   real(matrix[2]);
            p[4]    =   real(matrix[3]);    p[5]    =   real(matrix[4]);    p[6]    =   real(matrix[5]);
            p[8]    =   real(matrix[6]);    p[9]    =   real(matrix[7]);    p[10]   =   real(matrix[8]);
            p[12]   =   real(matrix[9]);    p[13]   =   real(matrix[10]);   p[14]   =   real(matrix[11]);

            /// 各基本体本地轴向约定不同:
            ///   PRRT(3) 环在 XZ 平面,需要 +90 度 X 旋转把 Y-up 映射到 Z-up;
            ///   FETorus(4) 已是 XY 平面绕 Z 轴;面片(11)直接使用世界坐标;
            ///   其余基本体为 Y-up(+Y 为轴向),需要 -90 度 X 旋转到 Z-up。
            if (kind != 4 && kind != 11)
            {
                const   real    angle   =   kind == 3
                                        ?   real(glm::radians(real(-90.0)))
                                        :   real(glm::radians(real(90.0)));
                const   mat4r   fix     =   glm::rotate(mat4r(real(1.0)),angle,real3(1,0,0));
                mat                     =   mat * fix;
            }

            PrimXform   xform;
            FE::decompose(mat,xform.translation,xform.scale,xform.rotation);
            if (!std::isfinite(xform.translation.x) || !std::isfinite(xform.translation.y) || !std::isfinite(xform.translation.z))
                xform.translation   =   real3(0.0);
            if (!std::isfinite(xform.scale.x) || !std::isfinite(xform.scale.y) || !std::isfinite(xform.scale.z)
                || std::fabs(xform.scale.x) < 1e-12 || std::fabs(xform.scale.y) < 1e-12 || std::fabs(xform.scale.z) < 1e-12)
                xform.scale =   real3(1.0);
            xform.rotation  =   glm::normalize(xform.rotation);
            return  xform;
        }
        /// <summary>
        /// 构造基本体网格(参数单位为毫米,由节点缩放转换到米)
        /// </summary>
        Mesh    buildParametric(uint32 kind,const float args[9],const float matrix[12])
        {
            const   Attrs   attrs   =   {
                FEAttribute(IS_VERTEX_POS,      FMT_R32G32B32_FLOAT),
                FEAttribute(IS_VERTEX_NOR,      FMT_R32G32B32_FLOAT),
                FEAttribute(IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT)
            };
            Mesh    mesh    =   nullptr;
            switch (kind)
            {
            case 1:     ///< 棱台(沿局部 Z 轴,建模时映射到 Y 轴)
                {
                    FEGeometryPRPY  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.bottomXlen    =   args[0];
                    p.bottomYlen    =   args[1];
                    p.topXlen       =   args[2];
                    p.topYlen       =   args[3];
                    p.xOffset       =   args[4];
                    p.yOffset       =   -args[5];   ///< Y-up -> Z-up 后 Y 方向取反
                    p.height        =   args[6];
                    mesh            =   geo.triangular(attrs);
                }
                break;
            case 2:     ///< 长方体:x/y/z 长度在 Y-up 建模中对应 x/z/y
                {
                    FEGeometryBox   geo(_ctx);
                    geo.param()._size   =   float3(args[0],args[2],args[1]);
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 3:     ///< 方环体:外半径,内半径,高度,角度(弧度)
                {
                    FEGeometryPRRT  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.outerRadius   =   args[0];
                    p.innerRadius   =   args[1];
                    p.height        =   args[2];
                    p.arcAngle      =   args[3];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 4:     ///< 圆环体:偏移,管半径,角度(弧度)
                {
                    FEGeometryTorus geo(_ctx);
                    auto&   p   =   geo.param();
                    p.radius        =   args[0];
                    p.tube          =   args[1];
                    p.arc           =   args[2];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 5:     ///< 椭圆碟:半径,高度
                {
                    FEGeometryPRDH  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.radius        =   args[0];
                    p.height        =   args[1];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 6:     ///< 球冠:半径,高度
                {
                    FEGeometryPRCP  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.radius        =   args[0];
                    p.height        =   args[1];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 7:     ///< 锥台:底半径,顶半径,高度,偏移与倾角
                {
                    FEGeometryPRCN  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.bottomRadius  =   args[0];
                    p.topRadius     =   args[1];
                    p.height        =   args[2];
                    p.xOffset       =   args[3];
                    p.yOffset       =   -args[4];
                    p.bottomTiltX   =   args[5];
                    p.bottomTiltY   =   args[6];
                    p.topTiltX      =   args[7];
                    p.topTiltY      =   args[8];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 8:     ///< 圆柱:半径,高度
                {
                    FEGeometryCylinder  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.radiusTop     =   args[0];
                    p.radiusBottom  =   args[0];
                    p.height        =   args[1];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 9:     ///< 球:半径
                {
                    FEGeometrySphere    geo(_ctx);
                    geo.param().radius  =   args[0];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            case 10:    ///< 线:半径,长度(用圆柱近似)
                {
                    FEGeometryCylinder  geo(_ctx);
                    auto&   p   =   geo.param();
                    p.radiusTop     =   args[0];
                    p.radiusBottom  =   args[0];
                    p.height        =   args[1];
                    mesh    =   geo.triangular(attrs);
                }
                break;
            default:
                break;
            }
            if (mesh == nullptr)
                return  nullptr;
            mesh->updateAabb();
            attachPrimitive(mesh,primitiveXform(kind,args,matrix));
            return  mesh;
        }
        /// <summary>
        /// 基本体节点变换。
        /// 棱台/圆台的 X/Y 偏移在 RVM 中以下表面中心为 -offset/2、上表面中心为 +offset/2,
        /// 而引擎基本体以下表面中心为 0,故把 -offset/2 补偿到节点平移上。
        /// </summary>
        static  PrimXform   primitiveXform(uint32 kind,const float args[9],const float matrix[12])
        {
            PrimXform   xform   =   makeXform(matrix,kind);
            real3       shift(0.0);
            if (kind == 1)
                shift   =   real3(real(-args[4]) * real(0.5),real(0.0),real(args[5]) * real(0.5));
            else if (kind == 7)
                shift   =   real3(real(-args[3]) * real(0.5),real(0.0),real(args[4]) * real(0.5));
            if (shift != real3(0.0))
            {
                const   real3   local   =   shift * xform.scale;
                xform.translation      +=   glm::mat3_cast(xform.rotation) * local;
            }
            return  xform;
        }
        /// <summary>
        /// 构造面片组网格:多边形三角化后写入位置/法线,节点平移取包围盒中心
        /// </summary>
        void    buildFacet(const std::vector<FacetPolygon>& polygons,const float matrix[12],const float bbox[6])
        {
            const   float3  centerRvm((bbox[0] + bbox[3]) * 0.5f,
                                      (bbox[1] + bbox[4]) * 0.5f,
                                      (bbox[2] + bbox[5]) * 0.5f);

            floats          poss;
            floats          nors;
            floats          uvs;
            uints           indices;
            std::map<std::array<uint32,6>,uint32>  vertexMap;

            for (const auto& polygon : polygons)
            {
                if (polygon.empty() || polygon[0].size() < 3)
                    continue;
                /// 平面法线:优先使用数据法线,否则用 Newell 法
                float3  normal(0.0f);
                for (const auto& v : polygon[0])
                    normal  +=  v.second;
                if (glm::length(normal) < 1e-12f)
                    normal  =   newellNormal(polygon[0]);
                if (glm::length(normal) < 1e-12f)
                    continue;
                normal  =   glm::normalize(normal);

                const   float3  axis    =   (std::fabs(normal.x) <= std::fabs(normal.y) && std::fabs(normal.x) <= std::fabs(normal.z))
                                        ?   float3(1,0,0)
                                        :   ((std::fabs(normal.y) <= std::fabs(normal.z)) ? float3(0,1,0) : float3(0,0,1));
                const   float3  u       =   glm::normalize(glm::cross(normal,axis));
                const   float3  v       =   glm::cross(normal,u);
                const   float3  origin  =   polygon[0][0].first;

                float2s         outer2d;
                std::vector<float2s>    holes2d;
                float2s         all2d;
                std::vector<FacetVertex>    all3d;
                auto    fnProject   =   [&](const FacetContour& contour,float2s& out)
                {
                    out.reserve(contour.size());
                    for (const auto& vertex : contour)
                    {
                        const   float3  d   =   vertex.first - origin;
                        const   float2  p2(glm::dot(d,u),glm::dot(d,v));
                        out.emplace_back(p2);
                        all2d.emplace_back(p2);
                        all3d.emplace_back(vertex);
                    }
                };
                /// 去掉首尾重复点,避免退化轮廓
                auto    fnStrip =   [](const FacetContour& contour) -> FacetContour
                {
                    FacetContour    result(contour);
                    if (result.size() > 3)
                    {
                        const   float3  d   =   result.front().first - result.back().first;
                        if (glm::dot(d,d) < 1e-12f)
                            result.pop_back();
                    }
                    return  result;
                };
                fnProject(fnStrip(polygon[0]),outer2d);
                if (outer2d.size() < 3)
                    continue;
                for (size_t i = 1; i < polygon.size(); ++i)
                {
                    float2s hole;
                    fnProject(fnStrip(polygon[i]),hole);
                    if (hole.size() >= 3)
                        holes2d.emplace_back(std::move(hole));
                }

                float2s outPoints;
                uints   tris    =   FEEarcut::triangulate(outer2d,holes2d,&outPoints);
                if (tris.empty() || outPoints.empty())
                {
                    LOG_WAR("RVM facet: triangulate empty(outer=%zu holes=%zu)",outer2d.size(),holes2d.size());
                    continue;
                }

                /// 输出点 -> 原始顶点索引(桥接点与原轮廓点坐标一致)
                std::vector<uint32> pointIndex(outPoints.size(),0);
                for (size_t i = 0; i < outPoints.size(); ++i)
                {
                    float       best    =   1e30f;
                    uint32      bestIdx =   0;
                    for (size_t k = 0; k < all2d.size(); ++k)
                    {
                        const   float2  d   =   outPoints[i] - all2d[k];
                        const   float   dd  =   d.x * d.x + d.y * d.y;
                        if (dd < best)
                        {
                            best    =   dd;
                            bestIdx =   uint32(k);
                        }
                    }
                    pointIndex[i]   =   bestIdx;
                }

                for (size_t i = 0; i + 2 < tris.size(); i += 3)
                {
                    for (uint j = 0; j < 3; ++j)
                    {
                        const   uint32  pi  =   pointIndex[tris[i + j]];
                        const   auto&   vertex  =   all3d[pi];
                        uint32  vi  =   addVertex(vertex,centerRvm,nors,poss,uvs,vertexMap);
                        indices.emplace_back(vi);
                    }
                }
            }

            if (poss.empty() || indices.empty())
                return;
            const   Attrs   attrs   =   {
                FEAttribute(IS_VERTEX_POS,      FMT_R32G32B32_FLOAT),
                FEAttribute(IS_VERTEX_NOR,      FMT_R32G32B32_FLOAT),
                FEAttribute(IS_VERTEX_TEXCOORD0,FMT_R32G32_FLOAT)
            };
            Mesh    mesh    =   FEMeshBuilder::makeMesh(_ctx,attrs,poss,nors,{},uvs,indices);
            if (mesh == nullptr)
                return;
            mesh->updateAabb();

            /// 面片数据已是世界坐标(mm),节点平移取包围盒中心,网格减去中心减少大坐标精度损失
            PrimXform   xform   =   makeXform(matrix,11);
            xform.translation   =   real3(real(centerRvm.x),real(centerRvm.y),real(centerRvm.z)) * real(0.001);
            attachPrimitive(mesh,xform);
        }
        static  float3  newellNormal(const FacetContour& contour)
        {
            float3  n(0.0f);
            for (size_t i = 0; i < contour.size(); ++i)
            {
                const   float3& p   =   contour[i].first;
                const   float3& q   =   contour[(i + 1) % contour.size()].first;
                n.x +=  (p.y - q.y) * (p.z + q.z);
                n.y +=  (p.z - q.z) * (p.x + q.x);
                n.z +=  (p.x - q.x) * (p.y + q.y);
            }
            return  n;
        }
        /// <summary>
        /// 面片顶点相对包围盒中心写入并去重,返回索引
        /// </summary>
        uint32  addVertex(const FacetVertex& vertex,const float3& center,
                          floats& nors,floats& poss,floats& uvs,
                          std::map<std::array<uint32,6>,uint32>& vertexMap)
        {
            const   float3  local   =   vertex.first - center;
            float3          normal(vertex.second);
            if (glm::length(normal) < 1e-12f)
            {
                normal  =   float3(0,1,0);
            }
            else
            {
                normal  =   glm::normalize(normal);
            }
            std::array<uint32,6>    key;
            std::memcpy(&key[0],&local.x,4);
            std::memcpy(&key[1],&local.y,4);
            std::memcpy(&key[2],&local.z,4);
            std::memcpy(&key[3],&normal.x,4);
            std::memcpy(&key[4],&normal.y,4);
            std::memcpy(&key[5],&normal.z,4);

            auto    itr =   vertexMap.find(key);
            if (itr != vertexMap.end())
                return  itr->second;
            const   uint32  index   =   uint32(poss.size() / 3);
            poss.insert(poss.end(),{local.x,local.y,local.z});
            nors.insert(nors.end(),{normal.x,normal.y,normal.z});
            uvs.insert(uvs.end(),{0.0f,0.0f});
            vertexMap.emplace(key,index);
            return  index;
        }
        /// <summary>
        /// 延迟创建当前组节点(空组不创建),并挂接网格/材质/变换
        /// </summary>
        Node    ensureNode(size_t index)
        {
            auto&   ctx =   _stack[index];
            if (ctx.node != nullptr)
                return  ctx.node;
            ctx.node    =   new FENode(_ctx);
            ctx.node->setName(ctx.name);
            if (index == 0)
                _root->addChild(ctx.node);
            else
                ensureNode(index - 1)->addChild(ctx.node);
            return  ctx.node;
        }
        /// <summary>
        /// 挂接基本体:组节点保持单位变换,每个基本体独立子节点承载矩阵
        /// (RVM 矩阵为绝对世界变换,父子节点不能叠加)
        /// </summary>
        void    attachPrimitive(Mesh mesh,const PrimXform& xform)
        {
            if (mesh == nullptr || _stack.empty())
                return;
            auto&   ctx     =   _stack.back();
            Node    parent  =   ensureNode(_stack.size() - 1);
            Node    target  =   new FENode(_ctx);
            target->setName(ctx.name + "_" + std::to_string(ctx.primCount));
            parent->addChild(target);
            Material    material    =   materialFor(ctx.color);
            target->setLocalTranslation(xform.translation);
            target->setLocalScaling(xform.scale);
            target->setLocalRotation(xform.rotation);
            target->setMesh(mesh.get());
            target->setMaterial(material.get());
            ++  ctx.primCount;
        }
        /// <summary>
        /// 按颜色缓存 PBR 材质(金属度-粗糙度工作流,颜色只走 baseColor,
        /// 不写自发光,让灯光/IBL 正常参与着色)
        /// </summary>
        Material    materialFor(const float4& color)
        {
            const   uint32  key =   (uint32(color.x * 255.0f) << 24) | (uint32(color.y * 255.0f) << 16)
                                |   (uint32(color.z * 255.0f) << 8)  |  uint32(color.w * 255.0f);
            auto    itr =   _materials.find(key);
            if (itr != _materials.end())
                return  itr->second;

            Material    material    =   new FEMaterialPBR(_ctx);
            auto        pbr         =   material->as<FEMaterialPBR>();
            auto&       value       =   pbr->data()._value;
            value._diffuse          =   color;
            value._emissive         =   float4(0.0f,0.0f,0.0f,1.0f);
            value._metallic         =   0.7f;
            value._roughness        =   0.3f;
            pbr->data().update();
            if (color.w < 0.999f)
                pbr->setAlphaBlend(true);
            _materials.emplace(key,material);
            return  material;
        }
        void    resetPalette()
        {
            static  const   uint8   defaults[256 * 3]   =
            {
                 80, 80, 80, 80, 80, 80, 80,  0,  0, 93, 60,  0,
                 80, 80,  0,  0, 80,  0,  0, 93, 93,  0,  0, 80,
                 87,  0, 87, 80, 17, 17,100,100,100, 98, 50, 44,
                 75, 75, 75, 66, 66, 66, 55, 40, 55, 96, 96, 96,
                 56, 14, 42,  0,100, 50, 96, 87, 70, 93, 79, 20,
                 28, 46,100, 93, 91, 67, 93,  7, 54, 14, 56, 14,
                100, 65,  0, 93, 93, 88, 93, 46, 13, 28, 51, 71,
                100,100,100, 18, 18, 31,  0,  0, 50, 80, 57, 62,
                 80, 36, 27,  0,  0,  0, 80,  0,  0, 93, 60,  0,
                 80, 80,  0,  0, 80,  0,  0, 93, 93,  0,  0, 80,
                 87,  0, 87, 80, 17, 17,100,100,100, 98, 50, 44,
                 75, 75, 75, 66, 66, 66, 55, 40, 55, 96, 96, 96,
                 56, 14, 42,  0,100, 50, 96, 87, 70, 93, 79, 20,
                 28, 46,100, 93, 91, 67, 93,  7, 54, 14, 56, 14,
                100, 65,  0, 93, 93, 88, 93, 46, 13, 28, 51, 71,
                100,100,100, 18, 18, 31,  0,  0, 50, 80, 57, 62,
                 80, 36, 27, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
                 80, 80, 80, 80, 80, 80,  0,  0,  0,100,100,100,
                 96, 96, 96, 93, 93, 88, 66, 66, 66, 75, 75, 75,
                 32, 55, 55, 18, 31, 31, 80,  0,  0,100,  0,  0,
                 80, 36, 27,100, 39, 28, 55, 40, 55, 93,  7, 54,
                 80, 57, 62, 98, 50, 44, 93, 60,  0,100, 65,  0,
                100, 50,  0, 56, 14, 42, 80, 80,  0, 93, 79, 20,
                 93, 93, 82, 93, 91, 67, 60, 80, 20,  0,100, 50,
                  0, 80,  0, 14, 56, 14, 18, 31, 18,  0, 93, 93,
                  0, 75, 80, 46, 93, 78,  0,  0, 80, 28, 46,100,
                  0,  0, 50, 69, 88, 90, 18, 18, 31, 28, 51, 71,
                 20,  0, 40, 40,  0, 60, 93, 51, 93, 87,  0, 87,
                 96, 96, 86, 96, 87, 70, 86, 58, 44, 96, 65, 37,
                 80, 17, 17, 62, 62, 37, 93, 46, 13, 55, 27,  8,
            };
            static_assert(sizeof(defaults) == sizeof(_palette),"RVM palette size mismatch");
            std::memcpy(_palette,defaults,sizeof(_palette));
        }
    protected:
        Node            _root  =   nullptr;
        std::vector<GroupCtx>   _stack;
    };
}
