#pragma     once

#include    "../axis/FENodeMoveEditor.h"
#include    "../material/FEMaterialV3.hpp"
#include    "FEFactoryRender.hpp"


namespace   FE
{
    /// <summary>
    /// 该实现逻辑简单，开发者可以自由发挥更新与渲染函数
    /// </summary>
    class   FEFactoryAxisMove :public FEFactoryRender
    {
    public:
        FEFactoryAxisMove(FEContext& ctx)
            :FEFactoryRender(ctx)
        {
            _move   =   new FENodeMoveEditor(_ctx);
            _mat    =   new FEMaterialV3(_ctx);
            /// 设置渲染优先级最低，场景中最后绘制
            _prioritys[PT_Render].setPriority(EP_Last);
            /// 同优先级中，最后绘制
            _prioritys[PT_Render].setOrder(MaxInt16);
        }
        FEFactoryAxisMove(const FEFactoryAxisMove& other)
            :FEFactoryRender(other)
            ,_move(other._move)
        {}
        inline  auto    inputComponent() const
        {
            return  _move;
        }
        inline  auto    inputComponent()
        {
            return  _move;
        }
    public:
        /// <summary>
        /// 每一帧调用
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    update(CMDPtr ) override
        {
            if (_move == nullptr 
                || !_move->flags().hasFlag(FE::FLAG_VISIBLE) 
                || !flags().hasFlag(FE::FLAG_VISIBLE))
                return;
            /// 每一帧调用更新
           _move->update(_ctx.activeCamera());
           auto&    indexs  =   _move->indexs();
           auto&    lines   =   _move->moveAxis();
           auto&    array   =   _move->axisArray();

           size_t  lineLen =   lines.size() * sizeof(float3);
           size_t  arrLen  =   array.size() * sizeof(float3);
           size_t  length  =   lineLen + arrLen;


            if (_axisLineIBO == nullptr)
            {
                _axisLineIBO    =   _ctx.device().createIBO(); 
                _axisLineIBO->create({indexs.size() * sizeof(uint16),DEVICE_LOCAL_BIT});
                _axisLineIBO->update(indexs.data(),indexs.size() * sizeof(uint16),0);
            }
            if (_axisLineVBO == nullptr)
            {
                _axisLineVBO    =   _ctx.device().createVBO(); 
                _axisLineVBO->create({lineLen,DEVICE_LOCAL_BIT});
            }

            if (_axisArrowVBO == nullptr || _axisArrowVBO->cInfo()._length < arrLen)
            {
                _axisArrowVBO    =   _ctx.device().createVBO(); 
                _axisArrowVBO->create({arrLen,DEVICE_LOCAL_BIT});
            }
            if (_cpu == nullptr)
            {
                _cpu     =   _ctx.device().createVBO(); 
                _cpu->create({length,HOST_VISIBLE_BIT});
            }
            uint8*  pDst    =   (uint8*)_cpu->lock(length,0);
            memcpy(pDst + 0,        lines.data(),lineLen);
            memcpy(pDst + lineLen,  array.data(),arrLen);
            _cpu->unlock();

            auto    cmdPool =   _device.transferCmdPool();
            assert (cmdPool != nullptr);
            if (cmdPool != nullptr)
            {
                CMDPtr      cmd     =   cmdPool->createCmd();
                cmd->begin(true);
                cmd->copyBuffer(_cpu,   _axisLineVBO,   lineLen,0,      0);
                cmd->copyBuffer(_cpu,   _axisArrowVBO,  arrLen, lineLen,0);
                cmd->end();
                cmd->submit(_device.queueTransfer());
            }
        }
        /// <summary>
        /// 每一帧调用
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    render(CMDPtr cmd) override
        {
            if (_move == nullptr 
                || !_move->flags().hasFlag(FE::FLAG_VISIBLE) 
                || !flags().hasFlag(FE::FLAG_VISIBLE))
                return;
            if (_mat == nullptr)
                return;
            auto        pl      =   _mat->pipeline(PRI_TRIANGLE_FAN)->as<FEGPipeline>();
            cmd->bindPipeline(pl);
            cmd->setDepthTest(false);
            FECmdBuffer::Viewport   viewPort    =   
            {
                0.0f,0.0f,(float)_ctx.windowsWidth(),(float)_ctx.windowsHeight(),0.0f,1.0f
            };
            RectU32     rect(0,0,_ctx.windowsWidth(),_ctx.windowsHeight());

            cmd->setViewport(0,  1,  &viewPort);
            cmd->setScissor(0,   1,  &rect);
            cmd->setCullMode(CullMode::CM_NULL);
            cmd->setDepthTest(false);
            _mat->appDynamicState(cmd,PRI_TRIANGLE_FAN);
            cmd->bindVBO(0,_axisArrowVBO,0);
            cmd->bindIBO(_axisLineIBO,0,INDEX_UINT16);
            /// 绘制箭头
            for (int i = 0; i < 3; ++i)
            {
                mat4r   tMat0   =   FE::translate(mat4r(),_move->position());
                mat4r   tMat1   =   FE::translate(mat4r(),real3(_move->moveAxis()[i + 1]));
                    
                mat4r   tMat2   =   _ctx.mvp() * tMat0 * tMat1;
                   
                float4  color   =   { 0.0f, 0.0f, 0.0f, 1.0f };
                        color[i]=   1.0f;
                PointData pushBlock;
                pushBlock._point    =   0;
                pushBlock._mvp      =   tMat2;
                if (_move->enabled(i + 1))
                    pushBlock._color    =   FE::packUnorm4x8(color);
                else
                    pushBlock._color    =   FE::packUnorm4x8(DisableColor);
                cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                cmd->setPrimitiveTopology(PRI_TRIANGLE_FAN);
                cmd->draw(32 * i, 32,0,1);
            }
            ///  绘制轴
            mat4r   tMat    =   FE::translate(mat4r(),_move->position());
            mat4r   mvp     =   _ctx.mvp() * tMat;

            pl      =   _mat->pipeline(PRI_LINES)->as<FEGPipeline>();
            cmd->bindPipeline(pl);
            cmd->setDepthTest(false);
            _mat->appDynamicState(cmd,PRI_LINES);
            cmd->bindVBO(0,_axisLineVBO,0);
            cmd->bindIBO(_axisLineIBO,0,INDEX_UINT16);
              
            for (int i = 0; i < 3; i++)
            {
                PointData   pushBlock;
                pushBlock._point    =   0;
                pushBlock._mvp      =   mvp;
                float4      color   =   { 0.0f, 0.0f, 0.0f, 1.0f };
                            color[i]=   1.0f;
                if (_move->enabled(i + 1))
                    pushBlock._color    =   FE::packUnorm4x8(color);
                else
                    pushBlock._color    =   FE::packUnorm4x8(DisableColor);

                cmd->setLineWidth(2);
                cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                cmd->setPrimitiveTopology(PRI_LINES);
                cmd->drawIndex(10 * i,4,0,0,1);
            }
            {   
                for (int i = 0; i < 3; i++)
                {
                    PointData   pushBlock;
                    pushBlock._point    =   0;
                    pushBlock._mvp      =   mvp;
                    float4      color   =   { 0.0f, 0.0f, 0.0f, 1.0f };
                    if (_move->hoveredAxis() == i + 1)
                    {
                        color[0]    =   1.0f;
                        color[1]    =   1.0f;
                        cmd->setLineWidth(4);
                    }
                    else
                    {
                        color[i]    =   1.0f;
                        cmd->setLineWidth(2);
                    }
                    if (_move->enabled(i + 1))
                        pushBlock._color    =   FE::packUnorm4x8(color);
                    else
                        pushBlock._color    =   FE::packUnorm4x8(DisableColor);
                    cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                    cmd->setPrimitiveTopology(PRI_LINES);
                    cmd->drawIndex(10 * i + 4,2,0,0,1);
                }
            }
            /// 绘制选中面
            pl      =   _mat->pipeline(PRI_TRIANGLE_FAN)->as<FEGPipeline>();
            cmd->bindPipeline(pl);
            cmd->setDepthTest(false);
            _mat->appDynamicState(cmd,PRI_TRIANGLE_FAN);
            cmd->bindVBO(0,_axisLineVBO,0);
            cmd->bindIBO(_axisLineIBO,0,INDEX_UINT16);

            /// 绘制选中面
            for (int i = 0; i < 3; ++i)
            {
                ///如果某个面被选中
                if (_move->hoveredAxis() != i + 4) 
                    continue;

                float4  color   =   { 1.0f,1.0f,0.0f,0.6f };
                mat4r   tMat1   =   FE::scale(mat4r(1),real3(0.97));
                mat4r   res     =   mvp * tMat1;

                PointData pushBlock;
                pushBlock._point    =   0;
                pushBlock._color    =   FE::packUnorm4x8(color);
                pushBlock._mvp      =   res;
                cmd->pushConstants(pl, pl->cInfo()._pushConstantStage.data(),0,sizeof(pushBlock),&pushBlock);
                cmd->setPrimitiveTopology(PRI_TRIANGLE_FAN);
                cmd->drawIndex(10 * i + 6,4, 0,0,1);
            }
        }
        /// <summary>
        /// 销毁前一定调用该函数，解除数据对工厂的引用计数
        /// 才能正确的释放数据
        /// </summary>
        virtual void    destroy() override
        {
            _axisLineVBO    =   nullptr;
            _axisArrowVBO   =   nullptr;
            _axisLineIBO    =   nullptr;
            _cpu            =   nullptr;
            _move           =   nullptr;
            _mat            =   nullptr;
        }
    protected:
        VBO             _axisLineVBO    =   nullptr;
        VBO             _axisArrowVBO   =   nullptr;
        IBO             _axisLineIBO    =   nullptr;
        VBO             _cpu            =   nullptr; 
        NodeMoveEditor  _move           =   nullptr;
        MaterialV3      _mat            =   nullptr;
    };
    using   FactorySimple   =   SharedPtr<FEFactoryAxisMove>;
}

