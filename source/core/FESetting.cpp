
#include    "../inc/FESetting.hpp"
#include    "../inc/FEXML.hpp"

#include    <cstring>
#include    <filesystem>
#include    <fstream>
#include    <iterator>
#include    <limits>
#include    <locale>
#include    <sstream>

namespace   FE
{
    /// <summary>
    /// 持久化辅助:仅本文件使用,XML/IO 相关依赖不再进入公共头
    /// </summary>
    namespace
    {
        using   Item    =   FESetting::Item;
        using   Value   =   FESetting::Value;

        /// <summary>
        /// 写 XML 属性,值拷贝进文档内存池
        /// (属性值的 & < > " ' 由 rapidxml_print 在输出时统一转义,这里不能重复转义)
        /// </summary>
        void    setAttr(XMLDoc& doc,XMLNode* node,const char* name,const char* value)
        {
            node->append_attribute(doc.allocate_attribute(name,doc.allocate_string(value)));
        }
        /// <summary>
        /// 标量转字符串:强制经典 locale,浮点用 max_digits10 保证可精确往返,
        /// 结果与宿主 setlocale 无关
        /// </summary>
        template<class T>
        String  scalarToString(T val)
        {
            std::ostringstream  stream;
            stream.imbue(std::locale::classic());
            if constexpr (std::is_floating_point_v<T>)
                stream.precision(std::numeric_limits<T>::max_digits10);
            stream << val;
            return  stream.str();
        }
        /// <summary>
        /// 向量转空格分隔字符串
        /// </summary>
        template<class T>
        String  vecToString(const T& val)
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
        auto    intTypeName()
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
        auto    vecTypeName()
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
        /// 下拉选项编码:选项之间用 '|' 分隔,选项内的 '\' 与 '|' 前加 '\'
        /// </summary>
        String  comboOptionsToString(const std::vector<String>& options)
        {
            String  result;
            for (size_t i = 0; i < options.size(); ++i)
            {
                if (i != 0)
                    result.push_back('|');
                for (char c : options[i])
                {
                    if (c == '\\' || c == '|')
                        result.push_back('\\');
                    result.push_back(c);
                }
            }
            return  result;
        }
        /// <summary>
        /// 值写入节点:type 描述类型,value 保存文本
        /// </summary>
        void    writeValue(XMLDoc& doc,XMLNode* node,const Value& value)
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
                else if constexpr (std::is_same_v<T,FESetting::Combo>)
                {
                    setAttr(doc,node,"type","combo");
                    String  text    =   scalarToString<int32>(arg._index);
                    setAttr(doc,node,"value",text.c_str());
                    String  options =   comboOptionsToString(arg._options);
                    setAttr(doc,node,"options",options.c_str());
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
        /// 文本转标量:强制经典 locale,避免宿主 locale 的小数点差异
        /// </summary>
        template<class T>
        bool    scalarFromString(const char* text,T& out)
        {
            std::istringstream  stream(text != nullptr ? text : "");
            stream.imbue(std::locale::classic());
            return  static_cast<bool>(stream >> out);
        }
        /// <summary>
        /// 空格分隔文本转向量
        /// </summary>
        template<class T>
        bool    vecFromString(const char* text,T& out)
        {
            std::istringstream  stream(text != nullptr ? text : "");
            stream.imbue(std::locale::classic());
            for (typename T::length_type i = 0; i < T::length(); ++i)
            {
                if (!(stream >> out[i]))
                    return  false;
            }
            return  true;
        }
        bool    readBool(Item& item,const char* text)
        {
            item.setUnchecked<bool>(text != nullptr
                && (std::strcmp(text,"1") == 0 || std::strcmp(text,"true") == 0));
            return  true;
        }
        bool    readString(Item& item,const char* text)
        {
            item.setUnchecked(String(text != nullptr ? text : ""));
            return  true;
        }
        template<class T>
        bool    readScalar(Item& item,const char* text)
        {
            T   val =   {};
            if (!scalarFromString(text,val))
                return  false;
            item.setUnchecked<T>(val);
            return  true;
        }
        template<class T>
        bool    readVec(Item& item,const char* text)
        {
            T   val =   {};
            if (!vecFromString(text,val))
                return  false;
            item.setUnchecked<T>(val);
            return  true;
        }
        /// <summary>
        /// 读取下拉框:index 来自 value,选项来自 options(空表示没有候选项)
        /// </summary>
        bool    readCombo(Item& item,const char* text,const char* options)
        {
            FESetting::Combo    combo;
            if (!scalarFromString(text,combo._index))
                return  false;
            if (options != nullptr && *options != '\0')
            {
                String  current;
                for (const char* c = options; *c != '\0'; ++c)
                {
                    if (*c == '\\' && (c[1] == '\\' || c[1] == '|'))
                    {
                        current.push_back(c[1]);
                        ++c;
                    }
                    else if (*c == '|')
                    {
                        combo._options.emplace_back(current);
                        current.clear();
                    }
                    else
                        current.push_back(*c);
                }
                combo._options.emplace_back(current);
            }
            item.setUnchecked(combo);
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
        bool    applyValue(Item& item,const char* type,const char* text,const char* options)
        {
            if (type == nullptr)
                return  false;
            if (std::strcmp(type,"combo") == 0)
                return  readCombo(item,text,options);
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
        /// 配置树最大嵌套层数,防止损坏或恶意 XML 递归过深导致栈溢出
        /// </summary>
        constexpr int   kMaxDepth   =   64;

        /// <summary>
        /// 递归写出子节点
        /// </summary>
        void    saveItems(XMLDoc& doc,XMLNode* parent,const Item& item,int depth)
        {
            if (depth > kMaxDepth)
                return;
            for (size_t i = 0; i < item.size(); ++i)
            {
                const   Item*   var =   item.childAt(i);
                XMLNode*    node    =   doc.allocate_node(rapidxml::node_element,"item");
                parent->append_node(node);
                setAttr(doc,node,"name",var->name().c_str());
                writeValue(doc,node,var->raw());
                saveItems(doc,node,*var,depth + 1);
            }
        }
        /// <summary>
        /// 递归读入子节点,已存在的节点复用;超过深度上限视为失败
        /// </summary>
        bool    loadItems(Item& parent,XMLNode* node,int depth)
        {
            if (node == nullptr)
                return  true;
            if (depth > kMaxDepth)
                return  false;
            bool    result  =   true;
            for (XMLNode* var = node->first_node("item"); var != nullptr; var = var->next_sibling("item"))
            {
                XMLAttr*    name    =   var->first_attribute("name");
                if (name == nullptr)
                    continue;
                Item&       item    =   parent.get(name->value());
                XMLAttr*    type    =   var->first_attribute("type");
                XMLAttr*    value   =   var->first_attribute("value");
                XMLAttr*    options =   var->first_attribute("options");
                if (type != nullptr && value != nullptr)
                    applyValue(item,type->value(),value->value(),options != nullptr ? options->value() : nullptr);
                if (!loadItems(item,var,depth + 1))
                    result  =   false;
            }
            return  result;
        }
    }

    bool    FESetting::save(const std::filesystem::path& path) const
    {
        if (path.empty())
            return  false;

        XMLDoc      doc;
        XMLNode*    decl    =   doc.allocate_node(rapidxml::node_pi,"xml version='1.0' encoding='utf-8'");
        doc.append_node(decl);

        XMLNode*    root    =   doc.allocate_node(rapidxml::node_element,"setting");
        doc.append_node(root);
        saveItems(doc,root,*_root,0);

        /// 先写临时文件再改名,避免写到一半失败把原配置清空
        std::filesystem::path   temp    =   path;
        temp    +=  ".tmp";

        std::ofstream   file(temp,std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return  false;
        file << doc;
        file.close();
        if (file.fail())
        {
            std::error_code ec;
            std::filesystem::remove(temp,ec);
            return  false;
        }

        std::error_code ec;
        std::filesystem::rename(temp,path,ec);
        if (ec)
        {
            std::filesystem::remove(temp,ec);
            return  false;
        }
        return  true;
    }

    bool    FESetting::load(const std::filesystem::path& path)
    {
        if (path.empty())
            return  false;

        std::ifstream   file(path,std::ios::binary);
        if (!file.is_open())
            return  false;

        std::vector<char>   buffer((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
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
        return  loadItems(*_root,root,0);
    }
}
