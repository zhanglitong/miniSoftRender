#pragma     once

#include    "FEObject.h"
#include    "FENotify.hpp"



namespace   FE
{
    using   Notify  =   SharedPtr<FENotify>;

    template<class T>
    class   FEItem :public FEObject
    {
    public:
        using   Item    =   SharedPtr<T>;
        using   Items   =   std::vector<Item>;
    public:
        FEItem(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEItem(const FEItem& other)
            :FEObject(other)
        {
            _childs =   FEObjectHelper::clone(other._childs);
        }

        /// <summary>
        /// 获取子对象(只读)
        /// </summary>
        /// <returns></returns>
        const   Items&  children()  const
        {
            return  _childs;
        }
        inline  Items&  children()
        {
            return  _childs;
        }
        /// <summary>
        /// 获取父对象
        /// </summary>
        /// <returns></returns>
        inline  Item    parent() const
        {
            return  _parent;
        }
        /// <summary>
        /// 添加子节点
        /// </summary>
        /// <param name="child">被添加对象</param>
        /// <returns></returns>
        virtual bool    addChild(Item child)
        {
            auto    itr =   std::find(_childs.begin(),_childs.end(),child);
            if (itr != _childs.end())
                return  false;
            _childs.emplace_back(child);
            child->setParent(this);
            onAddChildren();
            return  true;
        }
        /// <summary>
        /// 添加字对象，返回添加成功的个数
        /// </summary>
        /// <param name="objects">被添加item对象列表</param>
        /// <returns></returns>
        virtual size_t  addChildren(const Items& objects)
        {
            size_t  result  =   0;
            for (auto var: objects)
            {
                auto    itr =   std::find(_childs.begin(),_childs.end(),var);
                if (itr != _childs.end())
                    continue;
                var->setParent(this);
                _childs.emplace_back(var);
                ++result;
            }
            if (result)
            {
                onAddChildren();
            }
            return  result;
        }
        /// <summary>
        /// 移除子对象
        /// </summary>
        /// <param name="child"></param>
        /// <returns></returns>
        virtual bool    removeChild(Item child)
        {
            auto    itr =   std::find(_childs.begin(),_childs.end(),child);
            if (itr == _childs.end())
                return  false;
            _childs.erase(itr);
            (*itr)->setParent(nullptr);

            onRemoveChildren();
            return  true;
        }
        /// <summary>
        /// 根据对象id移除
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        virtual bool    removeChild(const FEUuid& id)
        {
            auto    itr =   std::find_if(_childs.begin(),_childs.end(),[&id](const Item& object)
                {
                    return  object->objectId() == id;
                });
            if (itr == _childs.end())
                return  false;
            _childs.erase(itr);
            (*itr)->setParent(nullptr);
            onRemoveChildren();
            return  true;
        }
        /// <summary>
        /// 移除
        /// </summary>
        /// <param name="objects"></param>
        /// <returns></returns>
        virtual size_t  removeChildren(const Items& objects)
        {
            size_t  result  =   0;
            for (auto var: objects)
            {
                auto    itr =   std::find(_childs.begin(),_childs.end(),var);
                if (itr == _childs.end())
                    continue;
                _childs.erase(itr);
                (*itr)->setParent(nullptr);
                ++result;
            }
            if (result)
            {
                onRemoveChildren();
            }
            return  result;
        }
        /// <summary>
        /// 移除所有子item
        /// </summary>
        virtual void    removeAllChildren()
        {
            for (auto& var : _childs)
            {
                var->setParent(nullptr);
            }
            _childs.clear();
            onRemoveChildren();
        }
    protected:
        virtual void    onAddChildren()     =   0;
        virtual void    onRemoveChildren()  =   0;
        /// <summary>
        /// 设置父对象
        /// 如果要修改父节，则需要调用parent()->addChild / parent()->removeChilds 实现
        /// </summary>
        /// <param name="parent"></param>
        inline  void    setParent(FEItem* parent)
        {
            _parent =   parent->cast<T>();
        }
    protected:
        Item            _parent;
        /// <summary>
        /// 子节点
        /// </summary>
        Items           _childs;
    };
}


