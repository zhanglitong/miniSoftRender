#pragma     once

#include    "../graphic/FEDevice.h"
#include    "../graphic/FEOITManager.h"
#include    "../FELog.hpp"
#include    "../font/FEFont.hpp"
#include    "FEMaterial.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialText,"{8B4C1D2E-6A97-4E35-8F10-3C7D5B92A6E4}");
    /// <summary>
    /// 距离场文本材质:SDF 文本管线(text/*),
    /// 图集通过 combined image sampler 绑定到 SB_Material(4),
    /// 视口像素尺寸经 push constant 传入,顶点着色器据此完成像素->NDC 变换。
    /// 两种绘制模式(setBillboard 切换,经 push constant _sdf.z 传给顶点着色器):
    ///   屏幕叠加(默认):关闭深度测试与背面剔除,文本作为屏幕叠加层始终最前;
    ///   三维布告板:节点平移为世界锚点,文本始终面向相机且屏幕像素尺寸恒定,
    ///   开启深度测试(不写深度),可被场景几何遮挡。
    /// 均开启 alpha 混合。
    /// </summary>
    class   FE_API  FEMaterialText:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FEMaterialText)
    public:
        /// <summary>
        /// push constant 布局,须与 text/shader.vert/.frag(及 .wgls)中的 TextPush 一致
        /// </summary>
        struct  TextData
        {
            /// <summary>
            /// xy = 视口像素尺寸, zw = 1/视口尺寸
            /// </summary>
            float4  _screen =   float4(1280.0f,720.0f,1.0f / 1280.0f,1.0f / 720.0f);
            /// <summary>
            /// x = 每个烘焙像素对应的 SDF 编码增量 = 1/(2*spread),
            /// 片元着色器据此换算恒定的抗锯齿宽度(须与字体实际 spread 一致)
            /// y = 渲染模式(0=SDF,1=位图)
            /// z = 绘制模式(0=屏幕叠加,1=三维布告板)
            /// </summary>
            float4  _sdf    =   float4(1.0f / 16.0f,0.0f,0.0f,0.0f);
            /// <summary>
            /// x = 描边宽度(屏幕像素,0=无描边); yzw = 描边颜色 RGB(归一化 0~1)
            /// </summary>
            float4  _stroke =   float4(0.0f,0.0f,0.0f,1.0f);
        };
    public:
        FEMaterialText(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("text");
            autoAttach();
            /// 显式绑定相机 UBO:text 着色器的相机块经反射得到的块名为
            /// "CameraBlock" 而非 "_camera",autoAttach 的名称匹配覆盖不到,
            /// 缺少此绑定会导致布告板模式取不到相机 VP 矩阵(同 v3c4 的教训)
            auto    cameraUBO   =   ctx.device().queryCache(FEConstUuid::CameraUBOId);
            bind(0,SB_Camera,   {cameraUBO});

            if (_texture != nullptr)
                bind(0,SB_Material,{_texture.get()});

            update();
        }

        FEMaterialText(const FEMaterialText& other)
            :FEMaterial(other)
        {
            _font          =   other._font;
            _texture       =   other._texture;
            _strokeWidth   =   other._strokeWidth;
            _strokeColor   =   other._strokeColor;
            _screenOverride=   other._screenOverride;
            _data          =   other._data;
            if (_texture != nullptr)
            {
                bind(0,SB_Material,{_texture.get()});
                update();
            }
        }
        virtual ~FEMaterialText()
        {}
        /// <summary>
        /// 设置字体(内部完成 SDF 图集绑定与上传)
        /// </summary>
        inline  void    setFont(Font font)
        {
            _font      =   font;
            /// 取出图集引用(非 const 局部,以取得可用的裸指针)
            _texture   =   (_font != nullptr) ? _font->texture() : nullptr;
            /// 渲染模式:SDF 由 spread 推算编码梯度;位图直接用覆盖率
            const   bool    bitmap  =   (_font != nullptr) && _font->isBitmap();
            _data._sdf.x =  (bitmap) ? 0.0f
                          : ((_font != nullptr) ? 0.5f / (real)_font->sdfSpread() : 1.0f / 16.0f);
            /// .y = 渲染模式标志(0=SDF,1=位图)
            _data._sdf.y =  bitmap ? 1.0f : 0.0f;
            if (_texture != nullptr)
            {
                bind(0,SB_Material,{_texture.get()});
                update();
            }
        }
        /// <summary>
        /// 获取字体
        /// </summary>
        inline  const   Font&   font() const
        {
            return  _font;
        }
        /// <summary>
        /// 设置绘制模式:false=屏幕叠加层(默认),true=三维布告板。
        /// 布告板模式下节点平移为世界锚点,文本面向相机且屏幕像素尺寸恒定,
        /// 可被场景几何深度遮挡;网格须以锚点为原点构建(createTextMesh origin 传 (0,0)
        /// 或按期望的对齐方式预偏移)。
        /// </summary>
        inline  void    setBillboard(bool b)
        {
            _data._sdf.z    =   b ? 1.0f : 0.0f;
        }
        /// <summary>
        /// 当前是否为三维布告板模式
        /// </summary>
        inline  bool    isBillboard() const
        {
            return  _data._sdf.z > 0.5f;
        }
        /// <summary>
        /// 覆盖屏幕像素尺寸(0 表示恢复为取窗口尺寸)。
        /// 离屏/无头渲染时窗口尺寸不可用(ctx.windowsWidth 返回极大值),
        /// 屏幕叠加模式必须显式传入渲染目标尺寸,否则像素->NDC 变换会按
        /// 1280x720 的兜底值计算,文本位置与字号全部错位。
        /// </summary>
        inline  void    setScreenSize(uint32 w,uint32 h)
        {
            _screenOverride =   uint2(w,h);
        }
        inline  void    clearScreenSize()
        {
            _screenOverride =   uint2(0);
        }
        /// <summary>
        /// 设置描边宽度(屏幕像素)。
        /// 0 表示无描边(默认),此时渲染与未开启描边完全一致。
        /// 描边位于字形轮廓外侧,宽度在屏幕空间恒定(缩放/布告板下粗细不变)。
        /// 仅 SDF 模式生效;位图模式(hinting 灰度)下描边为 no-op。
        /// </summary>
        inline  void    setStrokeWidth(float widthPx)
        {
            _strokeWidth    =   widthPx > 0.0f ? widthPx : 0.0f;
            _data._stroke.x =   _strokeWidth;
        }
        /// <summary>
        /// 当前描边宽度(屏幕像素)
        /// </summary>
        inline  float   strokeWidth() const
        {
            return  _strokeWidth;
        }
        /// <summary>
        /// 设置描边颜色(RGB,alpha 复用节点颜色的 alpha)。
        /// </summary>
        inline  void    setStrokeColor(const Rgba8& color)
        {
            _strokeColor    =   color;
            const   auto&   v   =   color.value();
            _data._stroke.y =   (float)v.x / 255.0f;
            _data._stroke.z =   (float)v.y / 255.0f;
            _data._stroke.w =   (float)v.z / 255.0f;
        }
        /// <summary>
        /// 当前描边颜色
        /// </summary>
        inline  Rgba8   strokeColor() const
        {
            return  _strokeColor;
        }
        /// <summary>
        /// text 管线开启了 alpha 混合(enableColorBlend=1),属于透明材质。
        /// 必须返回 true,否则工厂的不透明/透明两遍分组与渲染层排序会把文本
        /// 当作不透明物体先画,被后续不透明几何整体覆盖(commit 18c5b40 引入
        /// isTransparent 排序后暴露的问题)。
        /// </summary>
        virtual bool        isTransparent() const override
        {
            return  true;
        }
        /// ===== OIT 变体管线(文本为透明材质,参与逐像素排序)=====
        /// <summary>
        /// LIST(几何)趟绘制本材质用的管线:惰性构建 OIT 变体,
        /// 无 OIT 管理器或构建失败时返回 null(渲染层回退普通 alpha 混合)
        /// </summary>
        virtual Pipeline    oitPipeline(EPrimitive pri) override
        {
            if (_oitPipelines[uint(pri)] == nullptr && !_oitVariantFailed)
                setupOITVariants();
            return  _oitPipelines[uint(pri)];
        }
        /// <summary>
        /// 构建文本的 OIT 变体管线与描述符集:
        /// 克隆主管线改 4 项状态(无 color 输出/不写深度/无混合/采样数与场景一致),
        /// FS 换成 OIT_MODE 编译的 text/shader.oit.frag.spv(写 PPLL 链表 10/11/12),
        /// 注册名 "text.oit/<图元>";dsets 绑定相机 UBO、字体图集与 OIT SSBO。
        /// 仅在 FEOITManager::active() 时构建,LIST 趟首次绘制前调用
        /// </summary>
        void    setupOITVariants()
        {
            for (auto& pl : _oitPipelines)
                pl      =   nullptr;
            _oitDsets.clear();
            auto    mgr     =   FEOITManager::active();
            if (mgr == nullptr)
                return;
            if (_oitFS == nullptr)
            {
                _oitFS  =   loadOITFragmentShader();
                if (_oitFS == nullptr)
                {
                    _oitVariantFailed   =   true;
                    LOG_ERR("Text OIT: load text/shader.oit.frag.spv failed, text falls back to alpha blend");
                    return;
                }
            }
            auto&   plMgr   =   _ctx.device().pipelines();
            for (uint8 i = 0; i < PRI_MAX; ++i)
            {
                if (_pipelines[i] == nullptr)
                    continue;
                String  plName  =   "text.oit/" + String(nameOfEnum(EPrimitive(i)));
                Pipeline    pl;
                if (!plMgr.query(plName,pl))
                {
                    /// 克隆主管线:几何趟无 color attachment、不写深度、无颜色混合
                    FEPipeline::CreateInfo  cInfo   =   _pipelines[i]->cInfo();
                    /// text.xml 的 <shaders> 顺序不固定,按 stage 类型替换片元着色器
                    for (auto& shader : cInfo._shaders)
                    {
                        if (shader != nullptr && shader->cInfo()._shaderType == ST_FRAGMENT_BIT)
                            shader  =   _oitFS;
                    }
                    cInfo._colorAttachmentCount =   0;
                    cInfo._enableDepthWrite     =   false;
                    cInfo._enableColorBlend     =   false;
                    cInfo._sampleCount          =   mgr->sampleCount();
                    pl      =   _ctx.device().createGPipeline();
                    if (pl == nullptr || !pl->create(cInfo))
                    {
                        _oitVariantFailed   =   true;
                        LOG_ERR("Text OIT: create pipeline %s failed", plName.c_str());
                        return;
                    }
                    pl->setName(plName);
                    plMgr.add(plName,pl);
                }
                _oitPipelines[i]    =   pl;
            }
            if (_oitPipelines[0] == nullptr)
                return;
            _oitDsets   =   _oitPipelines[0]->createDSets();
            if (_oitDsets.empty())
            {
                _oitVariantFailed   =   true;
                LOG_ERR("Text OIT: createDSets for text.oit failed");
                return;
            }
            /// 与主管线绑定同样的资源:相机 UBO(着色器块名为 CameraBlock,
            /// autoAttach 的名称匹配覆盖不到,须显式绑定)与字体图集;
            /// 再挂 OIT SSBO(binding 10/11/12,与 text OIT 着色器一致)
            auto    cameraUBO   =   _ctx.device().queryCache(FEConstUuid::CameraUBOId);
            bindTo(_oitDsets,0,SB_Camera,{cameraUBO});
            if (_texture != nullptr)
                bindTo(_oitDsets,0,SB_Material,{_texture.get()});
            for (auto& dset : _oitDsets)
                mgr->bindToDSet(dset);
            update();
        }
        /// <summary>
        /// 加载 OIT_MODE 编译的 text fragment shader(输出写 PPLL 链表)
        /// </summary>
        Shader  loadOITFragmentShader()
        {
            constexpr   const char* OIT_FRAG_PATH   =   ":/data/shader/cache/text/shader.oit.frag.spv";
            String      absPath =   _ctx.resourcePath();
            absPath     +=  (OIT_FRAG_PATH + 1);    /// 跳过 ':',与 FEOITManager 同规则
            auto    buf     =   FEBuffer::loadFile(_ctx,absPath.c_str());
            if (buf == nullptr)
                return  nullptr;
            auto    shader  =   _ctx.device().createShader();
            FEShader::CreateInfo    sci;
            sci._shaderType =   ST_FRAGMENT_BIT;
            sci._buffer     =   buf;
            if (!shader->create(sci))
                return  nullptr;
            return  shader;
        }
        virtual uint    pushConstantSize() const override
        {
            return  (uint)sizeof(TextData);
        }
        virtual PCVOID  pushConstantData() const override
        {
            return  (PCVOID)&_data;
        }
        virtual void    appDynamicState(CMDPtr cmd,EPrimitive pri) override
        {
            /// 屏幕像素尺寸用于顶点着色器的像素->NDC 变换(优先使用显式覆盖值)
            uint32  w   =   _screenOverride.x != 0 ? _screenOverride.x : _ctx.windowsWidth();
            uint32  h   =   _screenOverride.y != 0 ? _screenOverride.y : _ctx.windowsHeight();
            if (w == 0 || h == 0 || w > 100000 || h > 100000)
            {
                w   =   1280;
                h   =   720;
            }
            _data._screen   =   float4((real)w,(real)h,1.0f / (real)w,1.0f / (real)h);

            FEMaterial::appDynamicState(cmd,pri);

            /// 文本不剔除;深度按模式区分:
            ///   屏幕叠加:关闭深度测试(同时避免深度写入),始终绘制在最前
            ///   三维布告板:开启深度测试(管线侧已关闭写入),可被场景几何遮挡
            cmd->setDepthTest(isBillboard());
            cmd->setCullMode(CullMode::CM_NULL);
        }
    protected:
        Font        _font      =   nullptr;
        /// <summary>
        /// SDF 图集(与 _font 同步,单独持有便于绑定描述符)
        /// </summary>
        TexPtr      _texture   =   nullptr;
        /// <summary>
        /// 描边宽度(屏幕像素),0=无描边;与 _data._stroke.x 同步
        /// </summary>
        float       _strokeWidth  =   0.0f;
        /// <summary>
        /// 描边颜色;与 _data._stroke.yzw 同步
        /// </summary>
        Rgba8       _strokeColor  =   Rgba8(0,0,0,255);
        /// <summary>
        /// 屏幕尺寸覆盖(0,0=使用窗口尺寸),离屏渲染由使用方设置
        /// </summary>
        uint2       _screenOverride =   uint2(0);
        mutable TextData    _data;
        /// <summary>
        /// OIT 变体管线(按图元类型)与片元着色器,惰性构建
        /// </summary>
        Pipeline    _oitPipelines[PRI_MAX]  =   {};
        Shader      _oitFS;
        bool        _oitVariantFailed       =   false;
    };
}
