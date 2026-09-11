#pragma     once

#include    "../axis/FENodeRotateEditor.h"
#include    "../material/FEMaterialV3.hpp"
#include    "FEFactoryRender.hpp"


namespace   FE
{
    /// <summary>
    /// 旋转编辑轴绘制工厂
    /// 负责把 FEEditAxisRotate 生成的顶点数据上传到 GPU 并绘制
    /// 绘制内容包含:
    ///   1. 三条坐标轴线 (X/Y/Z)
    ///   2. 与屏幕平行的大圆以及球面上的圆
    ///   3. 三条圆弧 (X/Y/Z)
    ///   4. 旋转中: 切线 + 扇形
    /// </summary>
    class   FEFactoryAxisRotate :public FEFactoryRender
    {
    public:
        FEFactoryAxisRotate(FEContext& ctx)
            :FEFactoryRender(ctx)
        {
            _rotate    =   new FENodeRotateEditor(_ctx);
            _mat       =   new FEMaterialV3(_ctx);
        }
        FEFactoryAxisRotate(const FEFactoryAxisRotate& other)
            :FEFactoryRender(other)
            ,_rotate(other._rotate)
        {}
        inline  auto    inputComponent() const
        {
            return  _rotate;
        }
        inline  auto    inputComponent()
        {
            return  _rotate;
        }
    public:
        /// <summary>
        /// 每一帧调用,更新顶点数据并上传到 GPU
        /// </summary>
        virtual void    update(CMDPtr ) override
        {
            if (_rotate == nullptr || !flags().hasFlag(FE::FLAG_VISIBLE))
                return;

            _rotate->update(_ctx.activeCamera());

            auto&   axisAxis        =   _rotate->rotateAxis();          /// 6
            auto&   screenCircle    =   _rotate->rotateScreenCircle();  /// 62 (31 ball + 31 screen)
            auto&   arcs            =   _rotate->rotateArcs();          /// 3
            auto&   tangent         =   _rotate->tangent();             /// 8
            auto&   fan             =   _rotate->fan();

            /// ----------------------------------------------------------------
            /// 1. 组装线顶点 (PRI_LINES)
            ///    布局:
            ///      [0..5]      三条坐标轴线  (6 顶点, 3 段)
            ///      [6..67]     屏幕大圆配对 (31 段, 62 顶点, 含闭合段)
            ///      [68..129]   球面圆配对   (31 段, 62 顶点, 含闭合段)
            ///      [130..137]  切线        (8 顶点, 4 段)
            ///    固定 138 顶点
            /// ----------------------------------------------------------------
            const   size_t  kAxisLineVertCount      =   6;
            const   size_t  kScreenCirclePairCount  =   62;     ///31 段 * 2
            const   size_t  kBallCirclePairCount    =   62;     ///31 段 * 2
            const   size_t  kTangentVertCount       =   8;
            const   size_t  kLineVertTotal          =   kAxisLineVertCount
                                                        + kScreenCirclePairCount
                                                        + kBallCirclePairCount
                                                        + kTangentVertCount;
            const   size_t  kLineByteLen            =   kLineVertTotal * sizeof(float3);

            /// ----------------------------------------------------------------
            /// 2. 组装圆弧顶点 (PRI_LINES)
            ///    每条圆弧 N 个点 => (N-1) 段 => 2*(N-1) 顶点
            ///    圆弧是开放曲线,不需要闭合段
            /// ----------------------------------------------------------------
            _arcVertTotal  =   0;
            for (int i = 0; i < 3; ++i)
            {
                _arcVertCount[i]    =   0;
                _arcVertOffset[i]   =   0;
            }
            for (int i = 0; i < 3; ++i)
            {
                size_t  n   =   arcs[i].size();
                if (n >= 2)
                    _arcVertCount[i]    =   2 * (n - 1);  ///(n-1) 段 => 2*(n-1) 顶点
                else
                    _arcVertCount[i]    =   0;
                _arcVertOffset[i]    =   _arcVertTotal;
                _arcVertTotal        +=  _arcVertCount[i];
            }
            const   size_t  kArcByteLen     =   _arcVertTotal * sizeof(float3);

            /// ----------------------------------------------------------------
            /// 3. 扇形顶点 (PRI_TRIANGLE_FAN)
            /// ----------------------------------------------------------------
            _fanVertCount   =   fan.size();
            const   size_t  kFanVertCount   =   _fanVertCount;
            const   size_t  kFanByteLen     =   kFanVertCount * sizeof(float3);

            const   size_t  totalLen    =   kLineByteLen + kArcByteLen + kFanByteLen;
            if (totalLen == 0)
                return;

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
            ///--- 坐标轴线 (6 顶点)
            memcpy(pDst + off, axisAxis, kAxisLineVertCount * sizeof(float3));
            off  +=  kAxisLineVertCount * sizeof(float3);

            ///--- 屏幕大圆配对 (31 段, 62 顶点, 含闭合段)
            ///    原 31 个点: [31..61]
            ///    配对: (31,32), (32,33), ..., (60,61), (61,31)
            for (int i = 0; i < 30; ++i)
            {
                memcpy(pDst + off, &screenCircle[31 + i], sizeof(float3));     off += sizeof(float3);
                memcpy(pDst + off, &screenCircle[31 + i + 1], sizeof(float3));  off += sizeof(float3);
            }
            ///闭合段
            memcpy(pDst + off, &screenCircle[61], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &screenCircle[31], sizeof(float3));  off += sizeof(float3);

            ///--- 球面圆配对 (31 段, 62 顶点, 含闭合段)
            ///    原 31 个点: [0..30]
            for (int i = 0; i < 30; ++i)
            {
                memcpy(pDst + off, &screenCircle[i],     sizeof(float3));  off += sizeof(float3);
                memcpy(pDst + off, &screenCircle[i + 1], sizeof(float3));  off += sizeof(float3);
            }
            ///闭合段
            memcpy(pDst + off, &screenCircle[30], sizeof(float3));  off += sizeof(float3);
            memcpy(pDst + off, &screenCircle[0],  sizeof(float3));  off += sizeof(float3);

            ///--- 切线 (8 顶点)
            memcpy(pDst + off, tangent, kTangentVertCount * sizeof(float3));
            off  +=  kTangentVertCount * sizeof(float3);

            ///--- 三条圆弧 (开放曲线,不闭合)
            for (int i = 0; i < 3; ++i)
            {
                const   float3s&    arc     =   arcs[i];
                size_t  n   =   arc.size();
                if (n < 2)
                    continue;
                for (size_t j = 0; j < n - 1; ++j)
                {
                    memcpy(pDst + off, &arc[j],     sizeof(float3));  off += sizeof(float3);
                    memcpy(pDst + off, &arc[j + 1], sizeof(float3));  off += sizeof(float3);
                }
            }

            ///--- 扇形
            if (kFanVertCount > 0)
            {
                memcpy(pDst + off, fan.data(), kFanByteLen);
                off += kFanByteLen;
            }

            _cpu->unlock();

            /// ----------------------------------------------------------------
            /// 创建/复用 GPU VBO
            /// ----------------------------------------------------------------
            if (_axisLineVBO == nullptr || _axisLineVBO->cInfo()._length < kLineByteLen)
            {
                _axisLineVBO    =   _ctx.device().createVBO();
                _axisLineVBO->create({kLineByteLen,DEVICE_LOCAL_BIT});
            }
            if (kArcByteLen > 0 && (_arcVBO == nullptr || _arcVBO->cInfo()._length < kArcByteLen))
            {
                _arcVBO    =   _ctx.device().createVBO();
                _arcVBO->create({kArcByteLen,DEVICE_LOCAL_BIT});
            }
            if (kFanByteLen > 0 && (_fanVBO == nullptr || _fanVBO->cInfo()._length < kFanByteLen))
            {
                _fanVBO    =   _ctx.device().createVBO();
                _fanVBO->create({kFanByteLen,DEVICE_LOCAL_BIT});
            }

            auto    cmdPool =   _device.transferCmdPool();
            assert(cmdPool != nullptr);
            if (cmdPool != nullptr)
            {
                CMDPtr  cmd =   cmdPool->createCmd();
                cmd->begin(true);
                cmd->copyBuffer(_cpu, _axisLineVBO, kLineByteLen, 0, 0);
                if (kArcByteLen > 0)
                    cmd->copyBuffer(_cpu, _arcVBO, kArcByteLen, kLineByteLen, 0);
                if (kFanByteLen > 0)
                    cmd->copyBuffer(_cpu, _fanVBO, kFanByteLen, kLineByteLen + kArcByteLen, 0);
                cmd->end();
                cmd->submit(_device.queueTransfer());
            }
        }
        /// <summary>
        /// 每一帧调用,绘制旋转编辑轴
        /// </summary>
        virtual void    render(CMDPtr cmd) override
        {
            if (_rotate == nullptr || !flags().hasFlag(FE::FLAG_VISIBLE))
                return;
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

            mat4r   tMat    =   FE::translate(mat4r(),_rotate->position());
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
                if (_rotate->hoveredAxis() == i + 1)
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
                pushBlock._color    =   _rotate->enabled(i + 1)
                                       ? FE::packUnorm4x8(color)
                                       : FE::packUnorm4x8(DisableColor);
                cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                cmd->draw(2 * i, 2, 0, 1);
            }

