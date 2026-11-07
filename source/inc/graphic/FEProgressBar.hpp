#pragma     once

#include    "FEFactoryRender.hpp"
#include    "../FEProgress.hpp"
#include    "../font/FEFont.hpp"
#include    "../node/FENode.hpp"

namespace   FE
{
    /// <summary>
    /// 不依赖 ImGui 的屏幕进度条(叠加层工厂):
    ///   - 背景/填充/边框由 progress 管线(全屏矩形 + push constant)绘制,
    ///     百分比文本复用 SDF 文本材质,窗口与离屏场景都可用;
    ///   - 归属 Overlay 层并在工厂排序中最后绘制,关闭深度测试,不参与剔除;
    ///   - 用法:
    ///       auto bar = new FEProgressBar(ctx);
    ///       scene->factoryMgr().addObject(bar);
    ///       bar->bindProgress(task.get());   // 或每帧 bar->setProgress(v)
    /// 注意:进度源须在工作线程或分帧任务中推进;同步阻塞的加载期间帧循环
    ///       不运行,进度条只会从 0 直接跳到 1。
    /// </summary>
    class   FE_API  FEProgressBar : public FEFactoryRender
    {
    public:
        /// <summary>
        /// 外观参数(默认:屏幕底部居中、60% 屏宽、10px 高、绿色填充)
        /// </summary>
        struct  Style
        {
            /// <summary>
            /// 背景色(RGBA)
            /// </summary>
            float4  _background     =   float4(0.1f,0.1f,0.1f,0.80f);
            /// <summary>
            /// 填充色(RGBA)
            /// </summary>
            float4  _fill           =   float4(0.20f,0.85f,0.35f,1.00f);
            /// <summary>
            /// 边框色(RGBA,alpha=0 表示不画边框)
            /// </summary>
            float4  _border         =   float4(1.0f,1.0f,1.0f,0.35f);
            /// <summary>
            /// 边框宽度(像素)
            /// </summary>
            float   _borderWidth    =   1.0f;
            /// <summary>
            /// 进度条宽度占屏幕宽度的比例
            /// </summary>
            float   _widthScale     =   0.6f;
            /// <summary>
            /// 进度条高度(像素)
            /// </summary>
            float   _height         =   10.0f;
            /// <summary>
            /// 距屏幕底部的边距(像素)
            /// </summary>
            float   _bottomMargin   =   32.0f;
            /// <summary>
            /// 是否在进度条上方显示百分比文本
            /// </summary>
            bool    _showText       =   true;
            /// <summary>
            /// 百分比文本字号(像素)
            /// </summary>
            float   _textSize       =   16.0f;
            /// <summary>
            /// 文本与进度条上沿的间距(像素)
            /// </summary>
            float   _textGap        =   6.0f;
            /// <summary>
            /// 文本颜色
            /// </summary>
            Rgba8   _textColor      =   Rgba8(255,255,255,255);
        };
    public:
        explicit    FEProgressBar(FEContext& ctx);
        virtual     ~FEProgressBar();
        /// <summary>
        /// 设置进度(自动钳制到 0~1)
        /// </summary>
        void        setProgress(real val);
        /// <summary>
        /// 当前进度
        /// </summary>
        real        progress() const;
        /// <summary>
        /// 绑定进度源:每帧轮询 source->progress()。
        /// 传入 nullptr 取消绑定(保留当前进度值)
        /// </summary>
        void        bindProgress(FEProgress* source);
        /// <summary>
        /// 修改外观参数(下一帧生效)
        /// </summary>
        void        setStyle(const Style& style);
        const   Style&  style() const;
        /// <summary>
        /// 显示/隐藏(隐藏时不绘制,也不重建文本)
        /// </summary>
        void        setVisible(bool show);
        bool        visible() const;
        /// <summary>
        /// 指定字体;不设置时首次绘制自动加载 data/fonts/consola.ttf
        /// </summary>
        void        setFont(Font font);
        const   Font&   font() const;
    public:
        virtual void    update(CMDPtr cmd) override;
        virtual void    render(CMDPtr cmd) override;
    protected:
        /// <summary>
        /// 与 progress/shader.vert/.frag 中的 ProgressPush 布局一致
        /// </summary>
        struct  PushData
        {
            /// <summary>
            /// NDC 矩形: x0,y0(上边),x1,y1(下边)
            /// </summary>
            float4  _rect;
            float4  _background;
            float4  _fill;
            float4  _border;
            /// <summary>
            /// 0~1(必须用 float:GPU push constant 按 4 字节布局读取)
            /// </summary>
            float   _progress;
            /// <summary>
            /// 边框宽度(像素)
            /// </summary>
            float   _borderWidth;
            /// <summary>
            /// 矩形像素尺寸
            /// </summary>
            float2  _size;
        };
    protected:
        bool        ensureFont();
        /// <summary>
        /// 计算进度条像素矩形: x,y,w,h
        /// </summary>
        float4      pixelRect(uint2 size) const;
        /// <summary>
        /// 按当前进度重建百分比文本节点(松开旧工厂引用)
        /// </summary>
        void        rebuildText(CMDPtr cmd,uint2 size,const float4& rect);
        /// <summary>
        /// 移除百分比文本节点
        /// </summary>
        void        releaseText();
    protected:
        Style       _style;
        real        _progress       =   0.0f;
        FEProgress* _source         =   nullptr;
        bool        _visible        =   true;
        Font        _font           =   nullptr;
        Node        _textNode       =   nullptr;
        RFactorys   _textFactorys;
        /// <summary>
        /// 已构建文本对应的百分比 / 矩形 / 视口尺寸(变更时才重建)
        /// </summary>
        int         _textPercent    =   -1;
        float4      _textRect       =   float4(0.0f);
        uint2       _textSize       =   uint2(0);
    };
}
