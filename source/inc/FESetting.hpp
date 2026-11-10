#pragma     once
#include    <cassert>
#include    <cstdint>
#include    <cstring>
#include    <filesystem>
#include    <functional>
#include    <memory>
#include    <string>
#include    <type_traits>
#include    <utility>
#include    <variant>
#include    <vector>
#include    "FEDefine.h"
#include    "FEDelegate.hpp"
#include    "FEMath.hpp"

namespace   FE
{
    /// <summary>
    /// 分层配置容器:每个节点持有一个 variant 值、若干子节点。
    /// 值的类型在首次写入时确定,之后不允许改变(反序列化除外)。
    /// </summary>
    class   FE_API  FESetting
    {
    public:
        class   Item;
        using   Value   =   std::variant<std::monostate
                                        , bool,  int32_t, uint32_t, int64_t, uint64_t, float, double
                                        , int2,   int3,   int4
                                        , uint2,  uint3,  uint4
                                        , float2, float3, float4
                                        , real2,  real3,  real4
                                        , std::string>;
        /// <summary>
        /// 值变更通知:第一个参数是当前 item,第二个是旧值;为 nullptr 表示首次设置
        /// 注册方式:bind(key,fn)、bind({对象,&Class::方法}) 或 changes() += {对象,&Class::方法}
        /// </summary>
        using   NotifyChange    =   std::function<void(const Item& ,   const Value*)>;
        using   NotifyChanges   =   FETMultiDelegate<void(const Item& ,const Value*)>;
        using   NotifyArg       =   FEDelegateArg<void(const Item& ,   const Value*)>;
        class   Item
        {
        public:
            using   ItemPtr     =   std::shared_ptr<Item>;
            using   ItemPtrs    =   std::vector<ItemPtr>;
        public:
            Item(const char* name)
                :   _name(name ? name : "")
            {
            }
            Item(const Item&)               =   delete;
            Item&   operator=(const Item&)  =   delete;
            /// <summary>
            /// 添加通知,按 key 覆盖已有绑定
            /// </summary>
            inline  Item&   bind(const void* key,const NotifyChange& notify)
            {
                if (notify && key)
                {
                    auto    obj =   const_cast<void*>(key);
                    _notifys    -=  obj;
                    _notifys    +=  NotifyArg(obj,notify);
                }
                return  *this;
            }
            /// <summary>
            /// 添加通知,支持 {对象,&Class::方法}、lambda、自由函数
            /// </summary>
            inline  Item&   bind(NotifyArg arg)
            {
                if (arg.fn)
                {
                    if (arg.obj)
                        _notifys    -=  arg.obj;
                    _notifys    +=  std::move(arg);
                }
                return  *this;
            }
            /// <summary>
            /// 移除指定对象的所有通知
            /// </summary>
            inline  Item&   unbind(const void* key)
            {
                if (key)
                    _notifys    -=  const_cast<void*>(key);
                return  *this;
            }
            /// <summary>
            /// 清除全部通知
            /// </summary>
            inline  void    clearNotify()
            {
                _notifys.clear();
            }
            /// <summary>
            /// 直接访问多播通知,支持 += {对象,&Class::方法}、-= 对象
            /// </summary>
            inline  auto&   changes()
            {
                return  _notifys;
            }
            inline  auto&   changes() const
            {
                return  _notifys;
            }
            /// <summary>
            /// 获取子节点,不存在则新建;新建时用 defaultValue 初始化其值
            /// </summary>
            inline  Item&   get(const char* name,const Value& defaultValue = Value())
            {
                auto    item    =   find(name);
                if (item == nullptr)
                {
                    auto    child   =   std::make_shared<Item>(name);
                    /// 默认值不是 monostate 时写入新节点,等价于 get(...).setIfUnset(...)
                    if (!std::holds_alternative<std::monostate>(defaultValue))
                        child->_value   =   defaultValue;
                    _childs.emplace_back(child);
                    return  *child;
                }
                return  *item;
            }
            /// <summary>
            /// 查询子节点,不存在返回 nullptr(不会创建)
            /// 返回的指针在父节点销毁或 remove 之前有效
            /// </summary>
            inline  auto    find(const char* name)
            {
                if (name == nullptr)
                    return  (Item*)nullptr;
                for (auto& var : _childs)
                {
                    if (var->_name == name)
                        return  var.get();
                }
                return  (Item*)nullptr;
            }
            const   Item*   find(const char* name) const
            {
                if (name == nullptr)
                    return  nullptr;
                for (auto& var : _childs)
                {
                    if (var->_name == name)
                        return  var.get();
                }
                return  nullptr;
            }
            /// <summary>
            /// 是否存在指定子节点
            /// </summary>
            inline  bool    has(const char* name) const
            {
                return  find(name) != nullptr;
            }
            /// <summary>
            /// 删除子节点(通知随节点一起销毁)
            /// </summary>
            inline  bool    remove(const char* name)
            {
                if (name == nullptr)
                    return  false;
                for (auto itr = _childs.begin(); itr != _childs.end(); ++itr)
                {
                    if ((*itr)->_name == name)
                    {
                        _childs.erase(itr);
                        return  true;
                    }
                }
                return  false;
            }
            /// <summary>
            /// 清空所有子节点
            /// </summary>
            inline  void    clear()
            {
                _childs.clear();
            }
            inline  size_t  size() const
            {
                return  _childs.size();
            }
            inline  auto&   name() const
            {
                return  _name;
            }
            /// <summary>
            /// 原始值(用于序列化)
            /// </summary>
            inline  auto&   raw() const
            {
                return  _value;
            }
            
