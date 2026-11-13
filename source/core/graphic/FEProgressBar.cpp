#include    "../inc/graphic/FEProgressBar.hpp"

#include    "../inc/FEContext.hpp"
#include    "../inc/graphic/FECmdBuffer.h"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/material/FEMaterialText.hpp"

#include    <cmath>
#include    <filesystem>

namespace   FE
{
    FEProgressBar::FEProgressBar(FEContext& ctx)
        :FEFactoryRender(ctx)
    {
        /// 叠加层:在场景几何之后绘制,不参与场景层排序
        _usages =   ViewerUsage::USAGE_Overlay;
        /// 更新/绘制都排到最后(注意必须写 _prioritys:priority() 返回的是副本)
        _prioritys[FEFactory::PT_Update].setPriority(EP_Last);
        _prioritys[FEFactory::PT_Update].setOrder(MaxInt16);
        _prioritys[FEFactory::PT_Render].setPriority(EP_Last);
        _prioritys[FEFactory::PT_Render].setOrder(MaxInt16);
        /// 常驻:节点树清空时不被连带清理
        setResident(true);
    }
    FEProgressBar::~FEProgressBar()
    {
        /// 不做 removeNodesFromFactory:场景销毁时其工厂表可能已先于本对象清空
        _textNode       =   nullptr;
        _textFactorys.clear();
    }
    void    FEProgressBar::setProgress(real val)
    {
        _progress   =   std::clamp(val,real(0.0),real(1.0));
    }
    real    FEProgressBar::progress() const
    {
        return  _progress;
    }
    void    FEProgressBar::bindProgress(FEProgress* source)
    {
        _source =   source;
    }
    void    FEProgressBar::setStyle(const Style& style)
    {
        _style          =   style;
        /// 强制下一帧按新样式重建文本
        _textPercent    =   -1;
    }
    const   FEProgressBar::Style&   FEProgressBar::style() const
    {
        return  _style;
    }
    void    FEProgressBar::setVisible(bool show)
    {
        _visible    =   show;
    }
    bool    FEProgressBar::visible() const
    {
        return  _visible;
    }
    void    FEProgressBar::setFont(Font font)
    {
        _font           =   font;
        _textPercent    =   -1;
    }
    const   Font&   FEProgressBar::font() const
    {
        return  _font;
    }
    bool    FEProgressBar::ensureFont()
    {
        if (_font != nullptr && _font->isValid())
            return  true;
        if (_font == nullptr)
            _font   =   new FEFont(_ctx);
        FEFontCreateInfo    cInfo   =   {};
        cInfo._pixelSize    =   64;
        cInfo._spread       =   8;
        cInfo._atlasWidth   =   256;
        cInfo._characters   =   "0123456789%";
        /// 资源根目录:窗口应用为 exe 的上层(bin),离屏为 CreateInfo._resourcePath
        const   std::filesystem::path   fontPath    =
            std::filesystem::path(_ctx.resourcePath()) / "data" / "fonts" / "consola.ttf";
        return  _font->loadFromFile(fontPath.string(),cInfo);
    }
    float4  FEProgressBar::pixelRect(uint2 size) const
    {
        const   real    screenW =   real(size.x);
        const   real    screenH =   real(size.y);
        real    w   =   screenW * std::clamp(real(_style._widthScale),real(0.05),real(1.0));
        real    h   =   (std::max)(real(_style._height),real(1.0));
        /// 高度加上下边距不能超过屏幕,保证小窗口下仍可见
        h   =   (std::min)(h,screenH);
        w   =   (std::min)(w,screenW);
        const   real    x   =   (screenW - w) * real(0.5);
        const   real    y   =   (std::max)(screenH - real(_style._bottomMargin) - h,real(0.0));
        return  float4(x,y,w,h);
    }
    void    FEProgressBar::releaseText()
    {
        FEScene*    scene   =   _ctx.scene();
        if (scene != nullptr && _textNode != nullptr)
            FEFactoryRender::removeNodesFromFactory(*scene,{_textNode});
        _textNode   =   nullptr;
        _textFactorys.clear();
    }
    void    FEProgressBar::rebuildText(CMDPtr cmd,uint2 size,const float4& rect)
    {
        FEScene*    scene   =   _ctx.scene();
        if (scene == nullptr)
            return;
        releaseText();
        if (!ensureFont())
            return;
        const   String  text    =   std::to_string(_textPercent) + "%";
        const   real    scale   =   real(_style._textSize) / real(64.0);
        const   real    textW   =   _font->measureWidth(text,float(scale));
        const   real    textH   =   _font->lineHeight() * float(scale);
        const   float2  origin  =   float2(float(rect.x + (rect.z - textW) * real(0.5)),
                                          float(rect.y - real(_style._textGap) - textH));
        Mesh    mesh    =   _font->createTextMesh(text,origin,float(scale));
        if (mesh == nullptr)
            return;
        /// 屏幕叠加模式:离屏渲染必须显式告知渲染目标尺寸(窗口尺寸不可用)
        FEMaterialText* mat =   new FEMaterialText(_ctx);
        mat->setFont(_font);
        mat->setScreenSize(size.x,size.y);
        mat->setStrokeWidth(2.0f);
        mat->setStrokeColor(Rgba8(0,0,0,255));

        Node    node    =   new FENode(_ctx);
        node->setMesh(mesh);
        node->setMaterial(mat);
        node->setColor(_style._textColor);
        node->makeDirty();
        node->update();

        DispatchResult  dr;
        scene->dispatchNodesToSystem({node},&dr);
        for (auto& factory : dr.rFactorys)
        {
            /// 常驻 + 关闭 GPU 剔除:文本节点不在节点树里,每帧位置由本类驱动
            factory->setResident(true);
            factory->setGPUCull(false);
            /// 工厂是在本帧工厂更新循环之后新建的,本帧结束时其"新增节点"
            /// 标记会被场景统一清除,导致下一帧不会构建 VBO(文本永远不绘制)。
            /// 这里立即构建一次,避免错过唯一的标记窗口
            if (cmd != nullptr)
                factory->update(cmd);
        }
        _textNode       =   node;
        _textFactorys   =   dr.rFactorys;
    }
    void    FEProgressBar::update(CMDPtr cmd)
    {
        if (!_visible)
        {
            /// 隐藏时文本节点是独立工厂(常驻),不会随本对象停止绘制,
            /// 必须显式移除,否则百分比会一直残留在屏幕上
            if (_textNode != nullptr)
                releaseText();
            return;
        }
        /// 轮询外部进度源(如 FETask / 文件 reader)
        if (_source != nullptr)
            _progress   =   std::clamp(_source->progress(),real(0.0),real(1.0));
        FEScene*    scene   =   _ctx.scene();
        if (scene == nullptr)
            return;
        const   uint2   size    =   scene->renderSize();
        if (size.x == 0 || size.y == 0)
            return;
        if (!_style._showText)
        {
            if (_textNode != nullptr)
                releaseText();
            return;
        }
        const   float4  rect    =   pixelRect(size);
        const   int     percent =   int(std::lround(_progress * 100.0f));
        const   bool    same    =   percent == _textPercent
                                &&  size.x == _textSize.x && size.y == _textSize.y
                                &&  std::abs(rect.x - _textRect.x) < 0.5f
                                &&  std::abs(rect.y - _textRect.y) < 0.5f
                                &&  std::abs(rect.z - _textRect.z) < 0.5f
                                &&  std::abs(rect.w - _textRect.w) < 0.5f;
        if (same)
            return;
        _textPercent    =   percent;
        _textRect       =   rect;
        _textSize       =   size;
        rebuildText(cmd,size,rect);
    }
    void    FEProgressBar::render(CMDPtr cmd,const FERenderPassInfo& pass)
    {
        if (!_visible || cmd == nullptr)
            return;
        /// 折射预渲染与 OIT 收集趟由场景在趟描述里标记为不叠 UI,
        /// 避免进度条被烘进折射目标/OIT 缓冲
        if (!pass._drawUi)
            return;
        FEScene*    scene   =   _ctx.scene();
        if (scene == nullptr)
            return;
        const   uint2   size    =   scene->renderSize();
        if (size.x == 0 || size.y == 0)
            return;
        Pipeline    pl;
        if (!_ctx.device().pipelines().query("progress/TRIANGLES",pl) || pl == nullptr)
            return;
        const   float4  r   =   pixelRect(size);
        /// 像素 -> NDC,x 向右 y 向上;y0 为矩形上边
        const   real    ndcX0   =   (r.x / real(size.x)) * 2.0f - 1.0f;
        const   real    ndcX1   =   ((r.x + r.z) / real(size.x)) * 2.0f - 1.0f;
        const   real    ndcY0   =   1.0f - (r.y / real(size.y)) * 2.0f;
        const   real    ndcY1   =   1.0f - ((r.y + r.w) / real(size.y)) * 2.0f;

        PushData    push    =   {};
        push._rect          =   float4(ndcX0,ndcY0,ndcX1,ndcY1);
        push._background    =   _style._background;
        push._fill          =   _style._fill;
        push._border        =   _style._border;
        push._progress      =   float(_progress);
        push._borderWidth   =   (std::max)(_style._borderWidth,0.0f);
        push._size          =   float2(r.z,r.w);

        cmd->bindPipeline(pl);
        cmd->pushConstants(pl,pl->cInfo()._pushConstantStage.data(),0,sizeof(push),&push);
        cmd->draw(0,6,0,1);
    }
}
