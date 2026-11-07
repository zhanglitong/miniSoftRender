#pragma     once
#include    <cstdio>
#include    <cstdlib>
#include    <fstream>
#include    <sstream>
#include    <string>
#include    <vector>
#include    <unordered_map>

#include    "../../FEContext.hpp"
#include    "../../FEString.hpp"
#include    "../../FEXML.hpp"
#include    "../../node/FESkin.hpp"
#include    "../../node/FENode.hpp"

namespace   FE
{
    /// <summary>
    /// 独立骨骼文件(XML)读写:
    ///   - 骨骼只描述关节层级与局部 TRS,不含网格;XML 中的关节姿态即绑定姿态,
    ///     加载时据此计算 inverseBind = inverse(关节全局变换);
    ///   - bindings 记录蒙皮网格的绑定信息(场景节点名 + 网格资源路径),
    ///     由调用方按需把 FESkinData 挂到对应网格节点上;
    ///   - worldRotation 属于共享骨骼数据(FESkinData),如 glTF 的 Y-up -> Z-up 校正。
    ///
    /// 文件格式示例:
    /// <skeleton version="1" worldRotation="1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1">
    ///   <joints>
    ///     <joint name="Root"  parent="-1" t="0,0,0" r="0,0,0,1" s="1,1,1"/>
    ///     <joint name="Hips"  parent="0"  t="0,0,0.95" r="0,0,0,1" s="1,1,1"/>
    ///   </joints>
    ///   <bindings>
    ///     <binding node="Soldier" mesh="data/model/Soldier.glb"/>
    ///   </bindings>
    /// </skeleton>
    /// </summary>
    class   FESkeletonXml
    {
    public:
        struct  JointDesc
        {
            String  name;
            /// 父关节下标(-1 表示根关节),必须小于自身下标
            int     parent      =   -1;
            real3   translation =   real3(0.0);
            quatr   rotation    =   quatr(1.0,0.0,0.0,0.0);
            real3   scaling     =   real3(1.0);
        };
        struct  MeshBinding
        {
            /// 场景中挂蒙皮的网格节点名
            String  node;
            /// 网格资源路径(信息字段,由调用方决定如何加载)
            String  mesh;
        };
        struct  Skeleton
        {
            mat4r                   worldRotation   =   mat4r(1.0);
            std::vector<JointDesc>  joints;
            std::vector<MeshBinding> bindings;
        };
        /// <summary>
        /// 加载结果:关节节点(与 XML 顺序一致)、顶层关节节点、共享骨骼数据
        /// </summary>
        struct  Instance
        {
            Nodes                   joints;
            Nodes                   roots;
            SharedPtr<FESkinData>   skin;
        };
    public:
        /// <summary>
        /// 读取骨骼 XML;失败时返回 false 并通过 err 输出原因
        /// </summary>
        static  bool    load(const String& path,Skeleton& out,String* err = nullptr)
        {
            out =   Skeleton();
            try
            {
                std::ifstream   file(path.c_str(),std::ios::binary);
                if (!file.is_open())
                {
                    if (err)    *err    =   String(u8"无法打开骨骼文件: ") + path;
                    return  false;
                }
                std::ostringstream  mem;
                mem << file.rdbuf();
                file.close();
                String  content =   mem.str();
                /// rapidxml 原地解析,缓冲区生命周期需覆盖整次解析
                std::vector<char>   buffer(content.begin(),content.end());
                buffer.push_back('\0');
                /// 跳过 UTF-8 BOM
                char*   text    =   buffer.data();
                if (buffer.size() >= 4
                    &&  static_cast<unsigned char>(text[0]) == 0xEF
                    &&  static_cast<unsigned char>(text[1]) == 0xBB
                    &&  static_cast<unsigned char>(text[2]) == 0xBF)
                    text    +=  3;

                rapidxml::xml_document<>    doc;
                doc.parse<0>(text);

                auto*   root    =   doc.first_node("skeleton");
                if (root == nullptr)
                {
                    if (err)    *err    =   u8"不是有效的骨骼 XML(缺少 skeleton 根节点)";
                    return  false;
                }
                if (const char* rot =   attr(root,"worldRotation"))
                    out.worldRotation   =   readMat(rot);
                if (const char* version =   attr(root,"version"))
                    UNUSED(version);

                auto*   jointsNode  =   root->first_node("joints");
                if (jointsNode == nullptr)
                {
                    if (err)    *err    =   u8"骨骼 XML 缺少 joints 节点";
                    return  false;
                }
                for (auto* jointNode = jointsNode->first_node("joint");
                     jointNode != nullptr;
                     jointNode = jointNode->next_sibling("joint"))
                {
                    JointDesc   joint;
                    joint.name          =   (attr(jointNode,"name") != nullptr) ? attr(jointNode,"name") : "";
                    joint.parent        =   (attr(jointNode,"parent") != nullptr) ? std::atoi(attr(jointNode,"parent")) : -1;
                    joint.translation   =   readReal3(attr(jointNode,"t"),real3(0.0));
                    joint.rotation      =   readQuat(attr(jointNode,"r"));
                    joint.scaling       =   readReal3(attr(jointNode,"s"),real3(1.0));

                    if (joint.parent < -1
                        ||  joint.parent >= int(out.joints.size()))
                    {
                        if (err)    *err    =   String(u8"关节父下标非法: ") + joint.name;
                        return  false;
                    }
                    out.joints.push_back(std::move(joint));
                }
                if (out.joints.empty())
                {
                    if (err)    *err    =   u8"骨骼 XML 未包含任何关节";
                    return  false;
                }

                if (auto*   bindingsNode    =   root->first_node("bindings"))
                {
                    for (auto* bindingNode = bindingsNode->first_node("binding");
                         bindingNode != nullptr;
                         bindingNode = bindingNode->next_sibling("binding"))
                    {
                        MeshBinding binding;
                        binding.node    =   (attr(bindingNode,"node") != nullptr) ? attr(bindingNode,"node") : "";
                        binding.mesh    =   (attr(bindingNode,"mesh") != nullptr) ? attr(bindingNode,"mesh") : "";
                        if (binding.node.empty())
                        {
                            if (err)    *err    =   u8"binding 缺少 node 属性";
                            return  false;
                        }
                        out.bindings.push_back(std::move(binding));
                    }
                }
            }
            catch (const rapidxml::parse_error& e)
            {
                if (err)    *err    =   String(u8"解析骨骼 XML 失败: ") + e.what();
                return  false;
            }
            catch (const std::exception& e)
            {
                if (err)    *err    =   String(u8"读取骨骼 XML 失败: ") + e.what();
                return  false;
            }
            return  true;
        }
        /// <summary>
        /// 保存骨骼 XML(关节 + 绑定信息)
        /// </summary>
        static  bool    save(const Skeleton& skel,const String& path,String* err = nullptr)
        {
            try
            {
                rapidxml::xml_document<>    doc;
                auto*   root    =   doc.allocate_node(rapidxml::node_element,"skeleton");
                doc.append_node(root);
                addAttr(doc,root,"version","1");
                if (skel.worldRotation != mat4r(1.0))
                    addAttr(doc,root,"worldRotation",matToText(skel.worldRotation).c_str());

                auto*   jointsNode  =   doc.allocate_node(rapidxml::node_element,"joints");
                root->append_node(jointsNode);
                for (auto& joint : skel.joints)
                {
                    auto*   jointNode   =   doc.allocate_node(rapidxml::node_element,"joint");
                    jointsNode->append_node(jointNode);
                    addAttr(doc,jointNode,"name",joint.name.c_str());
                    addAttr(doc,jointNode,"parent",std::to_string(joint.parent).c_str());
                    addAttr(doc,jointNode,"t",real3ToText(joint.translation).c_str());
                    addAttr(doc,jointNode,"r",quatToText(joint.rotation).c_str());
                    addAttr(doc,jointNode,"s",real3ToText(joint.scaling).c_str());
                }

                if (!skel.bindings.empty())
                {
                    auto*   bindingsNode    =   doc.allocate_node(rapidxml::node_element,"bindings");
                    root->append_node(bindingsNode);
                    for (auto& binding : skel.bindings)
                    {
                        auto*   bindingNode =   doc.allocate_node(rapidxml::node_element,"binding");
                        bindingsNode->append_node(bindingNode);
                        addAttr(doc,bindingNode,"node",binding.node.c_str());
                        if (!binding.mesh.empty())
                            addAttr(doc,bindingNode,"mesh",binding.mesh.c_str());
                    }
                }

                std::ostringstream  oss;
                oss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                rapidxml::print(static_cast<std::ostream&>(oss),*root,0);
                writeText(path,oss.str(),err);
            }
            catch (const std::exception& e)
            {
                if (err)    *err    =   String(u8"写入骨骼 XML 失败: ") + e.what();
                return  false;
            }
            return  true;
        }
        /// <summary>
        /// 从运行中的骨骼数据导出 XML:关节取自 FESkinData,局部 TRS 取当前节点变换
        /// (导出前请确保节点处于绑定姿态)
        /// </summary>
        static  bool    save(const FESkinData& data,const std::vector<MeshBinding>& bindings,const String& path,String* err = nullptr)
        {
            Skeleton    skel;
            skel.worldRotation  =   data.worldRotation();
            skel.bindings       =   bindings;

            auto&   bones   =   data.bones();
            std::unordered_map<const FENode*,int>   jointIndex;
            jointIndex.reserve(bones.size());
            for (size_t i = 0;i < bones.size();++i)
            {
                if (bones[i].node != nullptr)
                    jointIndex[bones[i].node.get()] =   int(i);
            }
            for (size_t i = 0;i < bones.size();++i)
            {
                JointDesc   joint;
                const   FENode* node    =   bones[i].node.get();
                if (node != nullptr)
                {
                    joint.name          =   (node->name() != nullptr) ? node->name() : "";
                    joint.translation   =   node->localTranslation();
                    joint.rotation      =   node->localRotation();
                    joint.scaling       =   node->localScaling();

                    auto    parent  =   node->parent();
                    const   FENode* parentNode  =   (parent == nullptr) ? nullptr : parent->cast<FENode>();
                    auto    itr =   jointIndex.find(parentNode);
                    joint.parent    =   (itr == jointIndex.end()) ? -1 : itr->second;
                }
                skel.joints.push_back(std::move(joint));
            }
            return  save(skel,path,err);
        }
        /// <summary>
        /// 按 XML 构建关节节点层级与 FESkinData:
        ///   - 关节节点尚未加入场景,由调用方把 roots 挂到场景中;
        ///   - inverseBind 以 XML 中的局部 TRS 作为绑定姿态计算。
        /// </summary>
        static  bool    build(FEContext& ctx,const Skeleton& skel,Instance& out,String* err = nullptr)
        {
            out =   Instance();
            if (skel.joints.empty())
            {
                if (err)    *err    =   u8"骨骼描述为空";
                return  false;
            }

            out.joints.reserve(skel.joints.size());
            for (size_t i = 0;i < skel.joints.size();++i)
            {
                auto&   desc    =   skel.joints[i];
                Node    node    =   new FENode(ctx);
                if (!desc.name.empty())
                    node->setName(desc.name.c_str());
                node->setLocalTranslation(desc.translation);
                node->setLocalRotation(desc.rotation);
                node->setLocalScaling(desc.scaling);
                out.joints.push_back(node);
            }
            /// 父子关系必须在所有节点创建后建立(父下标一定小于子下标)
            for (size_t i = 0;i < skel.joints.size();++i)
            {
                const   int parent  =   skel.joints[i].parent;
                if (parent < 0)
                    out.roots.push_back(out.joints[i]);
                else
                    out.joints[size_t(parent)]->addChild(out.joints[i]);
            }
            /// 刷新绑定姿态的全局变换,据此计算 inverseBind
            for (auto& root : out.roots)
                root->update();

            Joints      joints;
            InverseBinds    inverseBinds;
            joints.reserve(out.joints.size());
            inverseBinds.reserve(out.joints.size());
            for (auto& joint : out.joints)
            {
                joints.push_back(joint);
                inverseBinds.push_back(FE::inverse(joint->globalTransform()));
            }

            auto    skin    =   new FESkinData(ctx);
            skin->setWorldRotation(skel.worldRotation);
            skin->setJoints(joints,inverseBinds);
            out.skin    =   skin;
            return  true;
        }
        /// <summary>
        /// 在节点树中按名字查找网格节点并挂上 FESkin(已存在则复用);
        /// 返回挂接的组件,找不到节点返回 nullptr
        /// </summary>
        static  FESkin* bind(FENode* root,const String& nodeName,const SharedPtr<FESkinData>& data)
        {
            FENode* node    =   findNode(root,nodeName);
            if (node == nullptr || data == nullptr)
                return  nullptr;
            auto    skins   =   node->objects<FESkin>();
            FESkin* skin    =   skins.empty() ? nullptr : skins.front();
            if (skin == nullptr)
            {
                skin    =   new FESkin(node->ctx());
                node->addComponent(skin);
            }
            skin->setData(data);
            return  skin;
        }
        /// <summary>
        /// 按 XML 的 bindings 找到网格节点并挂接蒙皮,返回成功挂接的数量
        /// </summary>
        static  size_t  applyBindings(const Skeleton& skel,const SharedPtr<FESkinData>& data,FENode* searchRoot)
        {
            size_t  count   =   0;
            for (auto& binding : skel.bindings)
            {
                if (bind(searchRoot,binding.node,data) != nullptr)
                    ++count;
            }
            return  count;
        }

