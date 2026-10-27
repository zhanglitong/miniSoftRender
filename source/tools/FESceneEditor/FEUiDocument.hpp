/// <summary>
/// FEUiDocument
/// 解析 Qt Designer 的 .ui 文件(rapidxml),生成与 Qt 无关的控件树,
/// 供 FEUiRenderer 用 ImGui 绘制。
/// 支持范围:
///   - 容器:QMainWindow / QWidget / QWidget(提升控件) / QSplitter / QStatusBar
///   - 布局:QHBoxLayout / QVBoxLayout / QGridLayout
///   - 控件:QPushButton / QToolButton / QCheckBox / QComboBox / QLineEdit /
///          QLabel / QScrollBar / QGroupBox
///   - 未识别的类按"自定义提升控件"处理,由渲染层占位并交给注册的回调。
/// </summary>

#pragma     once

#include    <string>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 屏幕矩形
    /// </summary>
    struct  FEUiRect
    {
        float       _x  =   0.0f;
        float       _y  =   0.0f;
        float       _w  =   0.0f;
        float       _h  =   0.0f;
    };

    class   FEUiWidget;
    class   FEUiLayout;

    /// <summary>
    /// 布局中的一项(控件或子布局),携带网格位置
    /// </summary>
    struct  FEUiItem
    {
        int         _row        =   0;
        int         _col        =   0;
        int         _rowSpan    =   1;
        int         _colSpan    =   1;
        FEUiWidget* _widget     =   nullptr;
        FEUiLayout* _layout     =   nullptr;
    };

    /// <summary>
    /// 一个布局节点(QHBoxLayout / QVBoxLayout / QGridLayout)
    /// </summary>
    class   FEUiLayout
    {
    public:
        ~FEUiLayout();
        FEUiItem&   addItem();
    public:
        /// <summary>
        /// QHBoxLayout / QVBoxLayout / QGridLayout
        /// </summary>
        std::string             _class;
        std::vector<FEUiItem>   _items;
        /// <summary>
        /// QGridLayout 的 rowstretch
        /// </summary>
        std::vector<int>        _rowStretch;
        /// <summary>
        /// QGridLayout 的 colstretch
        /// </summary>
        std::vector<int>        _colStretch;
        float                   _spacing    =   0.0f;
    };

    /// <summary>
    /// 一个控件节点
    /// </summary>
    class   FEUiWidget
    {
    public:
        ~FEUiWidget();
        /// <summary>
        /// QComboBox 的当前选项文本
        /// </summary>
        std::string     currentOption() const;
    public:
        std::string                 _class;
        std::string                 _name;
        std::string                 _text;
        /// <summary>
        /// QComboBox 的 item 文本
        /// </summary>
        std::vector<std::string>    _options;
        bool                        _enabled     =   true;
        bool                        _checked     =   false;
        int                         _currentIndex=   0;
        /// <summary>
        /// 布局权重(来自 sizePolicy 的 horstretch/verstretch,0 表示固定尺寸)
        /// </summary>
        int                         _hStretch    =   0;
        int                         _vStretch    =   0;
        /// <summary>
        /// 顶层窗口的 geometry
        /// </summary>
        FEUiRect                    _geometry;
        FEUiLayout*                 _layout      =   nullptr;
        std::vector<FEUiWidget*>    _children;
        /// <summary>
        /// QLineEdit 的编辑缓冲
        /// </summary>
        char                        _buffer[256]=   {   0   };
        /// <summary>
        /// QSplitter 的分割比例
        /// </summary>
        float                       _split       =   0.5f;
    };

    /// <summary>
    /// 解析后的 .ui 文档
    /// </summary>
    class   FEUiDocument
    {
    public:
        ~FEUiDocument();
        /// <summary>
        /// 解析 .ui 文件;失败时 error() 给出原因,root() 为 nullptr
        /// </summary>
        /// <param name="file">.ui 文件路径</param>
        /// <returns>true 解析成功</returns>
        bool            load(const std::string& file);
        void            clear();
        FEUiWidget*     root()  const   {   return  _root;  }
        const   std::string&    error() const   {   return  _error;    }
        /// <summary>
        /// 按控件名递归查找
        /// </summary>
        /// <param name="name">控件名</param>
        /// <returns>找到的控件;不存在返回 nullptr</returns>
        FEUiWidget*     find(const std::string& name) const;
        /// <summary>
        /// 按控件类名递归查找
        /// </summary>
        /// <param name="cls">类名</param>
        /// <returns>找到的控件;不存在返回 nullptr</returns>
        FEUiWidget*     findClass(const std::string& cls) const;
    protected:
        void*           parseWidget(void* node);
        void*           parseLayout(void* node);
        void            parseProperty(FEUiWidget* widget,void* property);
    protected:
        FEUiWidget*             _root   =   nullptr;
        std::string             _error;
        /// <summary>
        /// rapidxml 就地解析缓冲区
        /// </summary>
        std::vector<char>       _buffer;
    };
}
