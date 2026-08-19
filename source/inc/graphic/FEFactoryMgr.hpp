#pragma     once

#include    "FEFactoryTemplate.hpp"
#include    "graphic/FEFactoryRender.hpp"

namespace   FE
{
    /// lambda 表达式作为类型别名，确保RFactoryLess是类型名
    constexpr auto    RFactoryLessLambda = [](const RFactory& l, const RFactory& r)
    {
        return  l->key() < r->key();
    };
    /// 先定义类型别名，确保RFactoryLess是类型名
    using   RFactoryLess    =   decltype(RFactoryLessLambda);

    class   FEFactoryMgr :public FEFactoryTemplate<RFactory, RFactoryLess>
    {
    public:
        FEFactoryMgr(FEContext& ctx)
            :FEFactoryTemplate(ctx,RFactoryLessLambda)
        {}
        FEFactoryMgr(const FEFactoryMgr& other)
            :FEFactoryTemplate(other)
        {}
        /// <summary>
        /// 查找工厂对象,如果没有返回nullptr
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        RFactory    findObject(uint64 key) const
        {
            auto        itr =   std::lower_bound(_objects.begin(), _objects.end(), key,[](const RFactory& l, uint64 val)
            {
                return  l->key() < val;
            });
            if (itr != _objects.end() && (*itr)->key() == key)
                return  *itr;
            else
                return  nullptr;
        }
    protected:
        /// <summary>
        /// 子类可以重写，添加对象的回调函数
        /// </summary>
        virtual void    onAddObjects() override
        {
            buildCache();
        }
        virtual void    onRemoveObjects() override
        {
            buildCache();
        }
        inline  void    buildCache()
        {
            _factoryMap.clear();
            for (auto var : objects())
            {
                _factoryMap[var->key()] = var;
            }
        }
    protected:  
        RFactoryMap _factoryMap;
    };
}

