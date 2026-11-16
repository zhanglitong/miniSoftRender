#pragma     once

#include    "FEFactoryTemplate.hpp"
#include    "graphic/FEFactoryRender.hpp"

namespace   FE
{
    /// <summary>
    /// lambda 表达式作为类型别名，确保RFactoryLess是类型名
    /// </summary>
    constexpr auto    RFactoryLessLambda = [](const RFactory& l, const RFactory& r)
    {
        return  l->key().value() < r->key().value();
    };
    /// <summary>
    /// 先定义类型别名，确保RFactoryLess是类型名
    /// </summary>
    using   RFactoryLess    =   decltype(RFactoryLessLambda);

    /// <summary>
    /// 渲染工厂管理器:按 mesh key 有序保存工厂对象,
    /// 并提供 key / 对象 Id 两种查找方式;增删对象时同步重建 key 索引
    /// </summary>
    class   FEFactoryMgr :public FEFactoryTemplate<RFactory, RFactoryLess>
    {
    public:
        /// <summary>
        /// 构造:工厂按 mesh key 升序排列
        /// </summary>
        /// <param name="ctx">所属上下文</param>
        FEFactoryMgr(FEContext& ctx)
            :FEFactoryTemplate(ctx,RFactoryLessLambda)
        {}
        /// <summary>
        /// 拷贝构造:工厂列表与索引按源对象复制
        /// </summary>
        /// <param name="other">源管理器</param>
        FEFactoryMgr(const FEFactoryMgr& other)
            :FEFactoryTemplate(other)
        {}
        /// <summary>
        /// 按 mesh key 查找工厂对象,没有返回 nullptr。
        /// 工厂表按 key 有序,使用二分查找
        /// </summary>
        /// <param name="key">mesh key(MeshKey::value 生成的 uint64)</param>
        /// <returns>命中的工厂,未命中为 nullptr</returns>
        RFactory    findObject(uint64 key) const
        {
            auto        itr =   std::lower_bound(_objects.begin(), _objects.end(), key,[](const RFactory& l, uint64 val)
            {
                return  l->key().value() < val;
            });
            if (itr != _objects.end() && (*itr)->key().value() == key)
                return  *itr;
            else
                return  nullptr;
        }
        /// <summary>
        /// 按对象 Id 查找工厂对象,如果没有返回nullptr。
        /// 工厂表按 mesh key 排序,对象 Id 不在索引内,这里线性查找
        /// </summary>
        /// <param name="id">工厂的对象 Id(FEObject::objectId)</param>
        /// <returns>命中的工厂,未命中为 nullptr</returns>
        RFactory    findObject(const FEUuid& id) const
        {
            for (auto& var : _objects)
            {
                if (var->objectId() == id)
                    return  var;
            }
            return  nullptr;
        }
        /// <summary>
        /// 清空管理器:清掉 key 索引,移除所有非常驻工厂;
        /// 驻留工厂(isResident 为 true)保留
        /// </summary>
        void        clear()
        {
            _factoryMap.clear();
            auto   temp =   objects();
            for (auto var: temp)
            {
                if (!var->isResident())
                {
                    removeObject(var);
                }
            }
        }
    protected:
        /// <summary>
        /// 子类可以重写，添加对象的回调函数
        /// </summary>
        virtual void    onAddObjects() override
        {
            buildCache();
        }
        /// <summary>
        /// 移除对象后的回调:重建 key 索引
        /// </summary>
        virtual void    onRemoveObjects() override
        {
            buildCache();
        }
        /// <summary>
        /// 按当前工厂列表重建 key 索引(增删对象后调用)
        /// </summary>
        inline  void    buildCache()
        {
            _factoryMap.clear();
            for (auto var : objects())
            {
                _factoryMap[var->key().value()] = var;
            }
        }
    protected:  
        /// <summary>
        /// mesh key -> 工厂 的索引,随工厂列表增删重建
        /// </summary>
        RFactoryMap _factoryMap;
    };
}

