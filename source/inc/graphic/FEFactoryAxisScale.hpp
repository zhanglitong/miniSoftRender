#pragma     once

#include    "../axis/FENodeScaleEditor.h"
#include    "../material/FEMaterialV3.hpp"
#include    "FEFactoryRender.hpp"


namespace   FE
{
    /// <summary>
    /// 缩放编辑轴绘制工厂
    /// 负责把 FEEditAxisScale 生成的顶点数据上传到 GPU 并绘制
    /// 绘制内容包含:
    ///   1. 三条坐标轴线 (X/Y/Z)
    ///   2. 中心 XYZ 三角面 (统一缩放拾取面)
    ///   3. 三个外侧四边形面 (XY/YZ/XZ 双轴缩放拾取面)
    ///   4. 三个箭头立方体 (轴末端)
    /// </summary>
    class   FEFactoryAxisScale :public FEFactoryRender
    {
    public:
        FEFactoryAxisScale(FEContext& ctx)
            :FEFactoryRender(ctx)
        {
            _scale  =   new FENodeScaleEditor(_ctx);
            _mat    =   new FEMaterialV3(_ctx);
        }
        FEFactoryAxisScale(const FEFactoryAxisScale& other)
            :FEFactoryRender(other)
            ,_scale(other._scale)
        {}
        inline  auto    inputComponent() const
        {
            return  _scale;
        }
        inline  auto    inputComponent()
        {
            return  _scale;
        }
    public:
        /// <summary>
        /// 每一帧调用,更新顶点数据并上传到 GPU
        /// </summary>
        virtual void    update(CMDPtr ) override
        {
            _scale->update(_ctx.activeCamera());

            auto&   scaleAxis       =   _scale->scaleAxis();        ///16
            auto&   scaleAxisArr    =   _scale->scaleAxisArr();     ///15

            /// ----------------------------------------------------------------
            /// 1. 线顶点 (PRI_LINES)
            ///    三条坐标轴线: 中心 -> 轴末端 (6 顶点)
            ///    中心 XYZ 三角面轮廓: 1-6, 6-11, 11-1 (6 顶点)
            ///    XY 面轮廓: 1-3, 3-8, 8-6, 6-1 (8 顶点)
            ///    YZ 面轮廓: 6-8, 8-13, 13-11, 11-6 (8 顶点)
            ///    XZ 面轮廓: 11-13, 13-3, 3-1, 1-11 (8 顶点)
            ///    共 36 顶点
            /// ----------------------------------------------------------------
            const   size_t  kLineVertCount   =   36;
            const   size_t  kLineByteLen     =   kLineVertCount * sizeof(float3);

            /// ----------------------------------------------------------------
            /// 2. 三角形顶点 (PRI_TRIANGLE_FAN)
            ///    [0..3]   中心 XYZ 三角面  (0, 1, 6, 11)
            ///    [4..7]   外侧 XY 四边形   (1, 3, 8, 6)
            ///    [8..11]  外侧 YZ 四边形   (6, 8, 13, 11)
            ///    [12..15] 外侧 XZ 四边形   (11, 13, 3, 1)
            ///    共 16 顶点
            /// ----------------------------------------------------------------
            const   size_t  kTriVertCount    =   16;
            const   size_t  kTriByteLen      =   kTriVertCount * sizeof(float3);

            /// ----------------------------------------------------------------
            /// 3. 箭头顶点 (PRI_TRIANGLE_FAN)
            ///    三个轴末端立方体,每个 5 顶点 (1 中心 + 4 角)
            ///    共 15 顶点
            /// ----------------------------------------------------------------
            const   size_t  kArrowVertCount =   15;
            const   size_t  kArrowByteLen   =   kArrowVertCount * sizeof(float3);

            const   size_t  totalLen    =   kLineByteLen + kTriByteLen + kArrowByteLen;

            /// ----------------------------------------------------------------
            /// 创建/复用 CPU 暂存缓冲
            /// ----------------------------------------------------------------
            if (_cpu == nullptr)
            {
                _cpu    =   _ctx.device().createVBO();
                _cpu->create({totalLen,HOST_VISIBLE_BIT});
            }
            else if (_cpu->cInfo()._length < totalLen)
            {
                _cpu->create({totalLen,HOST_VISIBLE_BIT});
            }

            uint8*  pDst    =   (uint8*)_cpu->lock(totalLen,0);
            if (pDst == nullptr)
                return;

            size_t  off     =   0;

            ///--- 三条坐标轴线
            ///X: 0 -> 5
            memcpy(pDst + off, &scaleAxis[0], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[5], sizeof(float3));  off += sizeof(float3);
            ///Y: 0 -> 10
            memcpy(pDst + off, &scaleAxis[0],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[10], sizeof(float3));  off += sizeof(float3);
            ///Z: 0 -> 15
            memcpy(pDst + off, &scaleAxis[0],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[15], sizeof(float3));  off += sizeof(float3);

            ///--- 中心 XYZ 三角面轮廓 (1-6, 6-11, 11-1)
            memcpy(pDst + off, &scaleAxis[1],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[1],  sizeof(float3));  off += sizeof(float3);

            ///--- XY 面轮廓 (1-3, 3-8, 8-6, 6-1)
            memcpy(pDst + off, &scaleAxis[1], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[3], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[3], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[8], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[8], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[1], sizeof(float3));  off += sizeof(float3);

            ///--- YZ 面轮廓 (6-8, 8-13, 13-11, 11-6)
            memcpy(pDst + off, &scaleAxis[6],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[8],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[8],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[13], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[13], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6],  sizeof(float3));  off += sizeof(float3);

