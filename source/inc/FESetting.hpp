#pragma     once
#include    <stdarg.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdint.h>
#include    <assert.h>
#include    <functional>
#include    <variant>
#include    <map>

#include    "FEDefine.h"
#include    "FEDateTime.hpp"
#include    "FEMath.hpp"
#include    "FEObject.h"

namespace   FE
{
    class   FESetting
    {
    public:
        class   Item;
        using   Value   =   std::variant<std::monostate
                                        , bool,  int32_t, uint32_t, int64_t, uint64_t, float, double
                                        , int2,   int3,   int4
                                        , uint2,  uint3,  uint4
                                        , float2, float3, float4
                                        , real2,  real3,  real4>;
        /// <summary>
        /// 当前item,以及历史值,如果第二个为nullptr,则是第一次设置
        /// </summary>
        using   NotifyChange    =   std::function<void(const Item& Item,const Value*)>;
        using   NotifyChanges   =   std::map<const void*,NotifyChange>;
        class   Item :public std::enable_shared_from_this<Item>
        {
        public:
            using   ItemPtr     =   std::shared_ptr<Item>;
            using   ItemPtrs    =   std::vector<ItemPtr>;
        public:
            Item(const char* name)
            {
                _name   =   name;
            }
            /// <summary>
            /// 添加通知
            /// </summary>
            /// <param name="key"></param>
            /// <param name="notify"></param>
            /// <returns></returns>
            inline  auto&   bind(const void* key,const NotifyChange& notify)
            {
                if (notify && key)
                    _notifys[key]   =   notify;
                return  *this;
            }
            /// <summary>
            /// 移除通知
            /// </summary>
            /// <param name="key"></param>
            /// <returns></returns>
            inline  auto&   unbind(const void* key)
            {
                auto    itr =   _notifys.find(key);
                if (itr != _notifys.end())
                    _notifys.erase(itr);
                return  *this;
            }
            /// <summary>
            /// 清除通知
            /// </summary>
            inline  void    clearNotify()
            {
                _notifys.clear();
            }
            /// <summary>
            /// 获取，如果没有，添加
            /// </summary>
            /// <param name="name"></param>
            /// <returns></returns>
            inline  auto&   get(const char* name)
            {
                auto    item    =   query(name);
                if (item == nullptr)
                {
                    item    =   std::make_shared<Item>(name);
                    _childs.emplace_back(item);
                }
                return  *item;
            }
            /// <summary>
            /// 获取值
            /// </summary>
            /// <typeparam name="TVal"></typeparam>
            /// <returns></returns>
            template<class TVal>
            inline   TVal*   value(const TVal& val = {})
            {
                /// 如果没有初始化
                if (std::holds_alternative<std::monostate>(_value))
                {
                    _value  =   val;
                    fire(nullptr);
                }
                return std::get_if<TVal>(&_value);
            }
            /// <summary>
            /// 设置值
            /// </summary>
            /// <typeparam name="TVal"></typeparam>
            /// <param name="val"></param>
            /// <returns></returns>
            template<class TVal>
            inline  auto&   set(const TVal& val)
            {
                /// 说明还没有初始化
                if (std::holds_alternative<std::monostate>(_value))
                {
                    _value  =   val;
                    fire(nullptr);
                    return  *this;
                }
                
                auto    pOld    =   value<TVal>();
                assert(pOld != nullptr);
                if (pOld == nullptr || (*pOld) == val)  
                    return  *this;
                Value   old     =   _value;
                        *pOld   =   val;
                fire(&old);
                return  *this;
            }
        protected:
            /// <summary>
            /// 触发通知
            /// </summary>
            inline  void    fire(const Value* old) const
            {
                for (auto& var : _notifys)
                {
                    var.second(*this,old);
                }
            }
            /// <summary>
            /// 查询
            /// </summary>
            /// <param name="name"></param>
            /// <returns></returns>
            inline  ItemPtr query(const char* name)
            {
                for (auto  var : _childs)
                {
                    if (var->_name == name)
                        return  var;
                }
                return  nullptr;
            }
        protected:
            Value           _value;
            String          _name;
            NotifyChanges   _notifys;
            ItemPtrs        _childs;
        };
        using   ItemPtr     =   std::shared_ptr<Item>;
        using   ItemPtrs    =   std::vector<ItemPtr>;
    public:
        FESetting()
        {
            _root   =   std::make_shared<Item>("");
        }
        ~FESetting()
        {
            _root   =   nullptr;
        }
        inline  auto&   get(const char* name)
        {
            return  _root->get(name);
        }
    protected:
        ItemPtr _root;
    };

    using   Setting  =   SharedPtr<FESetting>;
}

