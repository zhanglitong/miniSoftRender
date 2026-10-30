/// summary
/// FEUiStrings
/// 编辑器共享的用户可见文案:按钮、搜索、约束名、时间线提示等。
/// 只放多处复用的文案,单处使用的文字仍留在原调用处。
/// </summary>

#pragma     once

namespace   FE
{
namespace   ui
{
namespace   text
{
    /// <summary>
    /// 搜索栏
    /// </summary>
    constexpr   const char*     kSearch        =   "搜索";
    constexpr   const char*     kCaseMatchShort    =   "Aa";
    constexpr   const char*     kWholeMatchShort   =   "全";
    constexpr   const char*     kCaseMatch     =   "大小写匹配";
    constexpr   const char*     kWholeMatch    =   "全匹配";
    constexpr   const char*     kNoMatch       =   "无匹配项";

    /// <summary>
    /// 通用按钮
    /// </summary>
    constexpr   const char*     kConfirm       =   "确定";
    constexpr   const char*     kCancel        =   "取消";
    constexpr   const char*     kClose         =   "关闭";
    constexpr   const char*     kDelete        =   "删除";
    constexpr   const char*     kReset         =   "重置";
    constexpr   const char*     kClear         =   "清除";
    constexpr   const char*     kNone          =   "无";

    /// <summary>
    /// 约束
    /// </summary>
    constexpr   const char*     kConstraint        =   "约束";
    constexpr   const char*     kTransConstraint   =   "平移约束";
    constexpr   const char*     kFollowConstraint  =   "跟随约束";
    constexpr   const char*     kLookAtConstraint  =   "朝向约束";
    constexpr   const char*     kPathConstraint    =   "路径约束";
    constexpr   const char*     kDriveConstraint   =   "驱动约束";
    constexpr   const char*     kAddConstraint     =   "添加约束";
    constexpr   const char*     kRemoveConstraint  =   "移除约束";

    /// <summary>
    /// 动画 / 播放器
    /// </summary>
    constexpr   const char*     kCreateAnimation   =   "创建动画";
    constexpr   const char*     kDeleteAnimation   =   "删除动画";
    constexpr   const char*     kAddKeyframe       =   "添加关键帧";
    constexpr   const char*     kDeleteKeyframe    =   "删除关键帧";
    constexpr   const char*     kPlayback          =   "播放器";
    constexpr   const char*     kCreatePlayback    =   "创建播放器";
    constexpr   const char*     kDeletePlayback    =   "删除播放器";
    constexpr   const char*     kRenamePlayback    =   "重命名播放器";

    /// <summary>
    /// 默认播放器:引擎存储名 / 界面显示名
    /// </summary>
    constexpr   const char*     kDefaultPlaybackStorage  =   "default";
    constexpr   const char*     kDefaultPlaybackDisplay  =   "默认播放器";

    /// <summary>
    /// 时间线状态提示(多处复用)
    /// </summary>
    constexpr   const char*     kSelectNodeFirst       =   "请先在模型树中选择节点";
    constexpr   const char*     kAnimationMissingNode  =   "动画没有所属节点";
    constexpr   const char*     kAnimationMissingClip  =   "动画没有可用的 clip";
    constexpr   const char*     kNoKeyframeChange      =   "当前帧没有产生关键帧变化";
    constexpr   const char*     kAddToPlaybackHint     =   "请先在模型树右键「动画」子菜单「添加到播放器」";
}
}
}
