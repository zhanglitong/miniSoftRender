#pragma     once
#include    <cstdio>
#include    <cstring>
#include    <fstream>
#include    <functional>
#include    <iterator>
#include    <sstream>
#include    <variant>
#include    "FEDefine.h"
#include    "FEDelegate.hpp"
#include    "FEMath.hpp"
#include    "FEXML.hpp"

namespace   FE
{
    /// <summary>
    /// 分层配置容器:每个节点持有一个 variant 值、若干子节点和一组变更通知。
    /// 值的类型在首次写入时确定,之后不允许改变。
    /// </summary>
    class   FESetting
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
        /// 当前item,以及历史值,如果第二个为nullptr,则是第一次设置
        /// 回调中可以安全地 bind/unbind;对同一个 Item 的重入 set 通知会被丢弃,避免递归
        /// 注册方式:bind(key,fn)、bind({对象,&Class::方法}) 或 changes() += {对象,&Class::方法}
        /// </summary>
        using   NotifyChange    =   std::function<void(const Item& Item,const Value*)>;
        using   NotifyChanges   =   FETMultiDelegate<void(const Item& Item,const Value*)>;
        using   NotifyArg       =   FEDelegateArg<void(const Item& Item,const Value*)>;
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
            /// 移除通知
            /// </summary>
            inline  Item&   unbind(const void* key)
            {
                if (key)
                    _notifys    -=  const_cast<void*>(key);
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
            /// 获取，如果没有，添加
            /// </summary>
            inline  Item&   get(const char* name)
            {
                auto    item    =   find(name);
                if (item == nullptr)
                {
                    auto    child   =   std::make_shared<Item>(name);
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
            inline  auto    find(const char* name) const
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
            inline  auto&   children() const
            {
                return  _childs;
            }
            /// <summary>
            /// 只读获取值,未设置或类型不符返回 nullptr,无副作用
            /// </summary>
            template<class TVal>
            const TVal*     value() const
            {
                static_assert(!std::is_same_v<TVal, const char*> && !std::is_same_v<TVal, char*>
                            , "字符串请使用 String");
                return  std::get_if<TVal>(&_value);
            }
            /// <summary>
            /// 只读获取值,未设置或类型不符返回默认值
            /// </summary>
            template<class TVal>
            inline  TVal    getOr(const TVal& def) const
            {
                static_assert(!std::is_same_v<TVal, const char*> && !std::is_same_v<TVal, char*>
                            , "字符串请使用 String");
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
                if ((*pOld) == val)
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
            /// 触发通知:先拷贝通知表,允许回调中 bind/unbind
            /// </summary>
            inline  void    fire(const Value* old) const
            {
                if (_notifying || _notifys.size() == 0)
                    return;
                struct  Guard
                {
                    bool&   _flag;
                    Guard(bool& flag)
                        :   _flag(flag)
                    {
                        _flag   =   true;
                    }
                    ~Guard()
                    {
                        _flag   =   false;
                    }
                };
                Guard           guard(_notifying);
                NotifyChanges   notifys =   _notifys;
                notifys(*this,old);
            }
        protected:
            Value           _value;
            String          _name;
            NotifyChanges   _notifys;
            ItemPtrs        _childs;
            mutable bool    _notifying  =   false;
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
        inline  Item&   get(const char* name)
        {
            return  _root->get(name);
        }
        /// <summary>
        /// 根节点(供通用遍历/属性面板等适配器使用)
        /// </summary>
        inline  auto    root()
        {
            return  _root.get();
        }
        inline  auto    root() const
        {
            return  _root.get();
        }
        inline  auto    find(const char* name) const
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
        inline  bool    save(const char* path) const
        {
            if (path == nullptr)
                return  false;

            XMLDoc      doc;
            XMLNode*    decl    =   doc.allocate_node(rapidxml::node_pi,"xml version='1.0' encoding='utf-8'");
            doc.append_node(decl);

            XMLNode*    root    =   doc.allocate_node(rapidxml::node_element,"setting");
            doc.append_node(root);
            saveItems(doc,root,*_root);

            std::ofstream   file(path,std::ios::binary | std::ios::trunc);
            if (!file.is_open())
                return  false;
            file << doc;
            return  file.good();
        }
        /// <summary>
        /// 从 XML 文件加载,已存在的节点保留,值按 type 覆盖
        /// </summary>
        inline  bool    load(const char* path)
        {
            if (path == nullptr)
                return  false;

            std::ifstream   file(path,std::ios::binary);
            if (!file.is_open())
                return  false;

            std::vector<char>   buffer((std::istreambuf_iterator<char>(file))
                                      ,std::istreambuf_iterator<char>());
            if (buffer.empty())
                return  false;
            buffer.push_back('\0');

            XMLDoc      doc;
            try
            {
                doc.parse<0>(buffer.data());
            }
            catch (...)
            {
                return  false;
            }

            XMLNode*    root    =   doc.first_node("setting");
            if (root == nullptr)
                return  false;
            loadItems(*_root,root);
            return  true;
        }
    protected:
        ItemPtr     _root;
    protected:
        /// <summary>
        /// 写 XML 属性,值拷贝进文档内存池
        /// </summary>
        inline  static  void    setAttr(XMLDoc& doc,XMLNode* node,const char* name,const char* value)
        {
            node->append_attribute(doc.allocate_attribute(name,doc.allocate_string(value)));
        }
        /// <summary>
        /// 标量转字符串
        /// </summary>
        template<class T>
        inline  static  String  scalarToString(T val)
        {
            if constexpr (std::is_same_v<T,float>)
            {
                char    buf[32] =   {};
                snprintf(buf,sizeof(buf),"%.9g",(double)val);
                return  buf;
            }
            else if constexpr (std::is_same_v<T,double>)
            {
                char    buf[32] =   {};
                snprintf(buf,sizeof(buf),"%.17g",val);
                return  buf;
            }
            else
            {
                return  std::to_string(val);
            }
        }
        /// <summary>
        /// 向量转空格分隔字符串
        /// </summary>
        template<class T>
        inline  static  String  vecToString(const T& val)
        {
            String  result;
            for (typename T::length_type i = 0; i < T::length(); ++i)
            {
                if (i != 0)
                    result.push_back(' ');
                result.append(scalarToString(val[i]));
            }
            return  result;
        }
        /// <summary>
        /// 整数类型名
        /// </summary>
        template<class T>
        inline  static  auto    intTypeName()
        {
            if constexpr (std::is_same_v<T,int32>)
                return  "int32";
            else if constexpr (std::is_same_v<T,uint32>)
                return  "uint32";
            else if constexpr (std::is_same_v<T,int64>)
                return  "int64";
            else
                return  "uint64";
        }
        /// <summary>
        /// 向量类型名
        /// </summary>
        template<class T>
        inline  static  auto    vecTypeName()
        {
            if constexpr (std::is_same_v<T,int2>)       return  "int2";
            else if constexpr (std::is_same_v<T,int3>)  return  "int3";
            else if constexpr (std::is_same_v<T,int4>)  return  "int4";
            else if constexpr (std::is_same_v<T,uint2>) return  "uint2";
            else if constexpr (std::is_same_v<T,uint3>) return  "uint3";
            else if constexpr (std::is_same_v<T,uint4>) return  "uint4";
            else if constexpr (std::is_same_v<T,float2>)return  "float2";
            else if constexpr (std::is_same_v<T,float3>)return  "float3";
            else if constexpr (std::is_same_v<T,float4>)return  "float4";
            else if constexpr (std::is_same_v<T,real2>) return  "real2";
            else if constexpr (std::is_same_v<T,real3>) return  "real3";
            else if constexpr (std::is_same_v<T,real4>) return  "real4";
            else                                        return  "vec";
        }
        /// <summary>
        /// 值写入节点:type 描述类型,value 保存文本
        /// </summary>
        inline  static  void    writeValue(XMLDoc& doc,XMLNode* node,const Value& value)
        {
            std::visit([&](const auto& arg)
            {
                using   T   =   std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T,std::monostate>)
                {
                    return;
                }
                else if constexpr (std::is_same_v<T,bool>)
                {
                    setAttr(doc,node,"type","bool");
                    setAttr(doc,node,"value",arg ? "true" : "false");
                }
                else if constexpr (std::is_same_v<T,String>)
                {
                    setAttr(doc,node,"type","string");
                    setAttr(doc,node,"value",arg.c_str());
                }
                else if constexpr (std::is_integral_v<T>)
                {
                    setAttr(doc,node,"type",intTypeName<T>());
                    String  text    =   scalarToString(arg);
                    setAttr(doc,node,"value",text.c_str());
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    setAttr(doc,node,"type",std::is_same_v<T,float> ? "float" : "double");
                    String  text    =   scalarToString(arg);
                    setAttr(doc,node,"value",text.c_str());
                }
                else
                {
                    setAttr(doc,node,"type",vecTypeName<T>());
                    String  text    =   vecToString(arg);
                    setAttr(doc,node,"value",text.c_str());
                }
            },value);
        }
        /// <summary>
        /// 文本转标量
        /// </summary>
        template<class T>
        inline  static  bool    scalarFromString(const char* text,T& out)
        {
            std::istringstream  stream(text != nullptr ? text : "");
            return  static_cast<bool>(stream >> out);
        }
        /// <summary>
        /// 空格分隔文本转向量
        /// </summary>
        template<class T>
        inline  static  bool    vecFromString(const char* text,T& out)
        {
            std::istringstream  stream(text != nullptr ? text : "");
            for (typename T::length_type i = 0; i < T::length(); ++i)
            {
                if (!(stream >> out[i]))
                    return  false;
            }
            return  true;
        }
        inline  static  bool    readBool(Item& item,const char* text)
        {
            item.set<bool>(text != nullptr
                && (std::strcmp(text,"1") == 0 || std::strcmp(text,"true") == 0));
            return  true;
        }
        inline  static  bool    readString(Item& item,const char* text)
        {
            item.set(text != nullptr ? text : "");
            return  true;
        }
        template<class T>
        inline  static  bool    readScalar(Item& item,const char* text)
        {
            T   val =   {};
            if (!scalarFromString(text,val))
                return  false;
            item.set<T>(val);
            return  true;
        }
        template<class T>
        inline  static  bool    readVec(Item& item,const char* text)
        {
            T   val =   {};
            if (!vecFromString(text,val))
                return  false;
            item.set<T>(val);
            return  true;
        }
        /// <summary>
        /// type 属性到读取器的映射
        /// </summary>
        struct  Reader
        {
            const char* _type;
            bool        (*_read)(Item& item,const char* text);
        };
        inline  static  bool    applyValue(Item& item,const char* type,const char* text)
        {
            if (type == nullptr)
                return  false;
            static  const   Reader  table[] =
            {
                { "bool",   readBool            },
                { "string", readString          },
                { "int32",  readScalar<int32>   },
                { "uint32", readScalar<uint32>  },
                { "int64",  readScalar<int64>   },
                { "uint64", readScalar<uint64>  },
                { "float",  readScalar<float>   },
                { "double", readScalar<double>  },
                { "int2",   readVec<int2>       },
                { "int3",   readVec<int3>       },
                { "int4",   readVec<int4>       },
                { "uint2",  readVec<uint2>      },
                { "uint3",  readVec<uint3>      },
                { "uint4",  readVec<uint4>      },
                { "float2", readVec<float2>     },
                { "float3", readVec<float3>     },
                { "float4", readVec<float4>     },
                { "real2",  readVec<real2>      },
                { "real3",  readVec<real3>      },
                { "real4",  readVec<real4>      },
            };
            for (const Reader& var : table)
            {
                if (std::strcmp(var._type,type) == 0)
                    return  var._read(item,text);
            }
            return  false;
        }
        /// <summary>
        /// 递归写出子节点
        /// </summary>
        inline  static  void    saveItems(XMLDoc& doc,XMLNode* parent,const Item& item)
        {
            for (auto& var : item.children())
            {
                XMLNode*    node    =   doc.allocate_node(rapidxml::node_element,"item");
                parent->append_node(node);
                setAttr(doc,node,"name",var->name().c_str());
                writeValue(doc,node,var->raw());
                saveItems(doc,node,*var);
            }
        }
        /// <summary>
        /// 递归读入子节点,已存在的节点复用
        /// </summary>
        inline  static  void    loadItems(Item& parent,XMLNode* node)
        {
            if (node == nullptr)
                return;
            for (XMLNode* var = node->first_node("item"); var != nullptr; var = var->next_sibling("item"))
            {
                XMLAttr*    name    =   var->first_attribute("name");
                if (name == nullptr)
                    continue;
                Item&       item    =   parent.get(name->value());
                XMLAttr*    type    =   var->first_attribute("type");
                XMLAttr*    value   =   var->first_attribute("value");
                if (type != nullptr && value != nullptr)
                    applyValue(item,type->value(),value->value());
                loadItems(item,var);
            }
        }
    };

    using   Setting  =   SharedPtr<FESetting>;
}
