#pragma     once

#include    "FEFactory.hpp"

namespace   FE
{
    class   FEFactoryObject :public FEFactory
    {
    public:
        FEFactoryObject(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEFactoryObject(const FEFactoryObject& other)
            :FEFactory(other)
        {
            _objects    =   other._objects;
        }
        const   Objects&objects() const
        {
            return  _objects;
        }
        /// <summary>
        /// 添加对象
        /// </summary>
        /// <param name="object"></param>
        /// <returns></returns>
        virtual size_t  addObject(const Object& object)
        {
            auto    nOld    =   _objects.size();
            _objects.push_back(object);
            /// 排序
            std::sort(_objects.begin(),_objects.end());
            /// unique 会把重复的放到后面
            auto    newEnd  =   std::unique(_objects.begin(),_objects.end());
            /// 删除重复的
            _objects.erase(newEnd, _objects.end());
            auto    result  =   _objects.size() - nOld;
            if (result != 0)
            {
                flags().addFlag(FLAG_ADD_OBJECT);
            }
            return  _objects.size() - nOld;
        }
        /// <summary>
        /// 返回增加的个数
        /// </summary>
        /// <param name="objects"></param>
        /// <returns></returns>
        virtual size_t  addObjects(const Objects& objects)
        {
            /// 数据先插入到末尾
            auto    nOld    =   _objects.size();
            _objects.insert(_objects.end(),objects.begin(),objects.end());
            /// 排序
            std::sort(_objects.begin(),_objects.end());
            /// unique 会把重复的放到后面
            auto    newEnd  =   std::unique(_objects.begin(),_objects.end());
            /// 删除重复的
            _objects.erase(newEnd, _objects.end());
            auto    result  =   _objects.size() - nOld;
            if (result != 0)
            {
                flags().addFlag(FLAG_ADD_OBJECT);
            }
            return  _objects.size() - nOld;
        }
        virtual size_t  removeObject(const Object& object)
        {
            return  removeObjectImpl(object);
        }
        /// <summary>
        /// 先复制一份,排序使用
        /// </summary>
        /// <param name="objects"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(const Objects& objects)
        {
            /// 先复制一份,排序使用,主要考虑性能问题
            Objects deletes     =   objects;
            return  removeObjectsImpl(std::move(deletes));
        }
        /// <summary>
        /// 不复制，函数会改deletes 数据顺序,但性能更好
        /// </summary>
        /// <param name="deletes"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(Objects& deletes)
        {
            return  removeObjectsImpl(std::move(deletes));
        }
        /// <summary>
        /// 支持右值
        /// </summary>
        /// <param name="deletes"></param>
        /// <returns></returns>
        virtual size_t  removeObjects(Objects&& deletes)
        {
            return  removeObjectsImpl(std::move(deletes));
        }
    protected:
        inline  size_t  removeObjectImpl(const Object& needDelete)
        {
            size_t  nOld    =   _objects.size();
            /// 
            auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const Object& object) 
            {
                return  object == needDelete;
            });
            _objects.erase(newEnd, _objects.end());
            auto    result  =   _objects.size() - nOld;
            if (result != 0)
            {
                flags().addFlag(FLAG_REMOVE_OBJECT);
            }
            return  result;
        }
        /// <summary>
        /// removeObjects的具体实现函数
        /// </summary>
        /// <param name="deletes"></param>
        /// <returns></returns>
        inline  size_t  removeObjectsImpl(Objects&& deletes)
        {
            size_t  nOld    =   _objects.size();
            /// 先排序，后续查找速度快
            std::sort(deletes.begin(),deletes.end());
            /// 删除逻辑：如果当前值在 deletes 里能找到，就删除(返回 true)
            auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const Object& object) 
            {
                return  std::binary_search(deletes.begin(), deletes.end(), object);
            });
            _objects.erase(newEnd, _objects.end());
            auto    result  =   _objects.size() - nOld;
            if (result != 0)
            {
                flags().addFlag(FLAG_REMOVE_OBJECT);
            }
            return  result;
        }
    protected:
        Objects     _objects;
    };
}