            /// ----------------------------------------------------------------
            /// 2. 球面圆 + 屏幕大圆 (PRI_LINES)
            /// ----------------------------------------------------------------
            {
                /// 球面圆: 固定灰色, 始终先画
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4  color       =   { 0.6f, 0.6f, 0.6f, 0.5f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->setLineWidth(2);
                    cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(68, 62, 0, 1);
                }
                /// 屏幕大圆: 高亮时变黄色加粗
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4  color;
                    if (_rotate->hoveredAxis() == FEEditAxisRotate::AXIS_SC)
                    {
                        color = { 1.0f, 1.0f, 0.0f, 1.0f };
                        cmd->setLineWidth(4);
                    }
                    else
                    {
                        color = { 0.7f, 0.7f, 0.7f, 0.8f };
                        cmd->setLineWidth(2);
                    }
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(6, 62, 0, 1);
                }
            }

            /// ----------------------------------------------------------------
            /// 3. 三条圆弧 (PRI_LINES)
            /// ----------------------------------------------------------------
            if (_arcVBO != nullptr)
            {
                cmd->bindVBO(0,_arcVBO,0);
                for (int i = 0; i < 3; ++i)
                {
                    if (_arcVertCount[i] == 0)
                        continue;
                    float4  color   =   { 0.0f, 0.0f, 0.0f, 1.0f };
                    color[i]    =   1.0f;
                    if (_rotate->hoveredAxis() == i + 1)
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
                    pushBlock._color    =   _rotate->enabled(i + 1)
                                           ? FE::packUnorm4x8(color)
                                           : FE::packUnorm4x8(DisableColor);
                    cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw((uint32_t)_arcVertOffset[i], (uint32_t)_arcVertCount[i], 0, 1);
                }
            }

