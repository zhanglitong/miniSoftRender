#pragma     once

#include    "FETexture.h"
#include    "FEGImage.h"
#include    "FEFactoryRender.hpp"
#include    "../FECamera.hpp"
#include    "../node/FENode.hpp"

namespace   FE
{
    class   FEScene;

    /// <summary>
    /// 天空环境烘焙:把程序化天空(大气散射 + 云)渲染进一张带 mip 链的 cubemap,
    /// 交给 PBR 的 IBL 直接采样,替代每个像素重复求值 Preetham 大气 + FBM 云。
    ///
    /// 关键约定:
    ///   1. 使用 skyenv 管线(片元着色器输出"线性 HDR 辐亮度"),不做色调映射与
    ///      sRGB 编码,数值与 PBR 原解析分支(skyDiffuseRadiance/skySpecularRadiance
    ///      × 曝光 × 天空开关)同量级,因此切换后亮度表现一致;
    ///   2. 烘焙相机位于原点、逐面 90° FOV,与 FEReflectionProbe 使用同一套面朝向;
    ///   3. 6 面 × 每级 mip 各自单独渲染(128 起,含完整 mip 链,总计约 13 万像素),
    ///      避免对 cubemap 做 vkCmdBlitImage 生成 mip 的兼容性问题;
    ///   4. 烘焙会改写相机 UBO,调用方(FEScene)负责随后恢复主相机数据。
    /// </summary>
    class   FE_API  FESkyEnvironment :public FEObject
    {
    public:
        FESkyEnvironment(FEContext& ctx)
            :FEObject(ctx)
        {}
        FESkyEnvironment(const FESkyEnvironment& other)
            :FEObject(other)
            ,_cube(other._cube)
            ,_faceViews(other._faceViews)
            ,_depthImage(other._depthImage)
            ,_depthView(other._depthView)
            ,_cameras(other._cameras)
            ,_node(other._node)
            ,_material(other._material)
            ,_factory(other._factory)
            ,_size(other._size)
            ,_mips(other._mips)
        {}
        virtual ~FESkyEnvironment();

        /// <summary>
        /// 创建烘焙 cubemap(含 mip 链)、深度目标、6 个面相机与烘焙用渲染工厂
        /// </summary>
        /// <param name="scene">用于创建不加入场景的专用渲染工厂</param>
        /// <param name="size">cubemap 单面边长(mip0)</param>
        bool    setup(FEScene& scene,uint size = 128);
        void    destroy();

        /// <summary>
        /// 逐 mip、逐面把天空渲染到 cubemap;会改写相机 UBO,
        /// 调用方负责随后恢复主相机数据
        /// </summary>
        bool    bake(FEScene& scene,CMDPtr cmd);

        inline  bool    isValid() const
        {
            return  _cube != nullptr && _factory != nullptr && _mips > 0;
        }
        inline  TexPtr  cube() const
        {
            return  _cube;
        }
        inline  uint    size() const
        {
            return  _size;
        }
        inline  uint    mips() const
        {
            return  _mips;
        }
    protected:
        TexPtr                  _cube       =   nullptr;
        /// [mip][face] 每级 mip 的单面视图(作为 color attachment)
        std::vector<GImgViews>  _faceViews;
        GImage                  _depthImage =   nullptr;
        GImgView                _depthView  =   nullptr;
        std::vector<Camera>     _cameras;
        Node                    _node       =   nullptr;
        Material                _material   =   nullptr;
        RFactory                _factory    =   nullptr;
        uint                    _size       =   0;
        uint                    _mips       =   0;
    };

    using   SkyEnvironment  =   SharedPtr<FESkyEnvironment>;
}
