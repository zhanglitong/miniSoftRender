/// <summary>
/// FEPropertyView
/// 通用 key/value 属性控件:两列树形表格,分组可折叠,值按类型内联编辑。
/// 数据来源通过 FEPropertyModel 抽象,任何键值树都可适配(如 FESetting),
/// 控件本身只依赖 ImGui 1.65 的 Columns / Drag* 等旧接口。
/// </summary>

#pragma     once

#include    "imgui.h"

#include    <algorithm>
#include    <cstdint>
#include    <cstring>
#include    <string>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 属性值:覆盖标量/向量/字符串;由适配器在数据源与控件之间转换
    /// </summary>
    struct  FEPropertyValue
    {
        enum class  Type
        {
            None
            , Bool
            , Int32,    UInt32,     Int64,  UInt64
            , Float,    Double
            , Int2,     Int3,       Int4
            , UInt2,    UInt3,      UInt4
            , Float2,   Float3,     Float4
            , Real2,    Real3,      Real4
            , String
        };

        Type        _type   =   Type::None;
        bool        _bool   =   false;
        int64_t     _int    =   0;
        uint64_t    _uint   =   0;
        double      _real   =   0.0;
        /// <summary>
        /// 向量分量(Int2 等为整数,Float/Real 为浮点)
        /// </summary>
        double      _vec[4] =   {};
        std::string _string;
    };

    /// <summary>
    /// 属性数据源接口:树形节点 + 类型化值
    /// Node 是数据源自己的节点标识,根节点用 nullptr
    /// </summary>
    class   FEPropertyModel
    {
    public:
        using   Node    =   const void*;
    public:
        virtual ~FEPropertyModel()  =   default;
        /// <summary>
        /// 子节点数量
        /// </summary>
        virtual size_t          childCount(Node parent) const               =   0;
        /// <summary>
        /// 第 index 个子节点标识
        /// </summary>
        virtual Node            child(Node parent,size_t index) const       =   0;
        /// <summary>
        /// 节点名(key)
        /// </summary>
        virtual std::string     name(Node node) const                        =   0;
        /// <summary>
        /// 节点值;Type::None 表示该节点只有子节点
        /// </summary>
        virtual FEPropertyValue value(Node node) const                       =   0;
        /// <summary>
        /// 写回节点值;返回 false 表示只读或类型不支持
        /// </summary>
        virtual bool            setValue(Node node,const FEPropertyValue&)   =   0;
    };

    /// <summary>
    /// 属性控件:静态绘制,不持有数据
    /// </summary>
    class   FEPropertyView
    {
    public:
        /// <summary>
        /// 绘制整棵属性树
        /// </summary>
        /// <param name="model">数据源</param>
        /// <param name="id">ImGui 标识(同一窗口多个控件时需区分)</param>
        /// <param name="nameRatio">名称列宽度比例</param>
        static  void    draw(FEPropertyModel& model,const char* id = "##property",
                             float nameRatio = 0.45f)
        {
            ImGui::Columns(2,id,true);
            ImGui::SetColumnWidth(0,ImGui::GetWindowWidth() * nameRatio);

            ImGui::TextDisabled("属性");
            ImGui::NextColumn();
            ImGui::TextDisabled("值");
            ImGui::NextColumn();

            drawChildren(model,nullptr,0);
            ImGui::Columns(1);
        }
    protected:
        static  void    drawChildren(FEPropertyModel& model,FEPropertyModel::Node parent,int depth)
        {
            const   size_t  count   =   model.childCount(parent);
            for (size_t i = 0; i < count; ++i)
                drawNode(model,model.child(parent,i),depth);
        }
        static  void    drawNode(FEPropertyModel& model,FEPropertyModel::Node node,int depth)
        {
            const   std::string     name    =   model.name(node);
            const   size_t          count   =   model.childCount(node);
            const   FEPropertyValue value   =   model.value(node);
            const   bool            group   =   count > 0;

            ImGui::PushID(node);

            ImGuiTreeNodeFlags  flags   =   ImGuiTreeNodeFlags_FramePadding;
            if (!group)
                flags   |=  ImGuiTreeNodeFlags_Leaf;
            if (depth == 0)
                flags   |=  ImGuiTreeNodeFlags_DefaultOpen;

            const   bool    open    =   ImGui::TreeNodeEx(name.c_str(),flags);
            ImGui::NextColumn();

            /// 有值就画编辑器;分组没有值则显示子项数量
            ImGui::PushItemWidth(-1.0f);
            if (value._type != FEPropertyValue::Type::None)
                drawValue(model,node,value);
            else if (group)
                ImGui::TextDisabled("%d 项",(int)count);
            ImGui::PopItemWidth();
            ImGui::NextColumn();

            if (group)
            {
                if (open)
                {
                    drawChildren(model,node,depth + 1);
                    ImGui::TreePop();
                }
            }
            else if (open)
            {
                ImGui::TreePop();
            }
            ImGui::PopID();
        }
        /// <summary>
        /// 向量分量数
        /// </summary>
        static  int     componentsOf(FEPropertyValue::Type type)
        {
            using   Type    =   FEPropertyValue::Type;
            switch (type)
            {
            case Type::Int2:    case Type::UInt2:   case Type::Float2:  case Type::Real2:
                return  2;
            case Type::Int3:    case Type::UInt3:   case Type::Float3:  case Type::Real3:
                return  3;
            case Type::Int4:    case Type::UInt4:   case Type::Float4:  case Type::Real4:
                return  4;
            default:
                return  0;
            }
        }
        /// <summary>
        /// 向量分量的 ImGui 数据类型
        /// </summary>
        static  ImGuiDataType   dataTypeOf(FEPropertyValue::Type type)
        {
            using   Type    =   FEPropertyValue::Type;
            switch (type)
            {
            case Type::Int2:    case Type::Int3:    case Type::Int4:
                return  ImGuiDataType_S32;
            case Type::UInt2:   case Type::UInt3:   case Type::UInt4:
                return  ImGuiDataType_U32;
            case Type::Float2:  case Type::Float3:  case Type::Float4:
                return  ImGuiDataType_Float;
            default:
                return  ImGuiDataType_Double;
            }
        }
        /// <summary>
        /// 向量编辑:按分量类型临时转换为数组后交给 DragScalarN
        /// </summary>
        template<class TScalar>
        static  bool    dragVector(FEPropertyModel& model,FEPropertyModel::Node node,
                                   const FEPropertyValue& value,ImGuiDataType dataType)
        {
            const   int comps   =   componentsOf(value._type);
            TScalar     scalars[4]  =   {};
            for (int i = 0; i < comps; ++i)
                scalars[i]  =   static_cast<TScalar>(value._vec[i]);
            if (!ImGui::DragScalarN("##value",dataType,scalars,comps,0.01f))
                return  false;

            FEPropertyValue next    =   value;
            for (int i = 0; i < comps; ++i)
                next._vec[i]    =   static_cast<double>(scalars[i]);
            return  model.setValue(node,next);
        }
        /// <summary>
        /// 按值类型选择内联编辑控件
        /// </summary>
        static  bool    drawValue(FEPropertyModel& model,FEPropertyModel::Node node,
                                  const FEPropertyValue& value)
        {
            using   Type    =   FEPropertyValue::Type;
            switch (value._type)
            {
            case Type::Bool:
            {
                bool    v   =   value._bool;
                if (!ImGui::Checkbox("##value",&v))
                    return  false;
                FEPropertyValue next    =   value;
                next._bool  =   v;
                return  model.setValue(node,next);
            }
            case Type::Int32:
            {
                int32_t v   =   (int32_t)value._int;
                if (!ImGui::DragScalar("##value",ImGuiDataType_S32,&v,0.25f))
                    return  false;
                FEPropertyValue next    =   value;
                next._int   =   v;
                return  model.setValue(node,next);
            }
            case Type::UInt32:
            {
                uint32_t    v   =   (uint32_t)value._uint;
                if (!ImGui::DragScalar("##value",ImGuiDataType_U32,&v,0.25f))
                    return  false;
                FEPropertyValue next    =   value;
                next._uint  =   v;
                return  model.setValue(node,next);
            }
            case Type::Int64:
            {
                int64_t v   =   value._int;
                if (!ImGui::DragScalar("##value",ImGuiDataType_S64,&v,1.0f))
                    return  false;
                FEPropertyValue next    =   value;
                next._int   =   v;
                return  model.setValue(node,next);
            }
            case Type::UInt64:
            {
                uint64_t    v   =   value._uint;
                if (!ImGui::DragScalar("##value",ImGuiDataType_U64,&v,1.0f))
                    return  false;
                FEPropertyValue next    =   value;
                next._uint  =   v;
                return  model.setValue(node,next);
            }
            case Type::Float:
            {
                float   v   =   (float)value._real;
                if (!ImGui::DragFloat("##value",&v,0.01f,0.0f,0.0f,"%.6g"))
                    return  false;
                FEPropertyValue next    =   value;
                next._real  =   v;
                return  model.setValue(node,next);
            }
            case Type::Double:
            {
                double  v   =   value._real;
                if (!ImGui::DragScalar("##value",ImGuiDataType_Double,&v,0.01f,nullptr,nullptr,"%.6g"))
                    return  false;
                FEPropertyValue next    =   value;
                next._real  =   v;
                return  model.setValue(node,next);
            }
            case Type::Int2:    case Type::Int3:    case Type::Int4:
                return  dragVector<int32_t>(model,node,value,ImGuiDataType_S32);
            case Type::UInt2:   case Type::UInt3:   case Type::UInt4:
                return  dragVector<uint32_t>(model,node,value,ImGuiDataType_U32);
            case Type::Float2:  case Type::Float3:  case Type::Float4:
                return  dragVector<float>(model,node,value,ImGuiDataType_Float);
            case Type::Real2:   case Type::Real3:   case Type::Real4:
                return  dragVector<double>(model,node,value,ImGuiDataType_Double);
            case Type::String:
            {
                const   size_t  len     =   value._string.size();
                std::vector<char>   buffer(std::max<size_t>(len + 128,256),0);
                std::memcpy(buffer.data(),value._string.c_str(),len);
                if (!ImGui::InputText("##value",buffer.data(),buffer.size()))
                    return  false;
                FEPropertyValue next    =   value;
                next._string.assign(buffer.data());
                return  model.setValue(node,next);
            }
            case Type::None:
            default:
                return  false;
            }
        }
    };
}
