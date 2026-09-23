#pragma     once

#include    <QtWidgets/QMainWindow>
#include    <QUndoStack>
#include    <QShortcut>
#include    <QMessageBox>
#include    <string>
#include    <map>
#include    "ui_MainWindow.h"

class   MainWindow;
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
private:
    /// <summary>
    /// 禁用动画系统中所有动画(标记为不可播放/不可编辑)
    /// </summary>
    void    disableAllAnimations();
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
};