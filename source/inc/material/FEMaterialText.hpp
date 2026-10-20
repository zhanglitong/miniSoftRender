#pragma     once

#include    "../graphic/FEDevice.h"
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
            /// 屏幕像素尺寸用于顶点着色器的像素->NDC 变换
            uint32  w   =   _ctx.windowsWidth();
            uint32  h   =   _ctx.windowsHeight();
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
        mutable TextData    _data;
    };
}
