#pragma     once

#include    <filesystem>
#include    <string>
#include    <vector>

struct  ImFont;

namespace   FE
{
    /// <summary>
    /// 自绘的文件选择对话框(ImGui 窗口,不依赖平台 API):
    ///   驱动/上级/常用位置导航 + 目录列表 + 当前目录文件列表 + 文件名输入 + 文件类型过滤。
    /// 用法:open() 置起标志,之后每帧 draw(),返回 true 时对话框结束。
    /// </summary>
    class   FEFileDialog
    {
    public:
        /// <summary>
        /// 对话框模式
        /// </summary>
        enum    Mode
        {
            kNone   =   0,
            /// <summary>
            /// 打开文件
            /// </summary>
            kOpen   =   1,
            /// <summary>
            /// 另存为
            /// </summary>
            kSave   =   2,
        };
        /// <summary>
        /// 文件类型过滤器:下拉框中的一项
        /// </summary>
        struct  Filter
        {
            /// <summary>
            /// 显示文本,例如 "3D 模型 (*.gltf;*.glb)"
            /// </summary>
            std::string     _label;
            /// <summary>
            /// 分号分隔的扩展名,为空表示显示全部文件
            /// </summary>
            std::string     _ext;
        };
    public:
        /// <summary>
        /// 弹出对话框
        /// </summary>
        /// <param name="mode">模式(打开/另存为)</param>
        /// <param name="title">窗口标题</param>
        /// <param name="filters">文件类型过滤器列表;第一项为默认,多于一项时显示类型下拉框</param>
        /// <param name="dir">初始目录;为空时用当前工作目录</param>
        /// <param name="defaultName">另存为模式下的默认文件名</param>
        void    open(Mode mode,const std::string& title,const std::vector<Filter>& filters,
                     const std::string& dir,const std::string& defaultName);
        /// <summary>
        /// 关闭对话框(不产出结果)
        /// </summary>
        void    close();
        /// <summary>
        /// 每帧绘制;返回 true 表示本次调用结束了对话框
        /// </summary>
        /// <param name="iconFont">图标字体,可为 nullptr</param>
        /// <param name="resultPath">确认时写入完整路径(UTF-8);取消时不变</param>
        /// <returns>true 表示对话框已关闭</returns>
        bool    draw(ImFont* iconFont,std::string& resultPath);
        /// <summary>
        /// 对话框是否处于打开状态
        /// </summary>
        bool    isOpen() const  {   return  _mode != kNone;   }
    protected:
        /// <summary>
        /// 列表项:一个文件或一个目录
        /// </summary>
        struct  Entry
        {
            std::string     _name;
            bool            _dir    =   false;
        };
        /// <summary>
        /// 左侧导航栏的快捷跳转项
        /// </summary>
        struct  QuickItem
        {
            /// <summary>
            /// 显示文本,例如 桌面 / 文档 / C:
            /// </summary>
            std::string             _label;
            std::filesystem::path   _path;
        };
        /// <summary>
        /// 重新扫描当前目录下的子目录与匹配文件
        /// </summary>
        void    refresh();
        /// <summary>
        /// 过滤扩展名中的第一个(另存为时补全用);_ext 支持以分号分隔多个扩展
        /// </summary>
        /// <returns>带点的扩展名,例如 .fepj;_ext 为空时返回空串</returns>
        std::string firstExt() const;
        /// <summary>
        /// 收集左侧导航栏的常用目录(主目录/桌面/下载/文档/程序目录/各盘符),
        /// 仅收录实际存在的目录
        /// </summary>
        /// <param name="startDir">初始目录(UTF-8),作为"程序目录"快捷项</param>
        void    buildQuickItems(const std::string& startDir);
        /// <summary>
        /// 切换到指定目录(规范化后扫描)
        /// </summary>
        /// <param name="dir">目标目录(UTF-8)</param>
        void    enter(const std::filesystem::path& dir);
        /// <summary>
        /// 地址栏跳转:可输入目录、盘符根(如 D:\)或文件路径。
        /// 输入文件时跳到其所在目录并填入文件名;路径无效时保留原位并提示
        /// </summary>
        /// <param name="text">用户输入的路径</param>
        void    gotoPath(const std::string& text);
        /// <summary>
        /// 把指定路径写入地址栏输入缓冲
        /// </summary>
        /// <param name="utf8Path">UTF-8 路径</param>
        void    setPathBuf(const std::string& utf8Path);
    protected:
        Mode        _mode   =   kNone;
        std::string _title;
        /// <summary>
        /// 文件类型过滤器列表(第一项为默认)
        /// </summary>
        std::vector<Filter> _filters;
        /// <summary>
        /// 当前选中的过滤器下标
        /// </summary>
        int         _filterIndex    =   0;
        /// <summary>
        /// 当前生效的扩展名过滤,例如 .fepj(取自 _filters[_filterIndex])
        /// </summary>
        std::string _ext;
        /// <summary>
        /// 当前目录
        /// </summary>
        std::filesystem::path   _dir;
        /// <summary>
        /// 文件名输入缓冲
        /// </summary>
        char        _nameBuf[260]   =   {   0   };
        /// <summary>
        /// 地址栏输入缓冲(当前目录;可直接编辑以跳转,含盘符根)
        /// </summary>
        char        _pathBuf[1024]  =   {   0   };
        std::vector<Entry>      _entries;
        /// <summary>
        /// 左侧导航栏的快捷跳转项(弹出对话框时构建一次)
        /// </summary>
        std::vector<QuickItem>  _quickItems;
        /// <summary>
        /// 需要重新扫描(切换目录后置位)
        /// </summary>
        bool        _dirty  =   true;
        /// <summary>
        /// 上一帧的选中项(用于双击判定)
        /// </summary>
        int         _selected   =   -1;
        /// <summary>
        /// 状态提示(无法进入目录等)
        /// </summary>
        std::string _notice;
        /// <summary>
        /// 首次绘制时需要 SetNextWindowSize
        /// </summary>
        bool        _firstFrame =   true;
    };
}
