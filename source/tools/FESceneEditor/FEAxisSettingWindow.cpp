#include    "FEAxisSettingWindow.hpp"
#include    "FECollapseGroup.hpp"

#include    <imgui.h>

#include    <algorithm>

namespace
{
    /// <summary>
    /// 窗口默认尺寸(参考 Qt 版 AxisEditor 对话框)
    /// </summary>
    const   float   kWindowW    =   300.0f;
    const   float   kWindowH    =   420.0f;
    /// <summary>
    /// 窗口默认位置:属性窗口右侧
    /// </summary>
    const   float   kWindowX    =   632.0f;
    const   float   kWindowY    =   136.0f;

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
}

namespace   FE
{
    void    drawAxisSettingWindow(FEAxisSetting& setting,bool& open,const Objects& selection)
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

        ImGui::SetNextWindowPos(ImVec2(kWindowX,kWindowY),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kWindowW,kWindowH),ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("坐标轴编辑",&open))
        {
            ImGui::End();
            return;
        }

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

        if (grpUser.begin())
        {
            float   values[3]   =   { (float)setting.userCenter.x,(float)setting.userCenter.y,(float)setting.userCenter.z };
            if (ImGui::DragFloat3("##userCenter",values,0.1f))
                setting.userCenter =   real3(values[0],values[1],values[2]);
            if (ImGui::Button("锚点填充"))
                setting.userCenter =   selectionCenter(selection,false);
        }
        grpUser.end();

        if (grpAxis.begin())
        {
            float   values[3]   =   { (float)setting.customAxis.x,(float)setting.customAxis.y,(float)setting.customAxis.z };
            if (ImGui::DragFloat3("##customAxis",values,0.1f))
                setting.customAxis =   real3(values[0],values[1],values[2]);
            if (ImGui::Button("归一化"))
            {
                const   real    len =   glm::length(setting.customAxis);
                if (len > real(0))
                    setting.customAxis /=  len;
            }
        }
        grpAxis.end();

        if (grpDisable.begin())
        {
            ImGui::Checkbox("X",&setting.disableX);
            ImGui::SameLine();
            ImGui::Checkbox("Y",&setting.disableY);
            ImGui::SameLine();
            ImGui::Checkbox("Z",&setting.disableZ);
        }
        grpDisable.end();

        ImGui::End();
    }
}
