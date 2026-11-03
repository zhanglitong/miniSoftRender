#include    "FEAxisSettingWindow.hpp"
#include    "FECollapseGroup.hpp"

#include    <imgui.h>

#include    <algorithm>

namespace
{
    /// <summary>
    /// 窗口默认尺寸(参考 Qt 版 AxisEditor 对话框 491×344)
    /// </summary>
    const   float   kWindowW    =   491.0f;
    const   float   kWindowH    =   360.0f;
    /// <summary>
    /// 窗口默认位置:属性窗口右侧
    /// </summary>
    const   float   kWindowX    =   632.0f;
    const   float   kWindowY    =   136.0f;

    /// <summary>
    /// 顶部三列宽度:左列(轴位置类型/编辑类型)、中列(坐标系类型)、右列占剩余
    /// </summary>
    const   float   kColumn0W   =   148.0f;
    const   float   kColumn1W   =   132.0f;

    /// <summary>
    /// 自定义轴心/轴向的拖动步进
    /// </summary>
    const   float   kAxisDragSpeed  =   0.1f;
    /// <summary>
    /// XYZ 单个输入框宽度与标签间距
    /// </summary>
    const   float   kAxisInputW     =   68.0f;
    const   float   kAxisLabelGap   =   2.0f;

    /// <summary>
    /// XYZ 行标签
    /// </summary>
    const   char*   kAxisLabels[3]  =   {   "X","Y","Z"   };

    /// <summary>
    /// 计算选区的中心点(世界空间)
    /// </summary>
    /// <param name="objects">选区</param>
    /// <param name="average">true=位移中心(平均值) false=包围盒中心</param>
    FE::real3   selectionCenter(const FE::Objects& objects,bool average)
    {
        FE::real3   mn(1e30);
        FE::real3   mx(-1e30);
        FE::real3   sum(0);
        int         count   =   0;
        for (auto& var : objects)
        {
            if (var == nullptr)
                continue;
            auto    val =   var->getProperty(FE::PROP_G_TRANSFORM_XYZ);
            if (!std::holds_alternative<FE::real3>(val))
                continue;
            FE::real3   pos =   std::get<FE::real3>(val);
            mn  =   glm::min(mn,pos);
            mx  =   glm::max(mx,pos);
            sum +=  pos;
            ++count;
        }
        if (count == 0)
            return  FE::real3(0);
        if (average)
            return  sum / FE::real(count);
        return  (mn + mx) * FE::real(0.5);
    }

    /// <summary>
    /// 拾取方向:取选区中第一个节点的世界坐标系 Z 轴(已归一化)
    /// </summary>
    /// <param name="objects">选区</param>
    /// <returns>方向;没有可用节点时返回零向量</returns>
    FE::real3   selectionDirection(const FE::Objects& objects)
    {
        for (auto& var : objects)
        {
            if (var == nullptr)
                continue;
            FE::FEObject*   raw     =   const_cast<FE::FEObject*>(var.get());
            FE::Node        node    =   raw->cast<FE::FENode>();
            if (node == nullptr)
                continue;
            const   FE::mat4r   mat =   node->globalTransform();
            FE::real3   axis(mat[2].x,mat[2].y,mat[2].z);
            const   FE::real    len =   glm::length(axis);
            if (len > FE::real(1e-6))
                return  axis / len;
        }
        return  FE::real3(0);
    }

    /// <summary>
    /// 拖动交互状态:记录拖动开始前的整份配置快照,松开时提交为一条撤销记录
    /// </summary>
    struct  AxisDragState
    {
        FE::FEAxisSetting   _before;
        bool                _active     =   false;
        bool                _commit     =   false;
        /// <summary>
        /// 正在拖动的分量下标(0=X,1=Y,2=Z),-1 表示没有
        /// </summary>
        int                 _item       =   -1;
    };

    /// <summary>
    /// 判断两份轴配置是否有差异(没有实际改动时不必入栈)
    /// </summary>
    bool    axisSettingChanged(const FE::FEAxisSetting& lhs,const FE::FEAxisSetting& rhs)
    {
        return  lhs.posType     != rhs.posType
            ||  lhs.editType    != rhs.editType
            ||  lhs.coordType   != rhs.coordType
            ||  lhs.centerType  != rhs.centerType
            ||  lhs.disableX    != rhs.disableX
            ||  lhs.disableY    != rhs.disableY
            ||  lhs.disableZ    != rhs.disableZ
            ||  lhs.userCenter  != rhs.userCenter
            ||  lhs.customAxis  != rhs.customAxis;
    }

