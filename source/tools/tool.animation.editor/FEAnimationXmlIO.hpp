#pragma     once

#include    <string>
#include    <vector>
#include    <fstream>
#include    <sstream>
#include    <cstdio>
#include    <cctype>
#include    <algorithm>

#include    "xml/rapidxml.hpp"
#include    "xml/rapidxml_print.hpp"
#include    "FEUuid.h"
#include    "animation/FEAnimation.hpp"
#include    "animation/FEAnimClip.hpp"
#include    "animation/FEKeyFrameTrack.hpp"

namespace   FE
{
    //////////////////////////////////////////////////////////////////////////////////
    // 节点动画 XML 导入/导出(移植自 editorV3 CELLAnimationHelper,适配 FE 值对象变体)
    // 格式:
    // <anims version="1">
    //   <anim objectId name offset owner>
    //     <clip objectId>
    //       <track objectId name prop type interp tension>
    //         <key t="0.5">x,y,z</key>
    //       </track>
    //     </clip>
    //   </anim>
    // </anims>
    //////////////////////////////////////////////////////////////////////////////////
    class   FEAnimationXmlIO
    {
    public:
        /// <summary>
        /// 导出条目: 动画组件 + 所属节点 objectId(写入 anim 的 owner 属性)
        /// </summary>
        struct  AnimExportItem
        {
            const FEAnimation*  anim    =   nullptr;
            FEUuid              owner;
        };
        /// <summary>
        /// 导入条目: 新建动画 + 记录的原所属节点 objectId
        /// hasOwner=false 表示 XML 无 owner 属性(旧文件),由调用方决定挂接位置
        /// </summary>
        struct  AnimImportItem
        {
            Animation       anim;
            FEUuid          owner;
            bool            hasOwner    =   false;
        };

    public:
        /// <summary>
        /// 导出一组动画到 XML 文件(动画组件 → clip → 轨道 → 关键帧全量序列化)
        /// </summary>
        static  bool    saveAnimsToXml(const std::vector<AnimExportItem>& anims, const std::string& path, std::string* err = nullptr)
        {
            try
            {
                rapidxml::xml_document<>    doc;
                auto*   root    =   doc.allocate_node(rapidxml::node_element, "anims");
                Detail::addAttr(doc, root, "version", "1");
                doc.append_node(root);
                for (auto& item : anims)
                {
                    auto*   anim    =   item.anim;
                    if (anim == nullptr)
                        continue;
                    auto*   animNode    =   doc.allocate_node(rapidxml::node_element, "anim");
                    Detail::writeIdAttr(doc, animNode, *anim);
                    Detail::addAttr(doc, animNode, "name", std::string(anim->name()));
                    Detail::addAttr(doc, animNode, "offset", Detail::numToText(double(anim->offset())));
                    /// 所属节点 objectId,导入时按此在目标子树中还原挂接
                    Detail::addAttr(doc, animNode, "owner", item.owner.toString());
                    root->append_node(animNode);

                    auto*   clipNode    =   doc.allocate_node(rapidxml::node_element, "clip");
                    animNode->append_node(clipNode);
                    auto    clip        =   anim->clip();
                    if (clip)
                    {
                        Detail::writeIdAttr(doc, clipNode, *clip);
                        for (auto& track : clip->tracks())
                        {
                            if (track == nullptr)
                                continue;
                            auto*   trackNode   =   doc.allocate_node(rapidxml::node_element, "track");
                            Detail::writeIdAttr(doc, trackNode, *track);
                            Detail::addAttr(doc, trackNode, "name", std::string(track->name()));
                            Detail::addAttr(doc, trackNode, "prop", std::to_string(track->propertyIndex()));
                            Detail::addAttr(doc, trackNode, "interp", std::to_string(int(track->_type)));
                            Detail::addAttr(doc, trackNode, "tension", Detail::numToText(double(track->_tension)));
                            auto    times       =   track->times();
                            std::visit([&](auto&& objPtr)
                            {
                                using   P   =   std::decay_t<decltype(objPtr)>;
                                if constexpr (std::is_same_v<P, std::monostate>)
                                {
                                    Detail::addAttr(doc, trackNode, "type", "none");
                                }
                                else
                                {
                                    Detail::addAttr(doc, trackNode, "type", Detail::valueTypeName<P>());
                                    const auto& vals    =   objPtr->values();
                                    size_t      count   =   times ? (std::min)(times->values().size(), vals.size()) : 0;
                                    for (size_t i = 0; i < count; ++i)
                                    {
                                        std::string text    =   Detail::elemToText(vals[i]);
                                        auto*   keyNode =   doc.allocate_node(rapidxml::node_element, "key"
                                            , doc.allocate_string(text.c_str()));
                                        Detail::addAttr(doc, keyNode, "t"
                                            , Detail::numToText(double(times->values()[i])));
                                        trackNode->append_node(keyNode);
                                    }
                                }
                            }, track->values());
                            clipNode->append_node(trackNode);
                        }
                    }
                }
                std::ostringstream  oss;
                oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                std::ostream&       os  =   oss;
                rapidxml::print(os, *root, 0);

                std::ofstream   file(path, std::ios::binary | std::ios::trunc);
                if (!file.is_open())
                {
                    if (err) *err   =   std::string(u8"无法打开文件: ") + path;
                    return  false;
                }
                std::string content =   oss.str();
                file.write(content.data(), std::streamsize(content.size()));
                file.close();
                if (!file.good())
                {
                    if (err) *err   =   std::string(u8"写入文件失败: ") + path;
                    return  false;
                }
                return  true;
            }
            catch (const std::exception& e)
            {
                if (err) *err   =   std::string(u8"写入 XML 失败: ") + e.what();
                return  false;
            }
        }