            /// <summary>
            /// 只读访问第 index 个子节点,越界返回 nullptr
            /// </summary>
           const   Item*    childAt(size_t index) const
            {
                return  index < _childs.size() ? _childs[index].get() : nullptr;
            }
            /// <summary>
            /// 只读获取值,未设置或类型不符返回 nullptr,无副作用
            /// </summary>
            template<class TVal>
            const   TVal*    valuePtr() const
            {
                static_assert(!std::is_same_v<TVal, const char*> && !std::is_same_v<TVal, char*>, "字符串请使用 String");
                return  std::get_if<TVal>(&_value);
            }
            /// <summary>
            /// 只读获取值,未设置或类型不符返回默认值
            /// </summary>
            template<class TVal>
            inline  TVal    value(const TVal& def = TVal()) const
            {
                static_assert(!std::is_same_v<TVal, const char*> && !std::is_same_v<TVal, char*>, "字符串请使用 String");
                auto    pVal    =   std::get_if<TVal>(&_value);
                return  pVal    ?   *pVal   :   def;
            }
            /// <summary>
            /// 设置值,类型在首次写入时确定;之后类型不符仅 Debug 断言,Release 忽略本次写入
            /// </summary>
            template<class TVal>
            inline  Item&   set(const TVal& val)
            {
                static_assert(!std::is_array_v<TVal>
                            && !std::is_same_v<TVal, const char*> && !std::is_same_v<TVal, char*>
                            , "字符串请使用 set(const char*) 或 set(const String&)");
                /// 说明还没有初始化
                if (std::holds_alternative<std::monostate>(_value))
                {
                    _value  =   val;
                    fire(nullptr);
                    return  *this;
                }
                auto    pOld    =   std::get_if<TVal>(&_value);
                if (pOld == nullptr)
                {
                    assert(false && "FESetting: value type mismatch");
                    return  *this;
                }
                if (sameValue(*pOld,val))
                    return  *this;
                /// 没有监听者时不需要保存旧值
                if (_notifys.size() == 0)
                {
                    *pOld   =   val;
                    return  *this;
                }
                Value   old     =   _value;
                        *pOld   =   val;
                fire(&old);
                return  *this;
            }
            /// <summary>
            /// 字符串重载,避免 const char* 被 variant 解释成 bool
            /// </summary>
            inline  Item&   set(const char* val)
            {
                return  set(String(val ? val : ""));
            }
            /// <summary>
            /// 反序列化专用:直接覆盖值,允许改变已有类型(跳过 set 的类型锁定)
            /// </summary>
            template<class TVal>
            inline  Item&   setUnchecked(const TVal& val)
            {
                _value  =   val;
                return  *this;
            }
            /// <summary>
            /// 未初始化时写入默认值,已初始化则保持原值
            /// </summary>
            template<class TVal>
            inline  Item&   setIfUnset(const TVal& def)
            {
                if (std::holds_alternative<std::monostate>(_value))
                    return  set(def);
                if (!std::holds_alternative<TVal>(_value))
                    assert(false && "FESetting: value type mismatch");
                return  *this;
            }
            inline  Item&   setIfUnset(const char* def)
            {
                return  setIfUnset(String(def ? def : ""));
            }
        protected:
            /// <summary>
            /// 触发通知:先拷贝通知表,允许回调中 bind/unbind;同一 Item 的重入 set 通知会被丢弃
            /// </summary>
            inline  void    fire(const Value* old) const
            {
                if (_notifying || _notifys.size() == 0)
                    return;
                _notifying  =   true;
                NotifyChanges   notifys =   _notifys;
                notifys(*this,old);
                _notifying  =   false;
            }
            /// <summary>
            /// 值等价判断:可平凡复制类型按字节比较,避免浮点 NaN 永不相等、-0.0 与 0.0 被误判相等
            /// </summary>
            template<class TVal>
            static  bool    sameValue(const TVal& lhs,const TVal& rhs)
            {
                if constexpr (std::is_trivially_copyable_v<TVal>)
                    return  std::memcmp(&lhs,&rhs,sizeof(TVal)) == 0;
                else
                    return  lhs == rhs;
            }
            mutable bool    _notifying  =   false;
            Value           _value;
            String          _name;
            ItemPtrs        _childs;
            NotifyChanges   _notifys;
        };
        using   ItemPtr     =   std::shared_ptr<Item>;
        using   ItemPtrs    =   std::vector<ItemPtr>;
    public:
        FESetting()
        {
            _root   =   std::make_shared<Item>("");
        }
        FESetting(const FESetting&)             =   delete;
        FESetting&  operator=(const FESetting&) =   delete;
        inline  Item&   get(const char* name,const Value& defaultValue = Value())
        {
            return  _root->get(name,defaultValue);
        }
        /// <summary>
        /// 根节点(供通用遍历/属性面板等适配器使用)
        /// </summary>
        inline  auto    root()
        {
            return  _root.get();
        }
        const   Item*   root() const
        {
            return  _root.get();
        }
        inline  Item*   find(const char* name)
        {
            return  _root->find(name);
        }
        const   Item*   find(const char* name) const
        {
            return  _root->find(name);
        }
        inline  bool    has(const char* name) const
        {
            return  _root->has(name);
        }
        inline  bool    remove(const char* name)
        {
            return  _root->remove(name);
        }
        inline  void    clear()
        {
            _root->clear();
        }
        /// <summary>
        /// 保存为 XML 文件
        /// </summary>
        bool    save(const std::filesystem::path& path) const;
        /// <summary>
        /// 从 XML 文件加载,已存在的节点保留,值按 type 覆盖(允许改变已有类型)
        /// </summary>
        bool    load(const std::filesystem::path& path);
    protected:
        ItemPtr     _root;
    };

    using   Setting  =   SharedPtr<FESetting>;
}
