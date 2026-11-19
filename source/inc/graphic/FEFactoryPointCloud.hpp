#pragma     once

#include    "FEFactoryRender.hpp"

namespace   FE
{
    /// <summary>
    /// 逐节点网格工厂:每个节点(带 mesh)持有自己的顶点/索引缓冲,渲染时逐节点
    /// 绑定并绘制一次,不做"全量交错重建"。
    ///
    /// 适用场景(点云 LOD 等):节点几何互不相同、运行期频繁增删。
    /// 基类工厂把组内所有 mesh 打进一份大 VBO,任何增删都要整体重算并重传;
    /// 本工厂把顶点数据按 mesh 缓存,增删只影响该 mesh 自身的缓冲,O(1)。
    /// 代价是绘制次数从"每组 1 次间接绘制"变为"每节点 1 次",
    /// 因此关闭 GPU 裁剪(裁剪命令与逐节点绑定无法对应)。
    /// </summary>
    class   FE_API  FEFactoryPointCloud :public FEFactoryRender
    {
    public:
        /// <summary>
        /// 单个 mesh 的 GPU 资源
        /// </summary>
        struct  GpuMesh
        {
            /// <summary>
            /// 顶点绑定(按管线 binding 交错后的独立缓冲)
            /// </summary>
            VBinds  _vbinds;
            /// <summary>
            /// 索引缓冲(非索引绘制为 nullptr)
            /// </summary>
            IBO     _ibo;
        };
        using   GpuMeshs    =   std::unordered_map<Mesh,GpuMesh>;
    public:
        FEFactoryPointCloud(FEContext& ctx);
        virtual ~FEFactoryPointCloud();
    public:
        virtual void    update(CMDPtr cmd) override;
        virtual void    render(CMDPtr cmd,const FERenderPassInfo& pass = FERenderPassInfo()) override;
    protected:
        virtual void    onDestroy() override;
        /// <summary>
        /// 按当前组内节点同步 mesh 缓冲缓存:新增 mesh 建缓冲,不再被引用的释放
        /// </summary>
        void            syncMeshes();
    protected:
        /// <summary>
        /// 每 mesh 独立的顶点/索引缓冲
        /// </summary>
        GpuMeshs        _meshes;
    };
}