        /// <summary>
        /// 从 XML 文件导入一组动画(新建动画对象,尚未挂接任何节点,由调用方挂接)
        /// </summary>
        static  std::vector<AnimImportItem>  loadAnimsFromXml(FEContext& ctx, const std::string& path, std::string* err = nullptr)
        {
            std::vector<AnimImportItem>  result;
            try
            {
                std::ifstream   file(path, std::ios::binary);
                if (!file.is_open())
                {
                    if (err) *err   =   std::string(u8"无法打开文件: ") + path;
                    return  result;
                }
                /// rapidxml 原地解析,缓冲区生命周期必须覆盖整次解析
                std::ostringstream  mem;
                mem << file.rdbuf();
                file.close();
                std::string content =   mem.str();
                std::vector<char>   buffer(content.begin(), content.end());
                buffer.push_back('\0');

                rapidxml::xml_document<>    doc;
                doc.parse<0>(buffer.data());
                auto*   root    =   doc.first_node("anims");
                if (root == nullptr)
                {
                    if (err) *err   =   u8"不是有效的动画 XML(缺少 anims 根节点)";
                    return  result;
                }
                for (auto* animNode = root->first_node("anim"); animNode; animNode = animNode->next_sibling("anim"))
                {
                    AnimImportItem  entry;
                    auto    anim    =   new FEAnimation(ctx);
                    entry.anim      =   anim;
                    /// 自身 objectId(跨文件保持身份稳定),缺失/非法时保留构造新 id
                    if (auto idOk = Detail::readIdAttr(animNode); idOk.second)
                        anim->setObjectId(idOk.first);
                    /// owner 属性可选(兼容旧文件),存在时解析为原所属节点 objectId
                    if (auto* ownerAttr = animNode->first_attribute("owner"))
                    {
                        bool    ok  =   false;
                        entry.owner     =   FEUuid::from(std::string(ownerAttr->value(), ownerAttr->value_size()).c_str(), &ok);
                        entry.hasOwner  =   ok && !(entry.owner == FEUuid::zero());
                    }
                    anim->setName(Detail::attrVal(animNode, "name"));
                    anim->setOffset(real(std::stod(Detail::attrVal(animNode, "offset", "0"))));
                    auto    clip    =   new FEAnimClip(ctx);
                    anim->setClip(clip);

                    auto*   clipNode    =   animNode->first_node("clip");
                    if (clipNode)
                    {
                        if (auto idOk = Detail::readIdAttr(clipNode); idOk.second)
                            clip->setObjectId(idOk.first);
                        for (auto* trackNode = clipNode->first_node("track"); trackNode; trackNode = trackNode->next_sibling("track"))
                        {
                            std::string typeName    =   Detail::attrVal(trackNode, "type");
                            int         propIdx     =   std::stoi(Detail::attrVal(trackNode, "prop", "-1"));
                            int         interp      =   std::stoi(Detail::attrVal(trackNode, "interp", "1"));
                            double      tension     =   std::stod(Detail::attrVal(trackNode, "tension", "0.4"));

                            std::vector<Detail::KeyRaw>    keys;
                            for (auto* keyNode = trackNode->first_node("key"); keyNode; keyNode = keyNode->next_sibling("key"))
                            {
                                Detail::KeyRaw  k;
                                k.time  =   real(std::stod(Detail::attrVal(keyNode, "t", "0")));
                                k.comps =   Detail::splitCsv(
                                    std::string(keyNode->value(), keyNode->value_size()));
                                keys.push_back(std::move(k));
                            }

                            KeyFrameTrack   track   =   new FEKeyFrameTrack(ctx, propIdx);
                            if (auto idOk = Detail::readIdAttr(trackNode); idOk.second)
                                track->setObjectId(idOk.first);
                            track->setName(Detail::attrVal(trackNode, "name"));
                            track->_type      =   FEKeyFrameTrack::InterpolateType(interp);
                            track->_tension   =   real(tension);
                            if (typeName != "none" && !keys.empty())
                            {
                                RealsObject times   =   new FERealsObject(ctx);
                                for (auto& k : keys)
                                    times->values().push_back(k.time);
                                track->setTimeObject(times);
                                track->setValueObject(Detail::buildValueObject(ctx, typeName, keys));
                            }
                            clip->addTrack(track);
                        }
                    }
                    result.push_back(std::move(entry));
                }
            }
            catch (const rapidxml::parse_error& e)
            {
                if (err) *err   =   std::string(u8"解析 XML 失败: ") + e.what();
                return  {};
            }
            catch (const std::exception& e)
            {
                if (err) *err   =   std::string(u8"导入失败: ") + e.what();
                return  {};
            }
            return  result;
        }

