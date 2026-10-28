/// <summary>
/// FEShortcutConfig
/// 编辑器快捷键表:命令来自 .ui 的 QAction,绑定用 Windows 虚拟键码表示
/// (与 ImGuiIO::KeysDown 的下标一致),绑定读写到 FEContext 的设置树(FESetting),
/// 由设置统一保存到 exe 同目录,例如 bin/WindowsDebugx64/FESceneEditor.xml。
/// </summary>

#pragma     once

#include    "FEUiDocument.hpp"
#include    "FESetting.hpp"

#include    <string>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 一条快捷键绑定
    /// </summary>
    struct  FEShortcutItem
    {
        /// <summary>
        /// .ui 中的 QAction 名,例如 action_openPrj
        /// </summary>
        std::string     _id;
        /// <summary>
        /// 动作显示名,例如 打开工程
        /// </summary>
        std::string     _text;
        /// <summary>
        /// 所属 Ribbon 页名(文件/编辑/创建对象/设置),仅用于排序
        /// </summary>
        std::string     _group;
        /// <summary>
        /// 虚拟键码;0 表示未绑定
        /// </summary>
        int             _key    =   0;
        bool            _ctrl   =   false;
        bool            _shift  =   false;
        bool            _alt    =   false;
    };

    /// <summary>
    /// 快捷键表:建立命令列表、应用默认值、读写设置
    /// </summary>
    class   FEShortcutConfig
    {
    public:
        /// <summary>
        /// 默认绑定(与 _items 一一对应)
        /// </summary>
        struct  DefaultBind
        {
            int     _key    =   0;
            bool    _ctrl   =   false;
            bool    _shift  =   false;
            bool    _alt    =   false;
        };
    public:
        /// <summary>
        /// 从 .ui 菜单栏收集动作并生成绑定表(按页/分组顺序,同名动作只保留一次)。
        /// 默认绑定优先取 .ui 里 QAction 的 shortcut 文本,其次取内置默认(Ctrl+Z 等)
        /// </summary>
        /// <param name="doc">已解析的 .ui 文档</param>
        /// <param name="allow">过滤器:返回 true 的动作才纳入配置页;传 nullptr 纳入全部动作</param>
        void    build(const FEUiDocument& doc,bool (*allow)(const std::string&));
        /// <summary>
        /// 从设置树读取绑定;快捷键节点不存在返回 false(保留默认值)
        /// </summary>
        /// <param name="setting">上下文设置</param>
        /// <returns>是否找到快捷键节点</returns>
        bool    load(const FESetting& setting);
        /// <summary>
        /// 把全部绑定写入设置树(动作名作为节点名,组合文本作为 string 值)
        /// </summary>
        /// <param name="setting">上下文设置</param>
        void    save(FESetting& setting) const;
        /// <summary>
        /// 恢复某条为默认绑定
        /// </summary>
        /// <param name="index">条目下标</param>
        void    reset(size_t index);
        /// <summary>
        /// 恢复全部为默认绑定
        /// </summary>
        void    resetAll();
        /// <summary>
        /// 与同一组合冲突的另一条下标;-1 表示无冲突
        /// </summary>
        /// <param name="index">条目下标</param>
        int     conflict(size_t index) const;
        /// <summary>
        /// 候选组合是否已被其他条目占用;返回占用者下标,-1 表示无冲突
        /// </summary>
        /// <param name="key">虚拟键码</param>
        /// <param name="ctrl">Ctrl</param>
        /// <param name="shift">Shift</param>
        /// <param name="alt">Alt</param>
        /// <param name="skip">跳过检查的条目下标(录制中的那一行)</param>
        int     conflictWith(int key,bool ctrl,bool shift,bool alt,int skip) const;
        /// <summary>
        /// 组合文本,例如 Ctrl+Shift+O;未绑定时为空
        /// </summary>
        /// <param name="index">条目下标</param>
        std::string text(size_t index) const;
        /// <summary>
        /// 把组合追加到条目(_key 为 0 表示解绑)
        /// </summary>
        void    assign(size_t index,int key,bool ctrl,bool shift,bool alt);
        /// <summary>
        /// 虚拟键码 → 名称(A/Z、0/9、F1、Left 等)
        /// </summary>
        static  std::string keyName(int vk);
        /// <summary>
        /// 名称 → 虚拟键码;认不出返回 0
        /// </summary>
        static  int     keyFromName(const std::string& name);
        /// <summary>
        /// 组合 → 文本,顺序固定为 Ctrl+Shift+Alt+键
        /// </summary>
        static  std::string comboText(int vk,bool ctrl,bool shift,bool alt);
        /// <summary>
        /// 文本 → 组合,例如 "Ctrl+Shift+O";解析失败时 vk 为 0
        /// </summary>
        static  void    parseCombo(const std::string& text,int& vk,
                                   bool& ctrl,bool& shift,bool& alt);
        /// <summary>
        /// 设置文件路径(与 exe 同名同目录,例如 FESceneEditor.xml);
        /// exe 路径取自平台层 FE::exePath(),取不到时回退到当前目录
        /// </summary>
        static  std::string exeXmlPath();
    public:
        std::vector<FEShortcutItem>     _items;
    protected:
        std::vector<DefaultBind>        _defaults;
    };
}
