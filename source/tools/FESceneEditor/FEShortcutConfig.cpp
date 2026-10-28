/// <summary>
/// FEShortcutConfig 实现:命令收集、默认绑定、VK 键名互转与设置读写。
/// 键位统一使用 Windows 虚拟键码,与 ImGuiIO::KeysDown 的下标一致。
/// </summary>

#include    "FEShortcutConfig.hpp"

#include    "FEPath.hpp"

#include    <windows.h>

#include    <algorithm>
#include    <cctype>
#include    <cstdio>
#include    <filesystem>
#include    <string>
#include    <vector>

namespace
{
    /// <summary>
    /// 名称 → 虚拟键码的别名表(与 keyName 的输出保持一致)
    /// </summary>
    struct  KeyAlias
    {
        /// <summary>
        /// 显示名称
        /// </summary>
        const   char*   _name;
        /// <summary>
        /// 虚拟键码
        /// </summary>
        int             _key;
    };

    const   KeyAlias    kKeyAliases[]  =
    {
        { "Backspace",      0x08 },
        { "Tab",            0x09 },
        { "Enter",          0x0D },
        { "Escape",         0x1B },
        { "Space",          0x20 },
        { "PageUp",         0x21 },
        { "PageDown",       0x22 },
        { "End",            0x23 },
        { "Home",           0x24 },
        { "Left",           0x25 },
        { "Up",             0x26 },
        { "Right",          0x27 },
        { "Down",           0x28 },
        { "Insert",         0x2D },
        { "Delete",         0x2E },
        { "PrintScreen",    0x2C },
        { ";",              0xBA },
        { "=",              0xBB },
        { ",",              0xBC },
        { "-",              0xBD },
        { ".",              0xBE },
        { "/",              0xBF },
        { "`",              0xC0 },
        { "[",              0xDB },
        { "\\",             0xDC },
        { "]",              0xDD },
        { "'",              0xDE },
    };

    /// <summary>
    /// 内置默认绑定:仅对 .ui 里没写 shortcut 的常用命令生效
    /// </summary>
    struct  BuiltinDefault
    {
        /// <summary>
        /// 动作名
        /// </summary>
        const   char*   _id;
        /// <summary>
        /// 组合文本,例如 Ctrl+Shift+O
        /// </summary>
        const   char*   _combo;
    };

    const   BuiltinDefault  kBuiltinDefaults[]  =
    {
        { "action_openPrj",     "Ctrl+O"        },
        { "action_savePrj",     "Ctrl+S"        },
        { "action_undo",        "Ctrl+Z"        },
        { "action_redo",        "Ctrl+Y"        },
        { "action_ShortcutSettings","Ctrl+K"    },
    };

    /// <summary>
    /// 字符串转大写(用于不区分大小写的键名比较)
    /// </summary>
    /// <param name="text">原始文本</param>
    /// <returns>大写文本</returns>
    std::string upper(const std::string& text)
    {
        std::string result  =   text;
        for (char& c : result)
            c   =   char(std::toupper(static_cast<unsigned char>(c)));
        return  result;
    }

    /// <summary>
    /// 递归收集某一页下的动作(页 → 分组 → 动作,分组可嵌套)
    /// </summary>
    /// <param name="node">当前节点</param>
    /// <param name="group">所属 Ribbon 页名</param>
    /// <param name="allow">动作过滤器</param>
    /// <param name="items">输出绑定表</param>
    /// <param name="defaults">输出默认绑定表</param>
    void    collectActions(const FE::FEUiMenuItem* node,const std::string& group,
                           bool (*allow)(const std::string&),
                           std::vector<FE::FEShortcutItem>&  items,
                           std::vector<FE::FEShortcutConfig::DefaultBind>& defaults)
    {
        for (const FE::FEUiMenuItem* child : node->_children)
        {
            if (child == nullptr)
                continue;
            if (child->_type == FE::kUiMenu)
            {
                /// 子菜单仍是同一页内的分组,页名沿用父级
                collectActions(child,group,allow,items,defaults);
                continue;
            }
            if (child->_type != FE::kUiAction)
                continue;
            if (child->_name.empty())
                continue;
            if (allow != nullptr && !allow(child->_name))
                continue;

            /// 同名动作只登记一次(菜单栏与工具栏可能引用同一个 QAction)
            const   bool    duplicated  =   std::any_of(items.begin(),items.end(),
                [&child](const FE::FEShortcutItem& item) {   return  item._id == child->_name;    });
            if (duplicated)
                continue;

            FE::FEShortcutItem  item;
            item._id    =   child->_name;
            item._text  =   child->_text;
            item._group =   group;
            items.push_back(item);

            /// 默认绑定:.ui 里写了 shortcut 就用它,否则查内置表
            FE::FEShortcutConfig::DefaultBind  def;
            if (!child->_shortcut.empty())
                FE::FEShortcutConfig::parseCombo(child->_shortcut,def._key,
                                                 def._ctrl,def._shift,def._alt);
            if (def._key == 0)
            {
                for (const BuiltinDefault& builtin : kBuiltinDefaults)
                {
                    if (child->_name == builtin._id)
                    {
                        FE::FEShortcutConfig::parseCombo(builtin._combo,def._key,
                                                         def._ctrl,def._shift,def._alt);
                        break;
                    }
                }
            }
            defaults.push_back(def);
        }
    }