    private:
        //////////////////////////////////////////////////////////////////////////////////
        // XML 序列化内部工具(值对象类型名与元素文本互转)
        //////////////////////////////////////////////////////////////////////////////////
        struct  Detail
        {
            /// <summary>
            /// 高精度浮点转字符串: %.17g 保证 double 往返精度
            /// </summary>
            static  std::string numToText(double v)
            {
                char    buf[64] =   {0};
                std::snprintf(buf, sizeof(buf), "%.17g", v);
                return  buf;
            }

            /// <summary>
            /// 关键帧元素转逗号分隔文本:
            /// 整型逐值输出;浮点 %.17g;向量/四元数按 x,y,z,w 连续布局输出
            /// </summary>
            template<class T>
            static  std::string elemToText(const T& v)
            {
                if constexpr (std::is_integral_v<T>)
                {
                    if constexpr (std::is_signed_v<T>)
                        return  std::to_string(static_cast<long long>(v));
                    else
                        return  std::to_string(static_cast<unsigned long long>(v));
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    return  numToText(double(v));
                }
                else
                {
                    /// tvec2/3/4、tquat: x,y,z,w 连续布局(引擎 operator[] 实现即 (&x)[i])
                    constexpr int   compN   =   int(sizeof(T) / sizeof(typename T::value_type));
                    const typename T::value_type* p   =   &v.x;
                    std::string s;
                    for (int i = 0; i < compN; ++i)
                    {
                        if (i)
                            s   +=  ',';
                        if constexpr (std::is_integral_v<typename T::value_type>)
                            s   +=  std::to_string(static_cast<long long>(p[i]));
                        else
                            s   +=  numToText(double(p[i]));
                    }
                    return  s;
                }
            }

            /// <summary>
            /// 值数组对象 Ptr 类型 → XML 类型名(monostate 返回空串)
            /// 注意:BoolsObject(显隐) 与 AlphaObject(透明度) 元素都是 uint8,用不同类型名区分
            /// </summary>
            template<class P>
            static  const char* valueTypeName()
            {
                if constexpr (std::is_same_v<P, RealsObject>)       return  "real";
                else if constexpr (std::is_same_v<P, Real2sObject>) return  "real2";
                else if constexpr (std::is_same_v<P, Real3sObject>) return  "real3";
                else if constexpr (std::is_same_v<P, Real4sObject>) return  "real4";
                else if constexpr (std::is_same_v<P, QuatrsObject>) return  "quatr";
                else if constexpr (std::is_same_v<P, FloatsObject>) return  "float";
                else if constexpr (std::is_same_v<P, Float2sObject>)return  "float2";
                else if constexpr (std::is_same_v<P, Float3sObject>)return  "float3";
                else if constexpr (std::is_same_v<P, Float4sObject>)return  "float4";
                else if constexpr (std::is_same_v<P, QuatfsObject>) return  "quatf";
                else if constexpr (std::is_same_v<P, BoolsObject>)  return  "bool";
                else if constexpr (std::is_same_v<P, AlphaObject>)  return  "alpha";
                else if constexpr (std::is_same_v<P, RgbaObject>)   return  "rgba";
                else if constexpr (std::is_same_v<P, IntsObject>)   return  "int";
                else if constexpr (std::is_same_v<P, Int2sObject>)  return  "int2";
                else if constexpr (std::is_same_v<P, Int3sObject>)  return  "int3";
                else if constexpr (std::is_same_v<P, Int4sObject>)  return  "int4";
                else if constexpr (std::is_same_v<P, UintsObject>)  return  "uint";
                else if constexpr (std::is_same_v<P, Uint2sObject>) return  "uint2";
                else if constexpr (std::is_same_v<P, Uint3sObject>) return  "uint3";
                else if constexpr (std::is_same_v<P, Uint4sObject>) return  "uint4";
                else                                                return  "";
            }

