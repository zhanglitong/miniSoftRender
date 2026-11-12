#pragma     once

#include    "graphic/FEFactoryRender.hpp"
#include    "material/FEMaterialLibrary.hpp"
#include    "FEInputCom.hpp"


namespace   FE
{
    /// <summary>
    /// 该实现逻辑简单，开发者可以自由发挥更新与渲染函数
    /// </summary>
    class   FERenderPlugin :public FEFactoryRender
    {
    public:
        FERenderPlugin(FEContext& ctx)
            :FEFactoryRender(ctx)
        {}
        FERenderPlugin(const FERenderPlugin& other)
            :FEFactoryRender(other)
        {}
        /// <summary>
        /// 如果需要
        /// </summary>
        /// <returns></returns>
        virtual Input   inputComponent() 
        {
            return  nullptr;
        }
        /// <summary>
        /// 每一帧调用
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    update(CMDPtr ) override
        {}
        /// <summary>
        /// 每一帧调用
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    render(CMDPtr cmd,const FERenderPassInfo& pass = FERenderPassInfo()) override
        {}
        /// <summary>
        /// 工厂销毁时由基类 destroy() 调用,派生出可在此释放自有资源
        /// </summary>
        virtual void    onDestroy() override
        {}
    };
    using   FactorySimple   =   SharedPtr<FERenderPlugin>;
}