    /// <summary>
    /// 设置树中的快捷键节点名:动作名作为子节点名,组合文本作为 string 值
    /// </summary>
    const   char    kSettingName[]  =   "shortcut";
}

namespace   FE
{
    void    FEShortcutConfig::build(const FEUiDocument& doc,bool (*allow)(const std::string&))
    {
        _items.clear();
        _defaults.clear();

        const   FEUiMenuItem*   bar =   doc.menuBar();
        if (bar == nullptr)
            return;

        _items.reserve(bar->_children.size());
        for (const FEUiMenuItem* page : bar->_children)
        {
            /// 菜单栏的子节点即 Ribbon 的 Page,其标题作为动作的分组名
            if (page == nullptr || page->_type != kUiMenu)
                continue;
            collectActions(page,page->_text,allow,_items,_defaults);
        }

        /// 默认值与条目一一对应
        _defaults.resize(_items.size());
        resetAll();
    }

    bool    FEShortcutConfig::load(const FESetting& setting)
    {
        const   FESetting::Item*    root    =   setting.find(kSettingName);
        if (root == nullptr)
            return  false;

        /// 节点名即动作名,值即组合文本(空文本表示未绑定)
        for (const auto& node : root->children())
        {
            auto    it  =   std::find_if(_items.begin(),_items.end(),
                [&node](const FEShortcutItem& item) {    return  item._id == node->name();  });
            if (it == _items.end())
                continue;

            const   std::string*    text    =   node->value<std::string>();
            if (text == nullptr)
                continue;
            parseCombo(*text,it->_key,it->_ctrl,it->_shift,it->_alt);
        }
        return  true;
    }

    void    FEShortcutConfig::save(FESetting& setting) const
    {
        FESetting::Item&    root    =   setting.get(kSettingName);
        for (const FEShortcutItem& item : _items)
        {
            root.get(item._id.c_str()).set(comboText(item._key,item._ctrl,
                                                      item._shift,item._alt));
        }
    }

    void    FEShortcutConfig::reset(size_t index)
    {
        if (index >= _items.size() || index >= _defaults.size())
            return;
        const   DefaultBind&    def =   _defaults[index];
        _items[index]._key   =   def._key;
        _items[index]._ctrl  =   def._ctrl;
        _items[index]._shift =   def._shift;
        _items[index]._alt   =   def._alt;
    }

    void    FEShortcutConfig::resetAll()
    {
        for (size_t i = 0; i < _items.size(); ++i)
            reset(i);
    }

    int     FEShortcutConfig::conflict(size_t index) const
    {
        if (index >= _items.size())
            return  -1;
        const   FEShortcutItem& item =   _items[index];
        return  conflictWith(item._key,item._ctrl,item._shift,item._alt,int(index));
    }

    int     FEShortcutConfig::conflictWith(int key,bool ctrl,bool shift,bool alt,int skip) const
    {
        if (key == 0)
            return  -1;
        for (size_t i = 0; i < _items.size(); ++i)
        {
            if (int(i) == skip)
                continue;
            const   FEShortcutItem& item =   _items[i];
            if (item._key == 0)
                continue;
            if (item._key == key && item._ctrl == ctrl && item._shift == shift && item._alt == alt)
                return  int(i);
        }
        return  -1;
    }

