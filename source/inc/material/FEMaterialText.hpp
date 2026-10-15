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
    /// 文本作为屏幕叠加层:关闭深度测试与背面剔除,并开启 alpha 混合。
    /// </summary>
    class   FE_API  FEMaterialText:public FEMaterial
    {
    public:
        IMPLEMENT_CLASS_REFLECT_MINI(FEMaterialText)
    public:
        /// push constant 布局,须与 text/shader.vert 中的 TextPush 一致
        struct  TextData
        {
            /// xy = 视口像素尺寸, zw = 1/视口尺寸
            float4  _screen =   float4(1280.0f,720.0f,1.0f / 1280.0f,1.0f / 720.0f);
            /// x = 每个烘焙像素对应的 SDF 编码增量 = 1/(2*spread),
            /// 片元着色器据此换算恒定的抗锯齿宽度(须与字体实际 spread 一致)
            float4  _sdf    =   float4(1.0f / 16.0f,0.0f,0.0f,0.0f);
        };
    public:
        FEMaterialText(FEContext& ctx)
            :FEMaterial(ctx)
        {
            setup("text");
            autoAttach();

            if (_texture != nullptr)
                bind(0,SB_Material,{_texture.get()});

            update();
        }

        FEMaterialText(const FEMaterialText& other)
            :FEMaterial(other)
        {
            _font      =   other._font;
            _texture   =   other._texture;
            _data      =   other._data;
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

            /// 文本为屏幕叠加层:关闭深度测试(同时避免深度写入)且不剔除
            cmd->setDepthTest(false);
            cmd->setCullMode(CullMode::CM_NULL);
        }
    protected:
        Font        _font      =   nullptr;
        /// SDF 图集(与 _font 同步,单独持有便于绑定描述符)
        TexPtr      _texture   =   nullptr;
        mutable TextData    _data;
    };
}
