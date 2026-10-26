#pragma     once

#include    "../FEObject.h"
#include    "../FEImage.h"
#include    "FEScene.h"

namespace   FE
{
    /// <summary>
    /// 离屏渲染工具类:内部持有一个无头 FEScene,负责尺寸管理、帧驱动、
    /// 像素回读与图片保存。
    /// 典型用法:
    ///   OffscreenRenderer renderer = new FEOffscreenRenderer(ctx);
    ///   renderer->create({256,256});
    ///   renderer->scene()->dispatchNodesToSystem({node});
    ///   renderer->render(8);
    ///   renderer->save("output.png");
    /// </summary>
    class   FE_API  FEOffscreenRenderer   :public FEObject
    {
    public:
        struct  CreateInfo
        {
            uint        _width      =   0;
            uint        _height     =   0;
            FEUuid      _rendererId =   RS_VULKAN;
            /// <summary>
            /// 多采样数(MSAA):1=单采样。回读像素始终来自解析后的单采样目标
            /// </summary>
            uint        _samples    =   1;
            /// <summary>
            /// 为空时沿用 FEContext 已设置的路径
            /// </summary>
            String      _workPath;
            String      _resourcePath;
        };
    public:
        FEOffscreenRenderer(FEContext& ctx);

        FEOffscreenRenderer(const FEOffscreenRenderer& other);

        virtual ~FEOffscreenRenderer();
    public:
        /// <summary>
        /// 创建离屏场景(无窗口),默认隐藏编辑网格
        /// </summary>
        bool        create(const CreateInfo& info);
        /// <summary>
        /// 销毁离屏场景(析构时自动调用)
        /// </summary>
        void        destroy();
        /// <summary>
        /// 内部场景,用于挂接节点/材质/相机/清屏色等
        /// </summary>
        inline  Scene   scene() const
        {
            return  _scene;
        }
        /// <summary>
        /// 渲染 frameCount 帧,最后一帧结束后回读 RGBA8 像素
        /// </summary>
        bool        render(uint frameCount = 1);
        /// <summary>
        /// 修改渲染尺寸(重建离屏目标与像素缓冲)
        /// </summary>
        bool        resize(uint width,uint height);

        inline  uint        width()  const  {   return  _width;    }
        inline  uint        height() const  {   return  _height;   }
        /// <summary>
        /// 最近一次 render 回读的 RGBA8 像素(紧密排布,第0行在图像顶部)
        /// </summary>
        inline  const   uint8s&     pixels() const  {   return  _pixels;   }
        /// <summary>
        /// 转成 FEImage(供图像处理或其它保存接口使用)
        /// </summary>
        ImagePtr            toImage();
        /// <summary>
        /// 保存图片,格式由扩展名决定(png/jpg/bmp/tga/ktx/ktx2)
        /// </summary>
        bool                save(const char* fileName,int quality = 90);
    protected:
        /// <summary>
        /// 按当前尺寸重置像素缓冲
        /// </summary>
        void        resetPixels();
    protected:
        FEContext&          _ctx;
        Scene               _scene  =   nullptr;
        uint                _width  =   0;
        uint                _height =   0;
        uint8s              _pixels;
    };

    using   OffscreenRenderer   =   SharedPtr<FEOffscreenRenderer>;
}