    std::string FEShortcutConfig::text(size_t index) const
    {
        if (index >= _items.size())
            return  std::string();
        const   FEShortcutItem& item =   _items[index];
        return  comboText(item._key,item._ctrl,item._shift,item._alt);
    }

    void    FEShortcutConfig::assign(size_t index,int key,bool ctrl,bool shift,bool alt)
    {
        if (index >= _items.size())
            return;
        _items[index]._key   =   key;
        _items[index]._ctrl  =   ctrl;
        _items[index]._shift =   shift;
        _items[index]._alt   =   alt;
    }

    std::string FEShortcutConfig::keyName(int vk)
    {
        if (vk == 0)
            return  std::string();
        if (vk >= 'A' && vk <= 'Z')
            return  std::string(1,char(vk));
        if (vk >= '0' && vk <= '9')
            return  std::string(1,char(vk));
        if (vk >= VK_F1 && vk <= VK_F24)
            return  std::string("F") + std::to_string(vk - VK_F1 + 1);
        if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
            return  std::string("Num") + std::to_string(vk - VK_NUMPAD0);
        for (const KeyAlias& alias : kKeyAliases)
        {
            if (alias._key == vk)
                return  alias._name;
        }
        return  std::string("VK") + std::to_string(vk);
    }

    int     FEShortcutConfig::keyFromName(const std::string& name)
    {
        const   std::string text    =   upper(name);
        if (text.empty())
            return  0;
        /// 修饰键不作为主键
        if (text == "CTRL" || text == "CONTROL" || text == "SHIFT" || text == "ALT")
            return  0;
        if (text.size() == 1)
        {
            const   char    c   =   text[0];
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                return  c;
        }
        if (text[0] == 'F')
        {
            const   int     n   =   std::atoi(text.c_str() + 1);
            if (n >= 1 && n <= 24 && text.size() <= 3)
                return  VK_F1 + n - 1;
        }
        if (text.rfind("NUM",0) == 0)
        {
            const   int     n   =   std::atoi(text.c_str() + 3);
            if (n >= 0 && n <= 9 && text.size() <= 4)
                return  VK_NUMPAD0 + n;
        }
        for (const KeyAlias& alias : kKeyAliases)
        {
            if (upper(alias._name) == text)
                return  alias._key;
        }
        if (text.rfind("VK",0) == 0)
        {
            const   int     n   =   std::atoi(text.c_str() + 2);
            if (n > 0 && n < 256)
                return  n;
        }
        return  0;
    }

    std::string FEShortcutConfig::comboText(int vk,bool ctrl,bool shift,bool alt)
    {
        if (vk == 0)
            return  std::string();
        std::string result;
        if (ctrl)
            result += "Ctrl+";
        if (shift)
            result += "Shift+";
        if (alt)
            result += "Alt+";
        result += keyName(vk);
        return  result;
    }

    void    FEShortcutConfig::parseCombo(const std::string& text,int& vk,
                                         bool& ctrl,bool& shift,bool& alt)
    {
        vk      =   0;
        ctrl    =   false;
        shift   =   false;
        alt     =   false;

        size_t  begin   =   0;
        while (begin <= text.size())
        {
            size_t          end =   text.find('+',begin);
            if (end == std::string::npos)
                end =   text.size();
            const   std::string token   =   text.substr(begin,end - begin);
            if (!token.empty())
            {
                const   std::string key =   upper(token);
                if (key == "CTRL" || key == "CONTROL")
                    ctrl    =   true;
                else if (key == "SHIFT")
                    shift   =   true;
                else if (key == "ALT")
                    alt     =   true;
                else if (vk == 0)
                    vk  =   keyFromName(token);
            }
            if (end == text.size())
                break;
            begin   =   end + 1;
        }
    }

    std::string FEShortcutConfig::exeXmlPath()
    {
        /// exe 路径由平台层提供;拿不到时退回到当前目录下的同名文件
        std::filesystem::path   path    =   exePath();
        if (path.empty())
            return  std::string("FESceneEditor.xml");
        path.replace_extension(".xml");
        return  path.string();
    }
}
