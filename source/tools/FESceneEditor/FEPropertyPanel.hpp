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
/// 节点挂有约束时,在变换分组之后追加对应约束的参数分组(平移/跟随/朝向/路径/驱动),
/// 没有该约束则不显示该组。
/// </summary>

#pragma     once

#include    "FECollapseGroup.hpp"
#include    "FETimeLine.hpp"

#include    "../../../inc/FEEulerObject.hpp"
#include    "../../../inc/constraint/FEConstraintTrans.hpp"
#include    "../../../inc/constraint/FEConstraintFollow.hpp"
#include    "../../../inc/constraint/FEConstraintLookAt.hpp"
#include    "../../../inc/constraint/FEConstraintPath.hpp"
#include    "../../../inc/constraint/FEConstraintDrive.hpp"
#include    "../../../inc/node/FENode.hpp"

#include    <cstdio>

namespace   FE
{
    class   FEPropertyPanel
    {
    public:
        FEPropertyPanel() = default;
        /// <summary>
        /// 拾取约束目标期间固定显示的对象(为空表示未在拾取);
        /// 宿主据此保持属性面板停留在约束所有者节点上,不随拾取点击切换
        /// </summary>
        const   Object&     pickingOwner() const
        {
            return  _pickOwner;
        }
        /// <summary>
        /// 取走"拾取结束后需要把模型树选中恢复到的对象"(取走后清空)
        /// </summary>
        Object  takeRestoreSelection()
        {
            Object  object  =   _restoreSelection;
            _restoreSelection   =   Object();
            return  object;
        }
        /// <summary>
        /// 绘制三个变换分组、颜色分组与节点已挂载的约束分组;node 为空时显示提示文字
        /// </summary>
        /// <param name="node">当前选中节点</param>
        /// <param name="timeLine">时间线(提供当前时间的关键帧状态与添加/移除),可为 nullptr</param>
        /// <param name="selection">当前选中对象列表(约束的目标节点拾取/驱动节点加入使用),可为 nullptr</param>
        void    draw(FENode* node,FETimeLine* timeLine,const Objects* selection = nullptr)
        {
            if (node == nullptr)
            {
                ImGui::TextDisabled("未选择节点");
                return;
            }
            drawGroup(_translate,node,GK_TRANSLATE,timeLine);
            drawGroup(_scale,node,GK_SCALE,timeLine);
            drawGroup(_rotate,node,GK_ROTATE,timeLine);
            drawColorGroup(node,timeLine);
            drawVisibleGroup(node,timeLine);
            drawConstraints(node,selection);
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
            /// 播放中禁止编辑:播放驱动每帧用采样值覆盖节点,编辑结果不会保留
            /// </summary>
            const   bool    locked  =   timeLine != nullptr && timeLine->isPlaying();

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
                /// 节点累计欧拉角账本(度):拖拽/打帧后保留圈数,可超过 ±180;
                /// 账本失效时节点会退回四元数换算(范围 ±180)
                current =   node->localEuler();
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

            if (edited && !locked)
            {
                const   real3   next(values[0],values[1],values[2]);
                if (kind == GK_TRANSLATE)
                    node->setLocalTranslation(next);
                else if (kind == GK_SCALE)
                    node->setLocalScaling(next);
                else
                    /// 角度(度),直接写入累计欧拉角账本,不做 ±180 归一化
                    node->setLocalEuler(next);
            }
            group.end();
        }
        /// <summary>
        /// 颜色分组内容:RGB 与 Alpha 两行,分别对应独立的关键帧轨道,可单独控制
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="timeLine">时间线,可为 nullptr(此时 check 只显示不响应)</param>
        void    drawColorGroup(FENode* node,FETimeLine* timeLine)
        {
            if (!_colorGroup.begin())
            {
                _colorGroup.end();
                return;
            }
            const   bool    locked  =   timeLine != nullptr && timeLine->isPlaying();

            /// 每帧从节点读取颜色,保证动画/脚本等外部改动能同步显示
            const   Rgba8   color   =   node->color();
            float           rgb[3]  =
            {
                float(color.value().r) / 255.0f,
                float(color.value().g) / 255.0f,
                float(color.value().b) / 255.0f,
            };
            float           alpha   =   float(color.value().a) / 255.0f;

            bool    rgbKeyed    =   timeLine != nullptr && timeLine->hasKeyframe(PROP_COLOR_RGB);
            bool    alphaKeyed  =   timeLine != nullptr && timeLine->hasKeyframe(PROP_COLOR_ALPHA);
            bool    rgbToggled  =   false;
            bool    alphaToggled    =   false;
            bool    edited      =   false;
            if (propertyRow("RGB",kColorLabelWidth,[&]()
                {
                    return  ImGui::ColorEdit3("##color",rgb);
                },rgbKeyed,&rgbToggled))
                edited  =   true;
            if (propertyRow("Alpha",kColorLabelWidth,[&]()
                {
                    return  ImGui::DragFloat("##alpha",&alpha,0.01f,0.0f,1.0f,"%.3f");
                },alphaKeyed,&alphaToggled))
                edited  =   true;

            /// 单独点击 check:按对应属性添加/移除当前时间的关键帧
            if (rgbToggled && timeLine != nullptr)
                timeLine->toggleKeyframe(PROP_COLOR_RGB);
            if (alphaToggled && timeLine != nullptr)
                timeLine->toggleKeyframe(PROP_COLOR_ALPHA);

            if (edited && !locked)
            {
                node->setColor(Rgba8(colorByte(rgb[0]),colorByte(rgb[1])
                                     ,colorByte(rgb[2]),colorByte(alpha)));
            }
            _colorGroup.end();
        }
        /// <summary>
        /// 显隐分组内容:一个"显示"勾选框,右侧为 PROP_VISIBLE 关键帧勾选框
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="timeLine">时间线,可为 nullptr(此时 check 只显示不响应)</param>
        void    drawVisibleGroup(FENode* node,FETimeLine* timeLine)
        {
            if (!_visibleGroup.begin())
            {
                _visibleGroup.end();
                return;
            }
            const   bool    locked  =   timeLine != nullptr && timeLine->isPlaying();
            bool            visible =   node->flags().hasFlag(FLAG_VISIBLE);
            bool            keyed   =   timeLine != nullptr && timeLine->hasKeyframe(PROP_VISIBLE);
            bool            toggled =   false;
            const   bool    edited  =   propertyRow("显示",kColorLabelWidth,[&]()
            {
                return  ImGui::Checkbox("##visible",&visible);
            },keyed,&toggled);

            if (toggled && timeLine != nullptr)
                timeLine->toggleKeyframe(PROP_VISIBLE);
            if (edited && !locked)
                setNodeVisible(node,visible);
            _visibleGroup.end();
        }
        /// <summary>
        /// 设置节点显隐:与模型树一致,显示时额外打 FLAG_UPDATE,并通知渲染刷新
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="visible">是否显示</param>
        static  void    setNodeVisible(FENode* node,bool visible)
        {
            if (visible)
                node->flags().addFlag(FLAG_VISIBLE | FLAG_UPDATE);
            else
                node->flags().removeFlag(FLAG_VISIBLE);
            node->flags().addFlag(FENode::FLAG_PROP_STATE);
            node->fireChanged();
        }
        /// <summary>
        /// 绘制节点已挂载的约束分组:没有对应约束时整组不显示
        /// </summary>
        /// <param name="node">当前节点</param>
        /// <param name="selection">当前选中对象列表,可为 nullptr</param>
        void    drawConstraints(FENode* node,const Objects* selection)
        {
            _selection  =   selection;
            _ownerNode  =   node;
            /// 拾取状态下把模型树/场景当前选中的(非所有者)节点记录为候选目标
            capturePickCandidate();

            const   auto    trans   =   node->objects<FEConstraintTrans>();
            if (!trans.empty())
            {
                if (_transGroup.begin())
                    drawTransBody(trans.front());
                _transGroup.end();
            }
            const   auto    follow  =   node->objects<FEConstraintFollow>();
            if (!follow.empty())
            {
                if (_followGroup.begin())
                    drawTargetBody(follow.front());
                _followGroup.end();
            }
            const   auto    lookAt  =   node->objects<FEConstraintLookAt>();
            if (!lookAt.empty())
            {
                if (_lookAtGroup.begin())
                    drawLookAtBody(lookAt.front());
                _lookAtGroup.end();
            }
            const   auto    path    =   node->objects<FEConstraintPath>();
            if (!path.empty())
            {
                if (_pathGroup.begin())
                    drawPathBody(path.front());
                _pathGroup.end();
            }
            const   auto    drive   =   node->objects<FEConstraintDrive>();
            if (!drive.empty())
            {
                if (_driveGroup.begin())
                    drawDriveBody(drive.front());
                _driveGroup.end();
            }
            _selection  =   nullptr;
            _ownerNode  =   nullptr;
        }
        /// <summary>
        /// 平移约束分组内容:每轴一行"最小值 + 最大值 + 启用勾选框"
        /// </summary>
        /// <param name="con">平移约束</param>
        void    drawTransBody(FEConstraintTrans* con)
        {
            static  const   char*   kAxis[3]    =   {   "X","Y","Z"   };
            for (int i = 0; i < 3; ++i)
            {
                float   minValue    =   float(con->minValue()[i]);
                float   maxValue    =   float(con->maxValue()[i]);
                bool    enabled     =   con->axisLimit(i);
                if (!rangeRow(kAxis[i],minValue,maxValue,enabled))
                    continue;
                real3   minValues   =   con->minValue();
                real3   maxValues   =   con->maxValue();
                minValues[i]    =   minValue;
                maxValues[i]    =   maxValue;
                con->setMinValue(minValues);
                con->setMaxValue(maxValues);
                con->setAxisLimit(i,enabled);
            }
        }
        /// <summary>
        /// 朝向约束分组内容:启用/拾取/注视/清除 + 对象名称框 + 目标点 + 上方向
        /// </summary>
        /// <param name="con">朝向约束</param>
        void    drawLookAtBody(FEConstraintLookAt* con)
        {
            /// 行1:启用 + 拾取(切换拾取状态) + 注视(绑定候选目标) + 清除(解除绑定)
            bool    enabled =   con->isEnable();
            if (ImGui::Checkbox("启用",&enabled))
                con->setEnable(enabled);
            ImGui::SameLine();
            if (pickButton())
                beginPick();
            ImGui::SameLine();
            if (ImGui::Button("注视"))
                bindPickTarget(con);
            ImGui::SameLine();
            if (ImGui::Button("清除"))
            {
                con->setTargetNode(Node());
                finishPick();
            }
            /// 行2:对象名称编辑框(只读;只显示已绑定目标,拾取中的候选对象不提前显示)
            Node    shown   =   con->targetNode();
            const   char*   name    =   (shown != nullptr && shown->name() != nullptr)
                                      ? shown->name() : "无";
            char    buffer[256] =   {};
            std::snprintf(buffer,sizeof(buffer),"%s",name);
            ImGui::PushItemWidth(-1.0f);
            ImGui::InputText("##target",buffer,sizeof(buffer),ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();
            drawVectorRows("目标点",con->target()
                           ,[con](const real3& point){ con->setTarget(point); });
            drawVectorRows("上方向",con->upAxis()
                           ,[con](const real3& up){ con->setUpAxis(up); });
        }
        /// <summary>
        /// 拾取按钮:拾取期间高亮显示。
        /// 是否压栈要在点击前固定,否则点击瞬间切换状态会造成样式栈失配。
        /// </summary>
        /// <returns>true 表示本次被点击</returns>
        bool    pickButton()
        {
            const   bool    highlighted =   _picking;
            if (highlighted)
            {
                const   ImGuiStyle& style   =   ImGui::GetStyle();
                ImGui::PushStyleColor(ImGuiCol_Button,style.Colors[ImGuiCol_ButtonActive]);
            }
            const   bool    clicked =   ImGui::Button("拾取");
            if (highlighted)
                ImGui::PopStyleColor();
            return  clicked;
        }
        /// <summary>
        /// 进入/退出拾取状态:拾取期间固定属性面板显示约束所有者节点,
        /// 模型树或场景中点击的对象只作为候选目标,不切换面板
        /// </summary>
        void    beginPick()
        {
            if (_picking)
            {
                finishPick();
                return;
            }
            _picking        =   true;
            _pickOwner      =   Object(_ownerNode);
            _pickCandidate  =   Node();
        }
        /// <summary>
        /// 结束拾取:清空状态,并请求宿主把模型树选中恢复回约束所有者节点
        /// </summary>
        void    finishPick()
        {
            if (_picking && _pickOwner)
                _restoreSelection   =   _pickOwner;
            _picking        =   false;
            _pickOwner      =   Object();
            _pickCandidate  =   Node();
        }
        /// <summary>
        /// 把拾取到的候选对象(没有候选时取模型树当前选中节点)绑定为朝向目标
        /// </summary>
        /// <param name="con">朝向约束</param>
        void    bindPickTarget(FEConstraintLookAt* con)
        {
            Node    target  =   _pickCandidate;
            if (target == nullptr)
                target  =   firstSelectedNode();
            if (target != nullptr)
                con->setTargetNode(target);
            finishPick();
        }
        /// <summary>
        /// 把拾取到的候选对象(没有候选时取模型树当前选中节点)加入被驱动节点列表
        /// </summary>
        /// <param name="con">驱动约束</param>
        void    addPickTargetToDrive(FEConstraintDrive* con)
        {
            Node    target  =   _pickCandidate;
            if (target == nullptr)
                target  =   firstSelectedNode();
            if (target != nullptr)
                con->addDrivenNode(target);
            finishPick();
        }
        /// <summary>
        /// 拾取状态下从模型树/场景当前选中列表取候选目标(排除所有者自身)
        /// </summary>
        void    capturePickCandidate()
        {
            if (!_picking || _selection == nullptr)
                return;
            for (const Object& object : *_selection)
            {
                if (!object)
                    continue;
                FEObject*   raw     =   const_cast<FEObject*>(object.get());
                Node        node    =   raw->cast<FENode>();
                if (node == nullptr || node.get() == _ownerNode)
                    continue;
                _pickCandidate  =   node;
                return;
            }
        }
        /// <summary>
        /// 跟随约束分组内容:启用 + 目标节点 + 目标点 + 保持偏移
        /// </summary>
        template<class TConstraint>
        void    drawTargetBody(TConstraint* con)
        {
            drawEnableRow(con);
            drawNodePickerRow("目标节点",con->targetNode()
                              ,[con](Node target){ con->setTargetNode(target); });
            drawVectorRows("目标点",con->target()
                           ,[con](const real3& point){ con->setTarget(point); });
            bool    maintain    =   con->maintainOffset();
            if (ImGui::Checkbox("保持偏移",&maintain))
                con->setMaintainOffset(maintain);
        }
        /// <summary>
        /// 路径约束分组内容:路径点数量 + 速度 + 起点偏移 + 循环 + 清空
        /// </summary>
        /// <param name="con">路径约束</param>
        void    drawPathBody(FEConstraintPath* con)
        {
            drawEnableRow(con);
            ImGui::TextDisabled("路径点 %d 个",int(con->pointCount()));
            float   speed   =   float(con->speed());
            if (axisRow("速度",speed))
                con->setSpeed(speed);
            float   offset  =   float(con->startOffset());
            if (axisRow("偏移",offset))
                con->setStartOffset(offset);
            bool    loop    =   con->loop();
            if (ImGui::Checkbox("循环",&loop))
                con->setLoop(loop);
            if (ImGui::Button("清空路径点"))
                con->clearPoints();
        }
        /// <summary>
        /// 驱动约束分组内容:启用/拾取/添加/删除/清除 + 被驱动节点列表
        /// </summary>
        /// <param name="con">驱动约束</param>
        void    drawDriveBody(FEConstraintDrive* con)
        {
            /// 行1:启用 + 拾取(切换拾取状态) + 添加(把候选目标加入列表) + 删除(移除选中项) + 清除(清空列表)
            drawEnableRow(con);
            ImGui::SameLine();
            if (pickButton())
                beginPick();
            ImGui::SameLine();
            if (ImGui::Button("添加"))
                addPickTargetToDrive(con);

            /// 选中项已不在列表中(节点被删除或列表被清空)时自动失效,删除按钮随之变灰
            bool    hasSelected =   false;
            for (auto& var : con->drivenNodes())
            {
                if (var._node != nullptr && var._node == _driveSelected)
                {
                    hasSelected =   true;
                    break;
                }
            }
            if (!hasSelected)
                _driveSelected  =   Node();

            ImGui::SameLine();
            if (!hasSelected)
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha * 0.45f);
            const   bool    removeClicked   =   ImGui::Button("删除");
            if (!hasSelected)
                ImGui::PopStyleVar();
            if (removeClicked && hasSelected)
            {
                con->removeDrivenNode(_driveSelected);
                _driveSelected  =   Node();
            }
            ImGui::SameLine();
            if (ImGui::Button("清除"))
            {
                con->clearDrivenNodes();
                _driveSelected  =   Node();
            }

            /// 行2:被驱动节点列表(可包含多个对象,单击选中,选中后"删除"才可用)
            ImGui::TextDisabled("被驱动节点 %d 个",int(con->drivenNodeCount()));
            int     index   =   0;
            for (auto& var : con->drivenNodes())
            {
                const   char*   name    =   (var._node != nullptr && var._node->name() != nullptr)
                                          ? var._node->name() : "(空)";
                ImGui::PushID(index++);
                if (ImGui::Selectable(name,var._node == _driveSelected))
                    _driveSelected  =   var._node;
                ImGui::PopID();
            }
        }
        /// <summary>
        /// 启用行(映射组件的 FLAG_ENABLE)
        /// </summary>
        /// <param name="con">约束组件</param>
        void    drawEnableRow(FEConstraint* con)
        {
            bool    enabled =   con->isEnable();
            if (ImGui::Checkbox("启用",&enabled))
                con->setEnable(enabled);
        }
        /// <summary>
        /// 目标节点行:名称 + 拾取选中(取模型树当前选中节点) + 清除
        /// </summary>
        /// <param name="label">行标签(组内唯一)</param>
        /// <param name="target">当前目标节点</param>
        /// <param name="setter">写回目标节点(空 Node 表示清除)</param>
        template<class Setter>
        void    drawNodePickerRow(const char* label,Node target,const Setter& setter)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine();
            const   char*   text    =   (target != nullptr && target->name() != nullptr)
                                      ? target->name() : "无";
            const   char*   show    =   (text != nullptr && text[0] != '\0') ? text : "(未命名)";
            ImGui::TextDisabled("%s",show);
            ImGui::SameLine();
            if (ImGui::SmallButton("拾取选中"))
            {
                Node    picked  =   firstSelectedNode();
                if (picked != nullptr)
                    setter(picked);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("清除"))
                setter(Node());
        }
        /// <summary>
        /// 三分量向量:每轴一行(标签 + 数值),修改后回调写回
        /// </summary>
        /// <param name="label">分组标题(组内唯一)</param>
        /// <param name="current">当前值</param>
        /// <param name="setter">写回回调</param>
        template<class Setter>
        static  void    drawVectorRows(const char* label,const real3& current,const Setter& setter)
        {
            ImGui::PushID(label);
            ImGui::TextDisabled("%s",label);
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
                if (axisRow(kAxis[i],values[i]))
                    edited  =   true;
            }
            ImGui::PopID();
            if (edited)
                setter(real3(values[0],values[1],values[2]));
        }
        /// <summary>
        /// 属性行:标签 + 任意编辑控件(由 draw 绘制) + 关键帧勾选框
        /// </summary>
        /// <param name="label">行标签(组内唯一)</param>
        /// <param name="labelWidth">标签列宽度(像素)</param>
        /// <param name="draw">绘制编辑控件的回调,返回 true 表示被修改</param>
        /// <param name="keyed">当前时间该属性是否有关键帧(输入输出)</param>
        /// <param name="keyToggled">本次勾选框是否被点击(可为 nullptr)</param>
        /// <returns>true 表示数值被修改</returns>
        template<class Draw>
        static  bool    propertyRow(const char* label,float labelWidth,const Draw& draw,
                                    bool& keyed,bool* keyToggled = nullptr)
        {
            const   ImVec2  start   =   ImGui::GetCursorScreenPos();
            const   float   frameH  =   ImGui::GetFrameHeight();
            const   float   avail   =   ImGui::GetContentRegionAvail().x;
            const   float   checkW  =   frameH;
            const   float   gap     =   6.0f;
            float           valueW  =   avail - labelWidth - checkW - gap;
            if (valueW < 80.0f)
                valueW  =   80.0f;

            ImDrawList*     dl      =   ImGui::GetWindowDrawList();
            dl->AddText(ImVec2(start.x + 2.0f,start.y + (frameH - ImGui::GetFontSize()) * 0.5f),
                        FE::ui::kTextColor,label);

            bool    changed =   false;
            ImGui::PushID(label);
            ImGui::SetCursorScreenPos(ImVec2(start.x + labelWidth,start.y));
            ImGui::PushItemWidth(valueW);
            changed =   draw();
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos(ImVec2(start.x + labelWidth + valueW + gap,start.y));
            if (ImGui::Checkbox("##keyed",&keyed) && keyToggled != nullptr)
                *keyToggled =   true;
            ImGui::PopID();

            /// 光标移到下一行
            ImGui::SetCursorScreenPos(ImVec2(start.x,start.y + frameH + 2.0f));
            ImGui::Dummy(ImVec2(0.0f,0.0f));
            return  changed;
        }
        /// <summary>
        /// 归一化浮点颜色分量转 8 位(0..1 → 0..255)
        /// </summary>
        static  uint8   colorByte(float value)
        {
            if (value < 0.0f)
                value   =   0.0f;
            if (value > 1.0f)
                value   =   1.0f;
            return  uint8(value * 255.0f + 0.5f);
        }
        /// <summary>
        /// 约束分组中的一行:标签 + 单个数值编辑框(不带关键帧勾选框)
        /// </summary>
        /// <param name="label">行标签(组内唯一)</param>
        /// <param name="value">数值(输入输出)</param>
        /// <param name="speed">拖动步进</param>
        /// <returns>true 表示数值被修改</returns>
        static  bool    axisRow(const char* label,float& value,float speed = 0.01f)
        {
            ImGui::PushID(label);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine(kValueLabelWidth);
            ImGui::PushItemWidth(-1.0f);
            const   bool    changed =   ImGui::DragFloat("##value",&value,speed,0.0f,0.0f,"%.3f");
            ImGui::PopItemWidth();
            ImGui::PopID();
            return  changed;
        }
        /// <summary>
        /// 约束取值范围行:标签 + 最小值 + 最大值 + 启用勾选框
        /// </summary>
        /// <param name="label">行标签(组内唯一)</param>
        /// <param name="minValue">最小值(输入输出)</param>
        /// <param name="maxValue">最大值(输入输出)</param>
        /// <param name="enabled">该轴是否限制范围(输入输出)</param>
        /// <returns>true 表示数值或勾选框被修改</returns>
        static  bool    rangeRow(const char* label,float& minValue,float& maxValue,bool& enabled)
        {
            ImGui::PushID(label);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::SameLine(kValueLabelWidth);
            /// 数值框各占一半,勾选框放在行尾
            const   float   avail   =   ImGui::GetContentRegionAvail().x;
            const   float   checkW  =   ImGui::GetFrameHeight();
            const   float   gap     =   6.0f;
            float           itemW   =   (avail - checkW - gap * 2.0f) * 0.5f;
            if (itemW < 40.0f)
                itemW   =   40.0f;
            bool    changed =   false;
            ImGui::PushItemWidth(itemW);
            changed =   ImGui::DragFloat("##min",&minValue,0.01f,0.0f,0.0f,"%.3f");
            ImGui::SameLine();
            changed |=  ImGui::DragFloat("##max",&maxValue,0.01f,0.0f,0.0f,"%.3f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            changed |=  ImGui::Checkbox("##limit",&enabled);
            ImGui::PopID();
            return  changed;
        }
        /// <summary>
        /// 选中列表中的第一个节点(排除约束所有者自身)
        /// </summary>
        /// <returns>节点;没有可用节点时返回空</returns>
        Node    firstSelectedNode() const
        {
            if (_selection == nullptr)
                return  Node();
            for (const Object& object : *_selection)
            {
                if (!object)
                    continue;
                FEObject*   raw     =   const_cast<FEObject*>(object.get());
                Node        node    =   raw->cast<FENode>();
                if (node != nullptr && node.get() != _ownerNode)
                    return  node;
            }
            return  Node();
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
        /// <summary>
        /// 约束行的标签列宽度(像素):保证中文标签不与数值框重叠
        /// </summary>
        static  constexpr   float   kValueLabelWidth   =   64.0f;
        /// <summary>
        /// 颜色行标签列宽度(像素):容纳 "RGB" / "Alpha"
        /// </summary>
        static  constexpr   float   kColorLabelWidth   =   40.0f;
    protected:
        /// <summary>
        /// 当前选中对象列表(约束的目标节点拾取/驱动节点加入)
        /// </summary>
        const   Objects*        _selection     =   nullptr;
        /// <summary>
        /// 当前属性面板所属节点(拾取/驱动时排除自身)
        /// </summary>
        FENode*                 _ownerNode     =   nullptr;
        /// <summary>
        /// 平移约束分组
        /// </summary>
        FECollapseGroup     _transGroup     =   FECollapseGroup("平移约束");
        /// <summary>
        /// 跟随约束分组
        /// </summary>
        FECollapseGroup     _followGroup    =   FECollapseGroup("跟随约束");
        /// <summary>
        /// 朝向约束分组
        /// </summary>
        FECollapseGroup     _lookAtGroup    =   FECollapseGroup("朝向约束");
        /// <summary>
        /// 路径约束分组
        /// </summary>
        FECollapseGroup     _pathGroup      =   FECollapseGroup("路径约束");
        /// <summary>
        /// 驱动约束分组
        /// </summary>
        FECollapseGroup     _driveGroup     =   FECollapseGroup("驱动约束");
        /// <summary>
        /// 是否处于约束目标拾取状态
        /// </summary>
        bool                _picking        =   false;
        /// <summary>
        /// 拾取期间固定显示的约束所有者对象(为空表示未拾取)
        /// </summary>
        Object              _pickOwner;
        /// <summary>
        /// 拾取过程中记录的候选目标节点
        /// </summary>
        Node                _pickCandidate;
        /// <summary>
        /// 驱动约束列表中被选中的节点(选中后"删除"按钮才可用)
        /// </summary>
        Node                _driveSelected;
        /// <summary>
        /// 拾取结束后需要恢复到模型树选中的对象(由宿主取走)
        /// </summary>
        Object              _restoreSelection;
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
        /// <summary>
        /// 颜色分组(RGB 与 Alpha 两行,可分别打关键帧)
        /// </summary>
        FECollapseGroup     _colorGroup =   FECollapseGroup("颜色");
        /// <summary>
        /// 显隐分组(显示勾选框 + 关键帧)
        /// </summary>
        FECollapseGroup     _visibleGroup   =   FECollapseGroup("显隐");
    };
}