            /// ----------------------------------------------------------------
            /// 4. 切线 + 扇形 (仅旋转中绘制)
            /// ----------------------------------------------------------------
            if (_rotate->isRotatting())
            {
                ///切线 (PRI_LINES)
                cmd->bindVBO(0,_axisLineVBO,0);
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 1.0f, 1.0f, 0.0f, 1.0f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->setLineWidth(2);
                    cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    ///切线: [130..137]
                    cmd->draw(130, 8, 0, 1);
                }

                ///扇形 (PRI_TRIANGLE_FAN)
                if (_fanVBO != nullptr && _fanVertCount > 0)
                {
                    auto    plFan   =   _mat->pipeline(PRI_TRIANGLE_FAN)->as<FEGPipeline>();
                    cmd->bindPipeline(plFan);
                    _mat->appDynamicState(cmd,PRI_TRIANGLE_FAN);
                    cmd->bindVBO(0,_fanVBO,0);
                    cmd->setPrimitiveTopology(PRI_TRIANGLE_FAN);

                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 1.0f, 1.0f, 0.0f, 0.4f };
                    pushBlock._color    =   FE::packUnorm4x8(color);
                    cmd->pushConstants(plFan, plFan->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->draw(0, (uint32_t)_fanVertCount, 0, 1);
                }
            }
        }
        /// <summary>
        /// 销毁前一定调用该函数,解除数据对工厂的引用计数
        /// </summary>
        virtual void    destroy() override
        {
            _axisLineVBO    =   nullptr;
            _arcVBO         =   nullptr;
            _fanVBO         =   nullptr;
            _cpu            =   nullptr;
            _rotate         =   nullptr;
            _mat            =   nullptr;
        }
    protected:
        VBO                 _axisLineVBO    =   nullptr;
        VBO                 _arcVBO         =   nullptr;
        VBO                 _fanVBO         =   nullptr;
        VBO                 _cpu            =   nullptr;
        NodeRotateEditor    _rotate         =   nullptr;
        MaterialV3          _mat            =   nullptr;
    private:
        /// 圆弧顶点缓存 (供 render 使用)
        size_t              _arcVertTotal       =   0;
        size_t              _arcVertCount[3]    =   {0,0,0};
        size_t              _arcVertOffset[3]   =   {0,0,0};
        size_t              _fanVertCount       =   0;
    };
    using   FactoryRotate    =   SharedPtr<FEFactoryAxisRotate>;
}