            ///--- XZ 面轮廓 (11-13, 13-3, 3-1, 1-11)
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[13], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[13], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[3],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[3],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[1],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[1],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);

            ///--- 中心 XYZ 三角面 (TRIANGLE_FAN, 4 顶点)
            memcpy(pDst + off, &scaleAxis[0],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[1],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);

            ///--- 外侧 XY 四边形 (TRIANGLE_FAN, 4 顶点)
            memcpy(pDst + off, &scaleAxis[1], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[3], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[8], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[6], sizeof(float3));  off += sizeof(float3);

            ///--- 外侧 YZ 四边形 (TRIANGLE_FAN, 4 顶点)
            memcpy(pDst + off, &scaleAxis[6],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[8],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[13], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);

            ///--- 外侧 XZ 四边形 (TRIANGLE_FAN, 4 顶点)
            memcpy(pDst + off, &scaleAxis[11], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[13], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[3],  sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &scaleAxis[1],  sizeof(float3));  off += sizeof(float3);

            ///--- 三个箭头立方体 (直接拷贝 _scaleAxisArr[15])
            memcpy(pDst + off, scaleAxisArr, kArrowByteLen);
            off  +=  kArrowByteLen;

            _cpu->unlock();

            /// ----------------------------------------------------------------
            /// 创建/复用 GPU VBO
            /// ----------------------------------------------------------------
            if (_axisLineVBO == nullptr || _axisLineVBO->cInfo()._length < kLineByteLen)
            {
                _axisLineVBO    =   _ctx.device().createVBO();
                _axisLineVBO->create({kLineByteLen,DEVICE_LOCAL_BIT});
            }
            if (_axisTriVBO == nullptr || _axisTriVBO->cInfo()._length < kTriByteLen)
            {
                _axisTriVBO    =   _ctx.device().createVBO();
                _axisTriVBO->create({kTriByteLen,DEVICE_LOCAL_BIT});
            }
            if (_axisArrowVBO == nullptr || _axisArrowVBO->cInfo()._length < kArrowByteLen)
            {
                _axisArrowVBO    =   _ctx.device().createVBO();
                _axisArrowVBO->create({kArrowByteLen,DEVICE_LOCAL_BIT});
            }