    /// <summary>
    /// 一行 XYZ 三个独立输入框(对应 Qt 版的三个 QDoubleSpinBox)
    /// </summary>
    /// <param name="id">行标识(同一窗口内唯一)</param>
    /// <param name="value">三分量数值(输入输出)</param>
    /// <param name="drag">拖动状态:记录拖动开始前的配置快照</param>
    /// <param name="frameBefore">本帧开始时的配置快照</param>
    /// <returns>true 表示本次被修改</returns>
    bool    axisVectorRow(const char* id,FE::real3& value,AxisDragState& drag
                          ,const FE::FEAxisSetting& frameBefore)
    {
        float   values[3]   =   {   (float)value.x,(float)value.y,(float)value.z    };
        bool    changed     =   false;

        ImGui::PushID(id);
        for (int i = 0; i < 3; ++i)
        {
            if (i > 0)
                ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::TextUnformatted(kAxisLabels[i]);
            ImGui::SameLine(0.0f,kAxisLabelGap);
            ImGui::PushItemWidth(kAxisInputW);
            changed |=  ImGui::DragFloat("##value",&values[i],kAxisDragSpeed,0.0f,0.0f,"%.3f");
            /// <summary>
            /// 拖动开始(控件被按住)时记下整份配置,松开后作为撤销记录的"修改前"。
            /// ImGui 1.65 没有 IsItemActivated,用 IsItemActive 判断按下/松开
            /// </summary>
            if (ImGui::IsItemActive())
            {
                if (!drag._active)
                {
                    drag._before    =   frameBefore;
                    drag._active    =   true;
                    drag._item      =   i;
                }
            }
            else if (drag._active && drag._item == i)
            {
                drag._active    =   false;
                drag._item      =   -1;
                /// 有实际数值改动才提交(单纯点一下输入框不算)
                if (ImGui::IsItemDeactivatedAfterEdit())
                    drag._commit    =   true;
            }
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
        ImGui::PopID();

        if (changed)
            value   =   FE::real3(values[0],values[1],values[2]);
        return  changed;
    }
}

namespace   FE
{
    bool    drawAxisSettingWindow(FEAxisSetting& setting,bool& open,const Objects& selection
                                  ,FEAxisSetting& before)
    {
        /// <summary>
        /// 分组状态在窗口生命周期内保持(折叠/展开记忆)
        /// </summary>
        static  FECollapseGroup     grpPos("轴位置类型");
        static  FECollapseGroup     grpEdit("编辑类型");
        static  FECollapseGroup     grpCoord("坐标系类型");
        static  FECollapseGroup     grpCenter("中心类型");
        static  FECollapseGroup     grpUser("自定义轴心(xyz)");
        static  FECollapseGroup     grpAxis("自定义轴向");
        static  FECollapseGroup     grpDisable("禁用编辑轴");
        /// <summary>
        /// 两个拖动行的交互状态(拖动松开后才提交,避免逐帧产生撤销记录)
        /// </summary>
        static  AxisDragState       dragUser;
        static  AxisDragState       dragAxis;

        ImGui::SetNextWindowPos(ImVec2(kWindowX,kWindowY),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kWindowW,kWindowH),ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("坐标轴编辑",&open))
        {
            ImGui::End();
            return  false;
        }

        /// <summary>
        /// 本帧开始的配置快照:点击类控件(单选/复选/按钮)的"修改前"状态
        /// </summary>
        const   FEAxisSetting   frameBefore =   setting;

        /// <summary>
        /// 顶部三列(参考 Qt 版:左列两组、中列坐标系、右列中心类型)
        /// </summary>
        ImGui::Columns(3,"axis_top",false);
        ImGui::SetColumnWidth(0,kColumn0W);
        ImGui::SetColumnWidth(1,kColumn1W);

        if (grpPos.begin())
        {
            if (ImGui::RadioButton("包围盒中心",setting.posType == APT_AabbCenter))
                setting.posType    =   APT_AabbCenter;
            if (ImGui::RadioButton("位移中心",setting.posType == APT_Translation))
                setting.posType    =   APT_Translation;
        }
        grpPos.end();

        if (grpEdit.begin())
        {
            if (ImGui::RadioButton("对象",setting.editType == AET_Object))
                setting.editType   =   AET_Object;
            if (ImGui::RadioButton("编辑轴",setting.editType == AET_Axis))
                setting.editType   =   AET_Axis;
        }
        grpEdit.end();

