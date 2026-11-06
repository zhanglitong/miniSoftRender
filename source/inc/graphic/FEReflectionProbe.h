#pragma     once

#include    "FETexture.h"
#include    "FEGImage.h"
#include    "FEViewer.h"
#include    "../FECamera.hpp"
#include    "../node/FENode.hpp"

namespace   FE
{
    class   FEScene;

    /// <summary>
    /// 实时反射探针:把场景渲染进一张 cubemap(6 面各 90° 视锥),
    /// 作为环境贴图交给 PBR 材质使用。
    /// 典型用法(玻璃球):
    ///   1. FEScene::createReflectionProbe(球节点,256)
    ///   2. 球材质 setEnvironmentMap(probe->cube())
    ///   3. 每帧由 FEScene::onFrameRender 在主趟之前调用 capture
    /// 探针相机位于 owner 的世界坐标;近裁剪面取 owner 包围盒外接半径的 1.02 倍,
    /// 把 owner 自身整体裁掉以避免自反射(不能隐藏工厂——同材质的其他对象也在其中)。
    /// </summary>
    class   FE_API  FEReflectionProbe :public FEObject
    {
    public:
        FEReflectionProbe(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEReflectionProbe(const FEReflectionProbe& other)
            :FEObject(other)
            ,_owner(other._owner)
            ,_cube(other._cube)
            ,_faceViews(other._faceViews)
            ,_depthImage(other._depthImage)
            ,_depthView(other._depthView)
            ,_cameras(other._cameras)
            ,_viewer(other._viewer)
            ,_size(other._size)
        {}
        virtual ~FEReflectionProbe();

        /// <summary>
        /// 创建 cubemap 渲染目标、深度目标与 6 个面相机
        /// </summary>
        /// <param name="owner">探针所属节点(捕获时排除自身)</param>
        /// <param name="format">颜色格式(需与场景渲染目标一致)</param>
        /// <param name="size">单面边长</param>
        bool    setup(const Node& owner,FEFormat format,uint size = 256);
        void    destroy();

        /// <summary>
        /// 逐面把场景渲染到 cubemap;会改写相机 UBO,
        /// 调用方(通常是 FEScene)负责随后恢复主相机数据
        /// </summary>
        bool    capture(FEScene& scene,CMDPtr cmd);

        inline  bool    isValid() const
        {
            return  _cube != nullptr && _faceViews.size() == 6 && _viewer != nullptr;
        }
        inline  TexPtr  cube() const
        {
            return  _cube;
        }
        inline  uint    size() const
        {
            return  _size;
        }
        inline  const   Node&   owner() const
        {
            return  _owner;
        }
    protected:
        Node                _owner      =   nullptr;
        TexPtr              _cube       =   nullptr;
        GImgViews           _faceViews;
        GImage              _depthImage =   nullptr;
        GImgView            _depthView  =   nullptr;
        std::vector<Camera> _cameras;
        Viewer              _viewer     =   nullptr;
        uint                _size       =   0;
    };

    using   ReflectionProbe     =   SharedPtr<FEReflectionProbe>;
    using   ReflectionProbes    =   std::vector<ReflectionProbe>;
}