            /// <summary>
            /// 单个关键帧的原始读取结果(时间 + 分量文本)
            /// </summary>
            struct  KeyRaw
            {
                real                        time;
                std::vector<std::string>    comps;
            };

            /// <summary>
            /// 按逗号拆分分量文本(去除首尾空白,跳过空段)
            /// </summary>
            static  std::vector<std::string>    splitCsv(const std::string& text)
            {
                std::vector<std::string>    result;
                size_t  begin   =   0;
                while (begin <= text.size())
                {
                    size_t  comma   =   text.find(',', begin);
                    size_t  end     =   (comma == std::string::npos) ? text.size() : comma;
                    size_t  b       =   begin;
                    size_t  e       =   end;
                    while (b < e && std::isspace((unsigned char)text[b])) ++b;
                    while (e > b && std::isspace((unsigned char)text[e - 1])) --e;
                    if (e > b)
                        result.push_back(text.substr(b, e - b));
                    if (comma == std::string::npos)
                        break;
                    begin   =   comma + 1;
                }
                return  result;
            }

            /// <summary>
            /// 按类型名创建值数组对象并填充关键帧;未知类型/分量不足返回 monostate
            /// </summary>
            static  ValueObject buildValueObject(FEContext& ctx, const std::string& typeName, const std::vector<KeyRaw>& keys)
            {
                auto    d0  =   [&keys](size_t i, size_t c) -> double
                {
                    return  (i < keys.size() && keys[i].comps.size() > c)
                            ? std::stod(keys[i].comps[c]) : 0.0;
                };
                auto    b0  =   [&d0](size_t i, size_t c) -> uint8
                {
                    return  uint8(std::clamp(d0(i, c), 0.0, 255.0));
                };
                if (typeName == "real")
                {
                    auto    o   =   new FERealsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(real(d0(i, 0)));
                    return  RealsObject(o);
                }
                if (typeName == "real2")
                {
                    auto    o   =   new FEReal2sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(real2(real(d0(i,0)), real(d0(i,1))));
                    return  Real2sObject(o);
                }
                if (typeName == "real3")
                {
                    auto    o   =   new FEReal3sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(real3(real(d0(i,0)), real(d0(i,1)), real(d0(i,2))));
                    return  Real3sObject(o);
                }
                if (typeName == "real4")
                {
                    auto    o   =   new FEReal4sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(real4(real(d0(i,0)), real(d0(i,1)), real(d0(i,2)), real(d0(i,3))));
                    return  Real4sObject(o);
                }
                if (typeName == "quatr")
                {
                    auto    o   =   new FEQuatrsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i)
                    {
                        quatr   q;
                        q.x = real(d0(i,0)); q.y = real(d0(i,1)); q.z = real(d0(i,2)); q.w = real(d0(i,3));
                        o->values().push_back(q);
                    }
                    return  QuatrsObject(o);
                }
                if (typeName == "float")
                {
                    auto    o   =   new FEFloatsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(float(d0(i, 0)));
                    return  FloatsObject(o);
                }
                if (typeName == "float2")
                {
                    auto    o   =   new FEFloat2sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(float2(float(d0(i,0)), float(d0(i,1))));
                    return  Float2sObject(o);
                }
                if (typeName == "float3")
                {
                    auto    o   =   new FEFloat3sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(float3(float(d0(i,0)), float(d0(i,1)), float(d0(i,2))));
                    return  Float3sObject(o);
                }
                if (typeName == "float4")
                {
                    auto    o   =   new FEFloat4sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(float4(float(d0(i,0)), float(d0(i,1)), float(d0(i,2)), float(d0(i,3))));
                    return  Float4sObject(o);
                }
                if (typeName == "quatf")
                {
                    auto    o   =   new FEQuatfsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i)
                    {
                        quatf   q;
                        q.x = float(d0(i,0)); q.y = float(d0(i,1)); q.z = float(d0(i,2)); q.w = float(d0(i,3));
                        o->values().push_back(q);
                    }
                    return  QuatfsObject(o);
                }
                if (typeName == "bool")
                {
                    auto    o   =   new FEBoolsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(d0(i, 0) != 0 ? uint8(1) : uint8(0));
                    return  BoolsObject(o);
                }
                if (typeName == "alpha")
                {
                    auto    o   =   new FEAlphaObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(b0(i, 0));
                    return  AlphaObject(o);
                }
                if (typeName == "rgba")
                {
                    auto    o   =   new FEColorObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i)
                        o->values().push_back(uint8x4(b0(i,0), b0(i,1), b0(i,2), b0(i,3)));
                    return  RgbaObject(o);
                }
                if (typeName == "int")
                {
                    auto    o   =   new FEIntsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(int(d0(i, 0)));
                    return  IntsObject(o);
                }
                if (typeName == "int2")
                {
                    auto    o   =   new FEInt2sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(int2(int(d0(i,0)), int(d0(i,1))));
                    return  Int2sObject(o);
                }
                if (typeName == "int3")
                {
                    auto    o   =   new FEInt3sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(int3(int(d0(i,0)), int(d0(i,1)), int(d0(i,2))));
                    return  Int3sObject(o);
                }
                if (typeName == "int4")
                {
                    auto    o   =   new FEInt4sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(int4(int(d0(i,0)), int(d0(i,1)), int(d0(i,2)), int(d0(i,3))));
                    return  Int4sObject(o);
                }
                if (typeName == "uint")
                {
                    auto    o   =   new FEUintsObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(uint(d0(i, 0)));
                    return  UintsObject(o);
                }
                if (typeName == "uint2")
                {
                    auto    o   =   new FEUint2sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(uint2(uint(d0(i,0)), uint(d0(i,1))));
                    return  Uint2sObject(o);
                }
                if (typeName == "uint3")
                {
                    auto    o   =   new FEUint3sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(uint3(uint(d0(i,0)), uint(d0(i,1)), uint(d0(i,2))));
                    return  Uint3sObject(o);
                }
                if (typeName == "uint4")
                {
                    auto    o   =   new FEUint4sObject(ctx);
                    for (size_t i = 0; i < keys.size(); ++i) o->values().push_back(uint4(uint(d0(i,0)), uint(d0(i,1)), uint(d0(i,2)), uint(d0(i,3))));
                    return  Uint4sObject(o);
                }
                return  ValueObject{};
            }

            /// <summary>
            /// 创建并挂接 XML 属性(字符串由文档池分配,生命周期随 doc)
            /// </summary>
            static  void    addAttr(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node
                , const char* name, const std::string& value)
            {
                auto*   attr    =   doc.allocate_attribute(
                    doc.allocate_string(name)
                    , doc.allocate_string(value.c_str()));
                node->append_attribute(attr);
            }

            /// <summary>
            /// 读取节点字符串属性(缺失返回 fallback)
            /// </summary>
            static  std::string attrVal(rapidxml::xml_node<>* node, const char* name, const std::string& fallback = "")
            {
                auto*   attr    =   node->first_attribute(name);
                return  attr ? std::string(attr->value(), attr->value_size()) : fallback;
            }

            /// <summary>
            /// 写出对象自身 objectId 属性
            /// </summary>
            static  void    writeIdAttr(rapidxml::xml_document<>& doc, rapidxml::xml_node<>* node
                , const FEObject& obj)
            {
                addAttr(doc, node, "objectId", obj.objectId().toString());
            }

            /// <summary>
            /// 读取 objectId 属性;缺失/解析失败/空 id 时 hasId=false,调用方保留新生成 id
            /// </summary>
            static  std::pair<FEUuid,bool>  readIdAttr(rapidxml::xml_node<>* node)
            {
                auto*   attr    =   node->first_attribute("objectId");
                if (attr == nullptr)
                    return  {FEUuid(), false};
                bool    ok      =   false;
                FEUuid  id      =   FEUuid::from(std::string(attr->value(), attr->value_size()).c_str(), &ok);
                if (!ok || id == FEUuid::zero())
                    return  {FEUuid(), false};
                return  {id, true};
            }
        };
    };
}
