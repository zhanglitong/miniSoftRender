#pragma     once

#include    "FEFactoryRender.hpp"

namespace   FE
{
    class   FEUpdateObject;
    using   UpdateNotify    =   std::function<void(CMDPtr,FEUpdateObject&)>;
    class   FEUpdateObject
    {
    public:
        /// <summary>
        /// 绑定更新的对象s
        /// </summary>
        Object          _object;
        /// <summary>
        /// 通知
        /// </summary>
        UpdateNotify    _notify;
        /// <summary>
        /// 更新的优先级
        /// </summary>
        FEPriority      _priority;
        /// <summary>
        /// gpu
        /// </summary>
        GPUBuffer       _cpu;
        GPUBuffer       _gpu;
    };
    using   UpdateObjects   =   std::vector<FEUpdateObject>;

    class   FEUpdateQueue
    {
    public:
        /// <summary>
        /// 添加需要更新的对象
        /// </summary>
        /// <param name="object"></param>
        /// <param name="pri"></param>
        /// <returns></returns>
        inline  bool    addObject(Object object,const UpdateNotify& notify,FEPriority pri = {})
        {
            FEUpdateObject  uData =   {object,notify,pri};
            auto    itr =   std::lower_bound(_queue.begin(),_queue.end(),uData,[](const FEUpdateObject& left,const FEUpdateObject& right)
            {
                if (left._priority.priority() == right._priority.priority())
                {
                    if(left._priority.order() == right._priority.order())
                        return  left._object.get() < right._object.get();
                    else
                        return  left._priority.order() < right._priority.order();
                }
                else
                {
                    return  left._priority.priority() < right._priority.priority();
                }
            });
            if (itr != _queue.end() && (*itr)._object == object)
                return  false;
            else
                _queue.insert(itr,uData);
            return  true;
        }
        /// <summary>
        /// 移除更新对象
        /// </summary>
        /// <param name="object"></param>
        /// <returns></returns>
        inline  bool    removeObject(Object object)
        {
            size_t  count   =   _queue.size();
            auto    itr     =   std::remove_if(_queue.begin(),_queue.end(),[&](const FEUpdateObject& value)
            {
                return  value._object == object;
            });
            _queue.erase(itr,_queue.end());
            return  count > _queue.size();
        }
        const   auto&   queue() const
        {
            return  _queue;
        }
        inline  auto&   queue()
        {
            return  _queue;
        }
        /// <summary>
        /// 执行更新
        /// </summary>
        /// <param name="cmd"></param>
        inline  void    update(CMDPtr cmd)
        {
            for (auto& var : _queue)
            {
                if (var._object == nullptr || !var._notify)
                    continue;
                else if(var._object->flags().hasFlag(FLAG_UPDATE))
                    var._notify(cmd,var);
            }
        }
    protected:
        UpdateObjects   _queue;
    };
}