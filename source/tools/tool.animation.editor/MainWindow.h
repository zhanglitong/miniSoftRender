#pragma     once

#include    <QtWidgets/QMainWindow>
#include    <QUndoStack>
#include    <QShortcut>
#include    <QMessageBox>
#include    <string>
#include    <map>
#include    <vector>
#include    "ui_MainWindow.h"
#include    "UndoCommand.h"

class   MainWindow;
class   UiProperties;
extern  MainWindow* _mainApp;

enum    ImageIndex
{
    ID_UNCHECK,
    ID_CHECK,
    ID_HALF,
    ID_SUB,
    ID_PLUS,
    /// 工程图标索引
    ID_PRJ,
    /// 节点
    ID_NODE,
    /// 几何体
    ID_GEO,
    /// 图层
    ID_LAYER,
    /// image
    ID_LAYER_IMAGE,
    /// dem
    ID_LAYER_DEM,
    /// 标会图层图标索引
    ID_LAYER_MARK,
    /// 动画图标
    ID_ANIMATION,
    /// 属性图标
    ID_ATTRIBUTE,
    /// 材质
    ID_MATERIAL,
    /// 数据数据
    ID_DATA_OBJECT,
    /// 定时器图标
    ID_TIMER,
    /// 矢量图层
    ID_LAYER_VEC,
    /// 视频
    ID_VIDEO,
    /// 声音
    ID_SOUND,
    /// UI
    ID_UI,
    /// 图片
    ID_IMAGE,
    /// 倾斜摄影
    ID_QXSY,
    /// 数据集合
    ID_FILES,
    /// 日光
    ID_SUN,
    /// 聚光灯
    ID_SPOT,
    /// 水面
    ID_WATER,
    /// 音乐
    ID_MUSIC,
    /// <summary>
    /// 对象
    /// </summary>
    ID_OBJECT,
};

#define     Q2C(qstr)     (qstr.toUtf8().data())
///const char* 转换为QString 
#define     C2Q(cstr)     (QString::fromUtf8(cstr))

#define     OBJECT_ICON(type)               _mainApp->objectIcon(type)
#define     ROW_HEIGHT                      _mainApp->rowHeight()
#define     UNDO_STACK                      _mainApp->undoStack()

class   MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
    /// <summary>
    /// 获取场景
    /// </summary>
    /// <returns></returns>
    Scene   scene();
    /// <summary>
    /// 设置标题栏
    /// </summary>
    /// <param name="fileName"></param>
    void    setTitile(QString fileName);
    QIcon   objectIcon(ImageIndex type);
    QRect   objectIconRect(ImageIndex type);
    int     rowHeight() const;

    auto    undoStack() const
    {
        return _undoStack;
    }
    /// 时间线当前时间(秒),供属性面板取关键帧时间
    double  curTime() const;
    /// AUTO 自动插帧开关
    bool    autoKeyframe() const;
    /// 刷新属性面板(选中变化/undo redo/关键帧增删后)
    void    noticeUpdatePropsUi();
    /// undo/redo 回调期间屏蔽属性控件实时写关键帧
    void    blockRealtimeUpdate(bool block);
    /// 刷新动画树
    void    refreshAnimTree();
    /// 刷新时间线与场景关键帧状态
    void    refreshTimeline();
    /// 刷新模型树(节点增删/重挂/排序后)
    void    refreshModelTree();
public slots:
    void    slotImportModel();
    void    slotOpenProject();
    void    slotSaveProject();

    void    slotReset();
    void    slotRedo();
    void    slotUndo();

    void    slotMoveEditor();
    void    slotRotEditor();
    void    slotScaleEditor();
    /// 时间线游标移动:刷新属性面板轨道勾选态
    void    slotPropsCurTimeChanged(double curTime);
    /// 重置 Dock 布局
    void    slotResetLayout();
private:
    /// <summary>
    /// 禁用动画系统中所有动画(标记为不可播放/不可编辑)
    /// </summary>
    void    disableAllAnimations();
    /// 重挂父节点(模型树拖拽嵌入/顶层/跨父前/后),newParent 为空表示提升到顶层
    bool    ctxReparentNode(FE::FENode* node, FE::FENode* newParent);
    /// 同父兄弟排序(模型树拖拽前/后插入)
    void    ctxReorderNode(FE::FENode* node, FE::FENode* target, bool after);
    /// 复制节点到剪贴板(存源引用,setCutNode 清状态)
    void    ctxCopyNode(FE::FENode* node);
    /// 剪切节点到剪贴板(存源引用,setCutNode 半透明绘制)
    void    ctxCutNode(FE::FENode* node);
    /// 粘贴: 剪切=重挂当前选中下(移动),复制=克隆为源父兄弟(PasteNodeCmd)
    void    ctxPasteNode(FE::FENode* node);
protected:
    /// <summary>
    /// 引擎启动后通知，函数内可以对依赖引擎的组件模块进行初始化
    /// </summary>
    /// <param name="scene"></param>
    void    notifyEngineStart(FEScene& scene);

protected:
    virtual void    closeEvent(QCloseEvent *event)  override ;
    
protected:
    QString         _projectName;
    QUndoStack*     _undoStack;
    Ui::MainWindow  ui;
    /// 移动编辑器拖拽期间暂存的旧位置 (EditStart 快照,EditEnd 时消费)
    std::vector<FE::MoveNodeCmd::ObjectMove>  _moveSnapshots;
    /// 属性面板(嵌入 widget_properties 占位容器)
    UiProperties*   _propsUi    =   nullptr;
    /// 剪贴板: 复制/剪切保存的源节点引用(保活),粘贴时据此克隆或移动
    FE::Node        _clipNode;
    /// 剪贴板模式: true=剪切(粘贴后清空),false=复制(可多次粘贴)
    bool            _clipCut    =   false;
};