            auto    cmdPool =   _device.transferCmdPool();
            assert(cmdPool != nullptr);
            if (cmdPool != nullptr)
            {
                CMDPtr  cmd =   cmdPool->createCmd();
                cmd->begin(true);
                cmd->copyBuffer(_cpu, _axisLineVBO,  kLineByteLen,  0,                       0);
                cmd->copyBuffer(_cpu, _axisTriVBO,   kTriByteLen,   kLineByteLen,            0);
                cmd->copyBuffer(_cpu, _axisArrowVBO, kArrowByteLen, kLineByteLen + kTriByteLen, 0);
                cmd->end();
                cmd->submit(_device.queueTransfer());
            }
        }
        /// <summary>
        /// 每一帧调用,绘制缩放编辑轴
        /// </summary>
        virtual void    render(CMDPtr cmd) override
        {
            if (_mat == nullptr || _axisLineVBO == nullptr)
                return;

            FECmdBuffer::Viewport   viewPort    =
            {
                0.0f,0.0f,(float)_ctx.windowsWidth(),(float)_ctx.windowsHeight(),0.0f,1.0f
            };
            RectU32     rect(0,0,_ctx.windowsWidth(),_ctx.windowsHeight());
            cmd->setViewport(0,  1,  &viewPort);
            cmd->setScissor(0,   1,  &rect);
            cmd->setCullMode(CullMode::CM_NULL);

            mat4r   tMat    =   FE::translate(mat4r(),_scale->position());
            mat4r   mvp     =   _ctx.mvp() * tMat;

            /// ----------------------------------------------------------------
            /// 1. 三条坐标轴线 (PRI_LINES)
            /// ----------------------------------------------------------------
            auto    pl  =   _mat->pipeline(PRI_LINES)->as<FEGPipeline>();
            cmd->bindPipeline(pl);
            _mat->appDynamicState(cmd,PRI_LINES);
            cmd->bindVBO(0,_axisLineVBO,0);
            cmd->setPrimitiveTopology(PRI_LINES);

            for (int i = 0; i < 3; ++i)
            {
                float4  color   =   { 0.0f, 0.0f, 0.0f, 1.0f };
                color[i]    =   1.0f;
                if (_scale->hoveredAxis() == i + 1)
                {
                    color[0]    =   1.0f;
                    color[1]    =   1.0f;
                    cmd->setLineWidth(4);
                }
                else
                {
                    cmd->setLineWidth(2);
                }
                PointData   pushBlock;
                pushBlock._point    =   0;
                pushBlock._mvp      =   mvp;
                pushBlock._color    =   _scale->enabled(i + 1)
                                       ? FE::packUnorm4x8(color)
                                       : FE::packUnorm4x8(DisableColor);
                cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                cmd->draw(2 * i, 2, 0, 1);
            }

            /// ----------------------------------------------------------------
            /// 1b. 面轮廓线 (PRI_LINES)
            ///    中心 XYZ 三角面: 顶点 6~11
            ///    XY 面: 顶点 12~19
            ///    YZ 面: 顶点 20~27
            ///    XZ 面: 顶点 28~35
            /// ----------------------------------------------------------------
            {
                struct  FaceLine
                {
                    int     _vertStart;
                    int     _vertCount;
                    float4  _color;
                    FEEditAxisScale::AXIS  _hoverEnum;
                };
                FaceLine faceLines[4]  =
                {
                    { 6,  6, { 1.0f, 1.0f, 1.0f, 0.6f }, FEEditAxisScale::AXIS_XYZ },  /// XYZ 面-白
                    { 12, 8, { 0.0f, 0.0f, 1.0f, 0.6f }, FEEditAxisScale::AXIS_XY  },  /// XY 面-蓝
                    { 20, 8, { 1.0f, 0.0f, 0.0f, 0.6f }, FEEditAxisScale::AXIS_YZ  },  /// YZ 面-红
                    { 28, 8, { 0.0f, 1.0f, 0.0f, 0.6f }, FEEditAxisScale::AXIS_XZ  },  /// XZ 面-绿
                };
                cmd->setLineWidth(2);
                for (int i = 0; i < 4; ++i)
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   faceLines[i]._color;
                    if (_scale->hoveredAxis() == faceLines[i]._hoverEnum)
                    {
                        color[0]    =   1.0f;
                        color[1]    =   1.0f;
                        color[2]    =   0.0f;
                        color[3]    =   1.0f;
                        cmd->setLineWidth(2);
                    }
                    else
                    {
                        cmd->setLineWidth(2);
                    }
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(faceLines[i]._vertStart, faceLines[i]._vertCount, 0, 1);
                }
            }

            /// ----------------------------------------------------------------
            /// 2. 中心 XYZ 三角面 + 外侧三 Quad (PRI_TRIANGLE_FAN)
            /// ----------------------------------------------------------------
            if (_axisTriVBO != nullptr)
            {
                auto    plFan   =   _mat->pipeline(PRI_TRIANGLE_FAN)->as<FEGPipeline>();
                cmd->bindPipeline(plFan);
                _mat->appDynamicState(cmd,PRI_TRIANGLE_FAN);
                cmd->bindVBO(0,_axisTriVBO,0);
                cmd->setPrimitiveTopology(PRI_TRIANGLE_FAN);

                ///中心 XYZ 三角面 (仅当 XYZ 被高亮时绘制)
                ///enum AXIS { AXIS_NULL=0, AXIS_X=1, AXIS_Y=2, AXIS_Z=3, AXIS_XY=4, AXIS_YZ=5, AXIS_XZ=6, AXIS_XYZ=7 };
                if (_scale->hoveredAxis() == FEEditAxisScale::AXIS_XYZ)
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 1.0f, 1.0f, 0.0f, 0.6f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(plFan, plFan->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(0, 4, 0, 1);
                }
                ///外侧 XY 面 (仅当 XY 被高亮时绘制)
                if (_scale->hoveredAxis() == FEEditAxisScale::AXIS_XY)
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 1.0f, 1.0f, 0.0f, 0.6f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(plFan, plFan->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(4, 4, 0, 1);
                }
                ///外侧 YZ 面 (仅当 YZ 被高亮时绘制)
                if (_scale->hoveredAxis() == FEEditAxisScale::AXIS_YZ)
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 1.0f, 1.0f, 0.0f, 0.6f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(plFan, plFan->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(8, 4, 0, 1);
                }
                ///外侧 XZ 面 (仅当 XZ 被高亮时绘制)
                if (_scale->hoveredAxis() == FEEditAxisScale::AXIS_XZ)
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 1.0f, 1.0f, 0.0f, 0.6f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(plFan, plFan->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(12, 4, 0, 1);
                }
            }

            /// ----------------------------------------------------------------
            /// 3. 三个箭头立方体 (PRI_TRIANGLE_FAN)
            /// ----------------------------------------------------------------
            if (_axisArrowVBO != nullptr)
            {
                auto    plFan   =   _mat->pipeline(PRI_TRIANGLE_FAN)->as<FEGPipeline>();
                cmd->bindPipeline(plFan);
                _mat->appDynamicState(cmd,PRI_TRIANGLE_FAN);
                cmd->bindVBO(0,_axisArrowVBO,0);
                cmd->setPrimitiveTopology(PRI_TRIANGLE_FAN);

                for (int i = 0; i < 3; ++i)
                {
                    float4  color   =   { 0.0f, 0.0f, 0.0f, 1.0f };
                    color[i]    =   1.0f;
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    pushBlock._color    =   _scale->enabled(i + 1)
                                           ? FE::packUnorm4x8(color)
                                           : FE::packUnorm4x8(DisableColor);
                    cmd->pushConstants(plFan, plFan->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(5 * i, 5, 0, 1);
                }
            }
        }
        /// <summary>
        /// 销毁前一定调用该函数,解除数据对工厂的引用计数
        /// </summary>
        virtual void    destroy() override
        {}
    protected:
        VBO             _axisLineVBO    =   nullptr;
        VBO             _axisTriVBO     =   nullptr;
        VBO             _axisArrowVBO   =   nullptr;
        VBO             _cpu            =   nullptr;
        NodeScaleEditor _scale          =   nullptr;
        MaterialV3      _mat            =   nullptr;
    };
    using   FactoryScale     =   SharedPtr<FEFactoryAxisScale>;
}