    private:
        static  const   char*   attr(rapidxml::xml_node<>* node,const char* name)
        {
            if (node == nullptr)
                return  nullptr;
            auto*   attribute   =   node->first_attribute(name);
            return  (attribute == nullptr) ? nullptr : attribute->value();
        }
        static  void    addAttr(rapidxml::xml_document<>& doc,rapidxml::xml_node<>* node,const char* name,const char* value)
        {
            node->append_attribute(doc.allocate_attribute(
                doc.allocate_string(name),doc.allocate_string(value != nullptr ? value : "")));
        }
        static  bool    parseReals(const char* text,real* values,size_t count)
        {
            if (text == nullptr)
                return  false;
            std::stringstream   ss(text);
            std::string         item;
            size_t              index   =   0;
            while (index < count && std::getline(ss,item,','))
                values[index++] =   real(std::strtod(item.c_str(),nullptr));
            return  index == count;
        }
        static  real3   readReal3(const char* text,const real3& def)
        {
            real    values[3];
            return  parseReals(text,values,3) ? real3(values[0],values[1],values[2]) : def;
        }
        static  quatr   readQuat(const char* text)
        {
            real    values[4];
            if (!parseReals(text,values,4))
                return  quatr(1.0,0.0,0.0,0.0);
            return  quatr(values[3],values[0],values[1],values[2]);
        }
        static  mat4r   readMat(const char* text)
        {
            real    values[16];
            if (!parseReals(text,values,16))
                return  mat4r(1.0);
            return  mat4r(values[0],values[1],values[2],values[3],
                          values[4],values[5],values[6],values[7],
                          values[8],values[9],values[10],values[11],
                          values[12],values[13],values[14],values[15]);
        }
        static  String  realsToText(const real* values,size_t count)
        {
            String  result;
            char    buf[64] =   {0};
            for (size_t i = 0;i < count;++i)
            {
                std::snprintf(buf,sizeof(buf),"%.9g",double(values[i]));
                if (i > 0)
                    result.push_back(',');
                result.append(buf);
            }
            return  result;
        }
        static  String  real3ToText(const real3& value)
        {
            return  realsToText(&value[0],3);
        }
        static  String  quatToText(const quatr& value)
        {
            /// 存储顺序 x,y,z,w(glm 构造函数为 w,x,y,z)
            const   real    values[4]   =   {value.x,value.y,value.z,value.w};
            return  realsToText(values,4);
        }
        static  String  matToText(const mat4r& value)
        {
            return  realsToText(&value[0][0],16);
        }
        static  void    writeText(const String& path,const String& text,String* err)
        {
            std::ofstream   file(path.c_str(),std::ios::binary | std::ios::trunc);
            if (!file.is_open())
            {
                if (err)    *err    =   String(u8"无法打开文件: ") + path;
                return;
            }
            file.write(text.data(),std::streamsize(text.size()));
            file.close();
            if (!file.good() && err != nullptr)
                *err    =   String(u8"写入文件失败: ") + path;
        }
        static  FENode* findNode(FENode* node,const String& name)
        {
            if (node == nullptr)
                return  nullptr;
            if (node->name() != nullptr && name == node->name())
                return  node;
            for (auto& child : node->children())
            {
                FENode* found   =   findNode(child.get(),name);
                if (found != nullptr)
                    return  found;
            }
            return  nullptr;
        }
    };
}
