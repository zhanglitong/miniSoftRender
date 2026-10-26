#include    "../inc/graphic/FEOffscreenRenderer.h"
#include    "../FEImageHelper.hpp"

#include    <cstring>

namespace   FE
{
    FEOffscreenRenderer::FEOffscreenRenderer(FEContext& ctx)
        :FEObject(ctx)
        ,_ctx(ctx)
    {}

    FEOffscreenRenderer::FEOffscreenRenderer(const FEOffscreenRenderer& other)
        :FEObject(other)
        ,_ctx(other._ctx)
        ,_scene(other._scene)
        ,_width(other._width)
        ,_height(other._height)
        ,_pixels(other._pixels)
    {}

    FEOffscreenRenderer::~FEOffscreenRenderer()
    {
        destroy();
    }

    bool    FEOffscreenRenderer::create(const CreateInfo& info)
    {
        if (info._width == 0 || info._height == 0)
            return  false;

        if (!info._workPath.empty())
            _ctx.setWorkPath(info._workPath);
        if (!info._resourcePath.empty())
            _ctx.setResourcePath(info._resourcePath);

        /// 重复创建时先释放旧场景
        destroy();

        _scene  =   new FEScene(_ctx);
        /// 采样数必须在 setup 之前指定(setup 内按设备能力钳制)
        _scene->setSampleCount(info._samples);
        if (!_scene->setupHeadless(info._width,info._height,info._rendererId))
        {
            _scene  =   nullptr;
            return  false;
        }
        /// 工具类默认不显示编辑器网格,避免干扰像素结果
        _scene->showGrid(false);

        _width  =   info._width;
        _height =   info._height;
        resetPixels();
        return  true;
    }

    void    FEOffscreenRenderer::destroy()
    {
        if (_scene != nullptr)
        {
            _scene->destroy();
            _scene  =   nullptr;
        }
        _pixels.clear();
    }

    bool    FEOffscreenRenderer::render(uint frameCount)
    {
        if (_scene == nullptr || _pixels.empty())
            return  false;
        for (uint i = 0; i < frameCount; ++i)
            _scene->renderFrame();
        return  _scene->readPixels(_pixels.data(),_pixels.size());
    }

    bool    FEOffscreenRenderer::resize(uint width,uint height)
    {
        if (_scene == nullptr || width == 0 || height == 0)
            return  false;
        _scene->resize(width,height);
        _width  =   width;
        _height =   height;
        resetPixels();
        return  true;
    }

    ImagePtr    FEOffscreenRenderer::toImage()
    {
        if (_pixels.empty())
            return  nullptr;

        ImagePtr    image   =   new FEImage(_ctx,_width,_height,FMT_R8G8B8A8_UNORM);
        auto        plane   =   image->plane(0,0);
        if (plane.data() == nullptr)
            return  nullptr;

        /// 回读像素为紧密排布,逐行拷贝以兼容 FEImage 的行对齐
        const size_t    srcPitch    =   size_t(_width) * 4;
        for (uint y = 0; y < _height; ++y)
            std::memcpy(plane.rowAt(y),_pixels.data() + size_t(y) * srcPitch,srcPitch);
        return  image;
    }

    bool    FEOffscreenRenderer::save(const char* fileName,int quality)
    {
        if (fileName == nullptr || _pixels.empty())
            return  false;
        ImagePtr    image   =   toImage();
        if (image == nullptr)
            return  false;
        return  FEImageHelper::save(*image,fileName,quality);
    }

    void    FEOffscreenRenderer::resetPixels()
    {
        _pixels.assign(size_t(_width) * size_t(_height) * 4,0);
    }
}
