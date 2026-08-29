#pragma     once

#include    "FEObject.h"

namespace   FE
{
    template<typename TObject, typename FuncSort>
    class   FEObjectsTemplate
    {
    public:
        using   TObjects     =   std::vector<TObject>;
    public:
        FEObjectsTemplate(FuncSort func)
            :_sortFunc(func)
        {}
        FEObjectsTemplate(const FEObjectsTemplate& other)
            :_sortFunc(other._sortFunc)
        {
            _objects    =   other._objects;
        }
        /// <summary>
        /// 获取对象列表
        /// </summary>
        /// <returns></returns>
        const   TObjects&   objects() const
        {
            return  _objects;
        }
        /// <summary>
        /// 是否已经存在对象
        /// </summary>
        /// <param name="object"></param>
        /// <returns></returns>
        inline  bool    exists(const TObject& object) const
        {
            auto        itr =   std::lower_bound(_objects.begin(), _objects.end(), object,_sortFunc);
            if (itr != _objects.end() && *itr == object)
                return  true;
            else
                return  false;
        }
        inline  bool    contains(const TObject& object) const
        {
            return  exists(object);
        }
        /// <summary>
        /// 添加对象,返回0,已经存在,返回1,添加成功
        /// </summary>
        /// <param name="object"></param>
        /// <returns>添加成功对象数</returns>
        virtual size_t  addObject(const TObject& object)
        {
            auto    itr     =   std::lower_bound(_objects.begin(), _objects.end(), object,_sortFunc);
            if (itr != _objects.end() && *itr == object)
                return  0;
            _objects.insert(itr, object);
            onAddObject(object);
            return  1;
        }
        /// <summary>
        /// 批量添加对象返回增加的个数
        /// </summary>
        /// <param name="objects"></param>
        /// <returns></returns>
        virtual size_t  addObjects(const TObjects& objects)
        {
            /// 数据先插入到末尾
            auto    nOld    =   _objects.size();
            _objects.insert(_objects.end(),objects.begin(),objects.end());
            /// 排序
            std::sort(_objects.begin(),_objects.end(),_sortFunc);
            /// unique 会把重复的放到后面
            auto    newEnd  =   std::unique(_objects.begin(),_objects.end());
            /// 删除重复的
            _objects.erase(newEnd, _objects.end());
            auto    result  =   _objects.size() - nOld;
            if (result == 0)
                return  result;
            else if (result == objects.size())
            {
                for (auto& var : objects)
                    onAddObject(var);
            } 
            else
            {
                for (auto& var : objects)
                {
                    if (exists(var))
                        onAddObject(var);
                }
            } 
            return  result;
        }
        virtual size_t  removeObject(const TObject& object)
        {
            return  removeObjectImpl(object);
        }
        /// <summary>
        /// 先复制一份,排序使用
        /// </summary>
        /// <param name="objects"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(const TObjects& objects)
        {
            /// 先复制一份,排序使用,主要考虑性能问题
            TObjects    deletes     =   objects;
            return  removeObjectsImpl(std::move(deletes));
        }
        /// <summary>
        /// 不复制，函数会改deletes 数据顺序,但性能更好
        /// </summary>
        /// <param name="deletes"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(TObjects& deletes)
        {
            return  removeObjectsImpl(std::move(deletes));
        }
        /// <summary>
        /// 支持右值
        /// </summary>
        /// <param name="deletes"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(TObjects&& deletes)
        {
            return  removeObjectsImpl(std::move(deletes));
        }
        /// <summary>
        /// 清空所有数据
        /// </summary>
        virtual void    clearObjects()
        {
            if (!_objects.empty())
            {
                for (auto& var: _objects)
                {
                    onRemoveObject(var);
                }
                _objects.clear();
            }
        }
    protected:
        /// <summary>
        /// 子类可以重写，添加对象的回调函数
        /// </summary>
        virtual void    onAddObject (TObject)
        {}
        /// <summary>
        /// 移除对象通知，子类重写
        /// </summary>
        virtual void    onRemoveObject(TObject)
        {}
    protected:
        inline  size_t  removeObjectImpl(const TObject& needDelete)
        {
            auto    itr     =   std::lower_bound(_objects.begin(), _objects.end(), needDelete,_sortFunc);
            if (itr == _objects.end()  || *itr != needDelete)
                return  0;
            _objects.erase(itr);
            onRemoveObject(needDelete);
            return  1;
        }
        /// <summary>
        /// removeObjects的具体实现函数
        /// </summary>
        /// <param name="deletes"></param>
        /// <returns></returns>
        inline  size_t  removeObjectsImpl(TObjects&& deletes)
        {
            size_t  nOld    =   _objects.size();
            /// 先排序，后续查找速度快
            std::sort(deletes.begin(),deletes.end());
            /// 删除逻辑：如果当前值在 deletes 里能找到，就删除(返回 true)
            auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const TObject& object) 
            {
                auto    result  =   std::binary_search(deletes.begin(), deletes.end(), object);
                return  result;
            });
            _objects.erase(newEnd, _objects.end());

            for (auto itr = newEnd; itr != _objects.end(); ++ itr)
            {
                onRemoveObject(*itr);
            }
            auto    result  =   _objects.size() - nOld;
            return  result;
        }
    protected:
        FuncSort    _sortFunc;  
        TObjects    _objects;
    };
}