        ImGui::NextColumn();

        if (grpCoord.begin())
        {
            if (ImGui::RadioButton("世界坐标系",setting.coordType == ACT_World))
                setting.coordType  =   ACT_World;
            if (ImGui::RadioButton("父节点坐标系",setting.coordType == ACT_Parent))
                setting.coordType  =   ACT_Parent;
            if (ImGui::RadioButton("局部坐标系",setting.coordType == ACT_Local))
                setting.coordType  =   ACT_Local;
            if (ImGui::RadioButton("屏幕坐标系",setting.coordType == ACT_Screen))
                setting.coordType  =   ACT_Screen;
        }
        grpCoord.end();

        ImGui::NextColumn();

        if (grpCenter.begin())
        {
            ImGui::PushID("center_aabb");
            if (ImGui::RadioButton("包围盒中心",setting.centerType == ACE_AabbCenter))
                setting.centerType =   ACE_AabbCenter;
            ImGui::SameLine();
            if (ImGui::Button("填充"))
                setting.userCenter =   selectionCenter(selection,false);
            ImGui::PopID();

            ImGui::PushID("center_trans");
            if (ImGui::RadioButton("位移中心",setting.centerType == ACE_Translation))
                setting.centerType =   ACE_Translation;
            ImGui::SameLine();
            if (ImGui::Button("填充"))
                setting.userCenter =   selectionCenter(selection,true);
            ImGui::PopID();

            if (ImGui::RadioButton("各自原点",setting.centerType == ACE_SelfOrigin))
                setting.centerType =   ACE_SelfOrigin;

            ImGui::PushID("center_user");
            if (ImGui::RadioButton("用户",setting.centerType == ACE_User))
                setting.centerType =   ACE_User;
            ImGui::SameLine();
            if (ImGui::Button("重置"))
                setting.userCenter =   real3(0);
            ImGui::PopID();
        }
        grpCenter.end();

        ImGui::Columns(1);

        if (grpUser.begin())
        {
            axisVectorRow("user_center",setting.userCenter,dragUser,frameBefore);
            ImGui::SameLine();
            if (ImGui::Button("锚点填充"))
                setting.userCenter =   selectionCenter(selection,false);
        }
        grpUser.end();

        if (grpAxis.begin())
        {
            axisVectorRow("custom_axis",setting.customAxis,dragAxis,frameBefore);
            ImGui::SameLine();
            if (ImGui::Button("拾取方向"))
            {
                const   real3   direction   =   selectionDirection(selection);
                if (glm::length(direction) > real(0))
                    setting.customAxis  =   direction;
            }
        }
        grpAxis.end();

        if (grpDisable.begin())
        {
            /// <summary>
            /// 三个勾选框按列均分,对应 Qt 版 X / Y / Z + 弹性间隔
            /// </summary>
            const   float   spacing =   ImGui::GetContentRegionAvail().x / 3.0f;
            ImGui::Checkbox("X",&setting.disableX);
            ImGui::SameLine(spacing);
            ImGui::Checkbox("Y",&setting.disableY);
            ImGui::SameLine(spacing * 2.0f);
            ImGui::Checkbox("Z",&setting.disableZ);
        }
        grpDisable.end();

        ImGui::End();

        /// <summary>
        /// 提交本次改动:拖动结束用拖动起点的快照,点击类控件用本帧开始的快照。
        /// 拖动进行中不返回,避免逐帧产生大量撤销记录
        /// </summary>
        bool    changed =   false;
        if (dragUser._commit)
        {
            dragUser._commit    =   false;
            if (axisSettingChanged(dragUser._before,setting))
            {
                before  =   dragUser._before;
                changed =   true;
            }
        }
        else if (dragAxis._commit)
        {
            dragAxis._commit    =   false;
            if (axisSettingChanged(dragAxis._before,setting))
            {
                before  =   dragAxis._before;
                changed =   true;
            }
        }
        /// <summary>
        /// 窗口被关闭或焦点丢失时清掉残留的拖动标记,避免后续点击类改动无法提交
        /// </summary>
        if (!ImGui::IsAnyItemActive())
        {
            dragUser._active    =   false;
            dragUser._item      =   -1;
            dragAxis._active    =   false;
            dragAxis._item      =   -1;
        }
        if (!changed && !dragUser._active && !dragAxis._active
            && axisSettingChanged(frameBefore,setting))
        {
            before  =   frameBefore;
            changed =   true;
        }
        return  changed;
    }
}
