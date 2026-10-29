/// <summary>
/// FEPropertyPanel
/// 属性面板内容:由多个 FECollapseGroup 组成,每组三行 "X/Y/Z: 值 + check"。
///   - 位置 / 缩放:直接读写节点的本地变换(real3);
///   - 旋转:节点的本地四元数 ↔ 欧拉角(度)互转(默认 ZYX 顺序);
///   - check 不控制编辑,只表示"当前时间该属性是否有关键帧":
///       时间线停在关键帧上时自动勾选;单独点击则添加对应属性的关键帧
///       (已有关键帧时点击则移除)。每行对应的属性索引:
///       PROP_TRANSFORM_X/Y/Z、PROP_SCALE_X/Y/Z、PROP_ROTATE_X/Y/Z。
/// 数据每帧从节点读取,编辑后立即写回节点,因此抓手/动画等外部改动也会同步显示。
/// 面板不持有节点,节点与时间线由调用方每帧传入。
/// </summary>

#pragma     once

#include    "FECollapseGroup.hpp"
#include    "FETimeLine.hpp"

#include    "../../../inc/FEEulerObject.hpp"
#include    "../../../inc/node/FENode.hpp"

namespace   FE
{
    class   FEPropertyPanel
    {
    public:
        FEPropertyPanel() = default;
        /// <summary>
        /// 绘制三个变换分组;node 为空时显示提示文字
        /// </summary>
        /// <param name="node">当前选中节点</param>
        /// <param name="timeLine">时间线(提供当前时间的关键帧状态与添加/移除),可为 nullptr</param>
        void    draw(FENode* node,FETimeLine* timeLine)
        {
            if (node == nullptr)
            {
                ImGui::TextDisabled("未选择节点");
                return;
            }
            drawGroup(_translate,node,GK_TRANSLATE,timeLine);
            drawGroup(_scale,node,GK_SCALE,timeLine);
            drawGroup(_rotate,node,GK_ROTATE,timeLine);
        }
    protected:
        /// <summary>
        /// 变换分组种类(同时作为 _props 与 GroupKind 的下标)
        /// </summary>
        enum    GroupKind
        {
            GK_TRANSLATE    =   0,
            GK_SCALE        =   1,
            GK_ROTATE       =   2,
            GK_COUNT        =   3,
        };
        /// <summary>
        /// 各分组的 X/Y/Z 行对应的属性索引(关键帧轨道按属性索引区分)
        /// </summary>
        static  PropIndex       propsOf(int kind,int axis)
        {
            switch (kind)
            {
            case    GK_TRANSLATE:
            {
                static  const   PropIndex   kProps[3]   =   {   PROP_TRANSFORM_X,PROP_TRANSFORM_Y,PROP_TRANSFORM_Z   };
                return  kProps[axis];
            }
            case    GK_SCALE:
            {
                static  const   PropIndex   kProps[3]   =   {   PROP_SCALE_X,PROP_SCALE_Y,PROP_SCALE_Z   };
                return  kProps[axis];
            }
            default:
            {
                static  const   PropIndex   kProps[3]   =   {   PROP_ROTATE_X,PROP_ROTATE_Y,PROP_ROTATE_Z  };
                return  kProps[axis];
            }
            }
        }
        /// <summary>
        /// 绘制一个分组:组内为 X/Y/Z 三行(label + 数值 + check)
        /// </summary>
        /// <param name="group">分组控件(持有折叠状态)</param>
        /// <param name="node">目标节点</param>
        /// <param name="kind">变换种类</param>
        /// <param name="timeLine">时间线,可为 nullptr(此时 check 只显示不响应)</param>
        void    drawGroup(FECollapseGroup& group,FENode* node,int kind,FETimeLine* timeLine)
        {
            if (!group.begin())
            {
                group.end();
                return;
            }

            /// <summary>
            /// 每帧从节点读取当前值,保证抓手/动画等外部改动能反映到面板
            /// </summary>
            real3           current;
            float           speed   =   kTransSpeed;
            const   char*   fmt     =   "%.3f";
            if (kind == GK_TRANSLATE)
            {
                current =   node->localTranslation();
            }
            else if (kind == GK_SCALE)
            {
                current =   node->localScaling();
            }
            else
            {
                /// quatToEuler 返回角度(度)
                current =   quatToEuler(node->localRotation());
                speed   =   kRotSpeed;
                fmt     =   "%.1f";
            }

            static  const   char*   kAxis[3]    =   {   "X","Y","Z"   };
            float   values[3]   =
            {
                float(current.x),
                float(current.y),
                float(current.z),
            };

            bool    edited  =   false;
            for (int i = 0; i < 3; ++i)
            {
                const   PropIndex   prop    =   propsOf(kind,i);
                bool    keyed   =   timeLine != nullptr && timeLine->hasKeyframe(prop);
                bool    keyToggled  =   false;
                if (FECollapseGroup::valueRow(kAxis[i],values[i],keyed,speed,fmt,&keyToggled))
                    edited  =   true;
                /// <summary>
                /// 单独点击 check:按该行属性添加/移除当前时间的关键帧
                /// </summary>
                if (keyToggled && timeLine != nullptr)
                    timeLine->toggleKeyframe(prop);
            }

            if (edited)
            {
                const   real3   next(values[0],values[1],values[2]);
                if (kind == GK_TRANSLATE)
                    node->setLocalTranslation(next);
                else if (kind == GK_SCALE)
                    node->setLocalScaling(next);
                else
                    /// eulerToQuat 接受角度(度)
                    node->setLocalRotation(eulerToQuat(next));
            }
            group.end();
        }
    protected:
        /// <summary>
        /// 位置/缩放的拖动步进
        /// </summary>
        static  constexpr   float   kTransSpeed    =   0.01f;
        /// <summary>
        /// 旋转(角度)的拖动步进
        /// </summary>
        static  constexpr   float   kRotSpeed      =   0.5f;
    protected:
        /// <summary>
        /// 位置分组
        /// </summary>
        FECollapseGroup     _translate  =   FECollapseGroup("位置");
        /// <summary>
        /// 缩放分组
        /// </summary>
        FECollapseGroup     _scale      =   FECollapseGroup("缩放");
        /// <summary>
        /// 旋转分组(欧拉角,单位:度)
        /// </summary>
        FECollapseGroup     _rotate     =   FECollapseGroup("旋转");
    };
}
