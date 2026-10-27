/// <summary>
/// FEUiDocument 实现:rapidxml 解析 .ui 为控件树。
/// </summary>

#include    "FEUiDocument.hpp"

#include    "../../../inc/xml/rapidxml.hpp"

#include    <cstdio>
#include    <cstdlib>
#include    <cstring>
#include    <map>

namespace
{
    /// <summary>
    /// 读取元素属性
    /// </summary>
    /// <param name="node">XML 节点</param>
    /// <param name="name">属性名</param>
    /// <returns>属性值;不存在返回 nullptr</returns>
    const   char*   attr(void* node,const char* name)
    {
        auto*   n   =   static_cast<rapidxml::xml_node<>*>(node);
        auto*   a   =   n->first_attribute(name);
        return  (a != nullptr) ? a->value() : nullptr;
    }

    /// <summary>
    /// 读取第一个指定名字的子元素
    /// </summary>
    /// <param name="node">XML 节点</param>
    /// <param name="name">子元素名</param>
    /// <returns>子元素;不存在返回 nullptr</returns>
    void*           childNode(void* node,const char* name)
    {
        auto*   n   =   static_cast<rapidxml::xml_node<>*>(node);
        return  n->first_node(name);
    }

    /// <summary>
    /// 读取子元素的文本
    /// </summary>
    /// <param name="node">XML 节点</param>
    /// <param name="name">子元素名</param>
    /// <returns>文本;不存在返回 nullptr</returns>
    const   char*   childText(void* node,const char* name)
    {
        if (node == nullptr)
            return  nullptr;
        auto*   n   =   static_cast<rapidxml::xml_node<>*>(node);
        auto*   c   =   n->first_node(name);
        return  (c != nullptr) ? c->value() : nullptr;
    }

    /// <summary>
    /// 读取子元素的数值
    /// </summary>
    /// <param name="node">XML 节点</param>
    /// <param name="name">子元素名</param>
    /// <param name="defValue">缺失时的默认值</param>
    /// <returns>解析出的整数</returns>
    int             childInt(void* node,const char* name,int defValue = 0)
    {
        const   char*   text    =   childText(node,name);
        return  (text != nullptr) ? std::atoi(text) : defValue;
    }

    /// <summary>
    /// 解析 Qt 常用实体(&lt; &gt; &amp; &quot; &#xNN;)
    /// </summary>
    /// <param name="text">原始文本</param>
    /// <returns>解码后的文本</returns>
    std::string     decodeEntities(const char* text)
    {
        std::string     result;
        if (text == nullptr)
            return  result;
        result.reserve(std::strlen(text));
        for (const char* p = text; *p != '\0'; ++p)
        {
            if (*p != '&')
            {
                result.push_back(*p);
                continue;
            }
            const   char*   end =   std::strchr(p,';');
            if (end == nullptr)
            {
                result.push_back(*p);
                continue;
            }
            const   std::string     entity(p + 1,size_t(end - p - 1));
            if (entity == "lt")
                result.push_back('<');
            else if (entity == "gt")
                result.push_back('>');
            else if (entity == "amp")
                result.push_back('&');
            else if (entity == "quot")
                result.push_back('"');
            else if (entity == "apos")
                result.push_back('\'');
            else if (!entity.empty() && entity[0] == '#')
            {
                const   int     code    =   (entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X'))
                                            ? int(std::strtol(entity.c_str() + 2,nullptr,16))
                                            : std::atoi(entity.c_str() + 1);
                if (code > 0 && code < 0x80)
                    result.push_back(char(code));
            }
            else
            {
                result.append(p,size_t(end - p + 1));
            }
            p   =   end;
        }
        return  result;
    }

    /// <summary>
    /// 逗号分隔的整数串("0,0" / "3")
    /// </summary>
    /// <param name="text">原始文本</param>
    /// <returns>整数列表</returns>
    std::vector<int>    parseIntList(const char* text)
    {
        std::vector<int>    result;
        if (text == nullptr)
            return  result;
        const   char*   p   =   text;
        while (*p != '\0')
        {
            result.push_back(std::atoi(p));
            const   char*   comma   =   std::strchr(p,',');
            if (comma == nullptr)
                break;
            p   =   comma + 1;
        }
        return  result;
    }

    /// <summary>
    /// 菜单名 -> XML 节点,用于解析 addaction 引用
    /// </summary>
    using   MenuNodeMap     =   std::map<std::string,void*>;

    /// <summary>
    /// 收集 QMainWindow 下所有 QMenu 定义(按 name 索引)
    /// </summary>
    /// <param name="node">起始 XML 节点</param>
    /// <param name="menus">输出的菜单名索引</param>
    void    collectMenus(void* node,MenuNodeMap& menus)
    {
        for (void* w = childNode(node,"widget"); w != nullptr;
             w = static_cast<rapidxml::xml_node<>*>(w)->next_sibling("widget"))
        {
            const   char*   cls     =   attr(w,"class");
            const   char*   name    =   attr(w,"name");
            if (cls != nullptr && name != nullptr && std::strcmp(cls,"QMenu") == 0)
                menus[name] =   w;
            collectMenus(w,menus);
        }
    }

    /// <summary>
    /// 读取 QMenu 的 title 属性
    /// </summary>
    /// <param name="node">QMenu 的 XML 节点</param>
    /// <returns>菜单标题</returns>
    std::string     menuTitle(void* node)
    {
        for (void* p = childNode(node,"property"); p != nullptr;
             p = static_cast<rapidxml::xml_node<>*>(p)->next_sibling("property"))
        {
            const   char*   name    =   attr(p,"name");
            if (name == nullptr || std::strcmp(name,"title") != 0)
                continue;
            const   char*   text    =   childText(p,"string");
            if (text != nullptr)
                return  decodeEntities(text);
        }
        return  std::string();
    }

    /// <summary>
    /// 读取 QAction 的 text / icon / shortcut / checkable / checked / enabled
    /// </summary>
    /// <param name="item">目标菜单项</param>
    /// <param name="actionNode">&lt;action&gt; 的 XML 节点</param>
    void    parseActionItem(FE::FEUiMenuItem* item,void* actionNode)
    {
        for (void* p = childNode(actionNode,"property"); p != nullptr;
             p = static_cast<rapidxml::xml_node<>*>(p)->next_sibling("property"))
        {
            const   char*   name    =   attr(p,"name");
            if (name == nullptr)
                continue;
            const   char*   text    =   nullptr;
            if (std::strcmp(name,"text") == 0)
            {
                const   char*   value   =   childText(p,"string");
                if (value != nullptr)
                    item->_text =   decodeEntities(value);
            }
            else if (std::strcmp(name,"shortcut") == 0)
            {
                text    =   childText(p,"string");
                if (text != nullptr)
                    item->_shortcut =   decodeEntities(text);
            }
            else if (std::strcmp(name,"icon") == 0)
            {
                /// <iconset><normaloff>:/EditorRes/res/xxx.png</normaloff></iconset>
                void*   iconset =   childNode(p,"iconset");
                const   char*   icon    =   childText(iconset,"normaloff");
                if (icon != nullptr)
                    item->_icon =   decodeEntities(icon);
            }
            else if (std::strcmp(name,"checkable") == 0)
            {
                text    =   childText(p,"bool");
                item->_checkable    =   (text != nullptr) && (std::strcmp(text,"true") == 0);
            }
            else if (std::strcmp(name,"checked") == 0)
            {
                text    =   childText(p,"bool");
                item->_checked  =   (text != nullptr) && (std::strcmp(text,"true") == 0);
            }
            else if (std::strcmp(name,"enabled") == 0)
            {
                text    =   childText(p,"bool");
                item->_enabled  =   (text == nullptr) || (std::strcmp(text,"false") != 0);
            }
        }
    }

    /// <summary>
    /// 把 QMenuBar / QMenu 递归解析成菜单树(Page / 分组 / 按钮)
    /// </summary>
    /// <param name="node">QMenuBar 或 QMenu 的 XML 节点</param>
    /// <param name="actions">动作名索引</param>
    /// <param name="menus">菜单名索引</param>
    /// <returns>新分配的菜单节点</returns>
    FE::FEUiMenuItem*   parseMenuItem(void* node,const MenuNodeMap& actions,const MenuNodeMap& menus)
    {
        FE::FEUiMenuItem*   item    =   new FE::FEUiMenuItem();
        item->_type =   FE::kUiMenu;
        const   char*   name    =   attr(node,"name");
        item->_name =   (name != nullptr) ? name : std::string();
        item->_text =   menuTitle(node);

        for (void* a = childNode(node,"addaction"); a != nullptr;
             a = static_cast<rapidxml::xml_node<>*>(a)->next_sibling("addaction"))
        {
            const   char*   ref =   attr(a,"name");
            if (ref == nullptr)
                continue;
            if (std::strcmp(ref,"separator") == 0)
            {
                FE::FEUiMenuItem*   separator   =   new FE::FEUiMenuItem();
                separator->_type    =   FE::kUiSeparator;
                item->_children.push_back(separator);
                continue;
            }
            auto    itAction    =   actions.find(ref);
            if (itAction != actions.end())
            {
                FE::FEUiMenuItem*   action  =   new FE::FEUiMenuItem();
                action->_type   =   FE::kUiAction;
                action->_name   =   ref;
                parseActionItem(action,itAction->second);
                item->_children.push_back(action);
                continue;
            }
            auto    itMenu  =   menus.find(ref);
            if (itMenu != menus.end())
                item->_children.push_back(parseMenuItem(itMenu->second,actions,menus));
        }
        return  item;
    }
}

namespace   FE
{
    FEUiMenuItem::~FEUiMenuItem()
    {
        for (auto* child : _children)
            delete  child;
    }

    FEUiLayout::~FEUiLayout()
    {
        for (auto& item : _items)
        {
            delete  item._widget;
            delete  item._layout;
        }
    }

    FEUiItem&   FEUiLayout::addItem()
    {
        _items.push_back(FEUiItem());
        return  _items.back();
    }

    FEUiWidget::~FEUiWidget()
    {
        delete  _layout;
        for (auto* child : _children)
            delete  child;
    }

    std::string FEUiWidget::currentOption() const
    {
        if (_currentIndex < 0 || _currentIndex >= int(_options.size()))
            return  std::string();
        return  _options[size_t(_currentIndex)];
    }

    FEUiDocument::~FEUiDocument()
    {
        clear();
    }

    void    FEUiDocument::clear()
    {
        delete  _root;
        _root   =   nullptr;
        delete  _menuBar;
        _menuBar    =   nullptr;
        _error.clear();
    }

    bool    FEUiDocument::load(const std::string& file)
    {
        clear();

        std::FILE*  fp  =   std::fopen(file.c_str(),"rb");
        if (fp == nullptr)
        {
            _error  =   "open failed: " + file;
            return  false;
        }
        std::fseek(fp,0,SEEK_END);
        const   long    size    =   std::ftell(fp);
        std::fseek(fp,0,SEEK_SET);
        if (size <= 0)
        {
            std::fclose(fp);
            _error  =   "empty file: " + file;
            return  false;
        }
        _buffer.resize(size_t(size) + 1);
        const   size_t  read    =   std::fread(_buffer.data(),1,size_t(size),fp);
        std::fclose(fp);
        _buffer[read]   =   '\0';

        char*   begin   =   _buffer.data();
        /// <summary>
        /// 跳过 UTF-8 BOM
        /// </summary>
        if (read >= 3 && static_cast<unsigned char>(begin[0]) == 0xEF
            && static_cast<unsigned char>(begin[1]) == 0xBB
            && static_cast<unsigned char>(begin[2]) == 0xBF)
            begin   +=  3;

        rapidxml::xml_document<>    doc;
        try
        {
            doc.parse<0>(begin);
        }
        catch (const rapidxml::parse_error& e)
        {
            _error  =   std::string("parse error: ") + e.what();
            return  false;
        }

        auto*   uiNode  =   doc.first_node("ui");
        auto*   widgetNode  =   (uiNode != nullptr) ? uiNode->first_node("widget") : nullptr;
        if (widgetNode == nullptr)
        {
            _error  =   "no <ui>/<widget> node";
            return  false;
        }
        _root   =   static_cast<FEUiWidget*>(parseWidget(widgetNode));

        /// <summary>
        /// 解析菜单栏:先收集 &lt;action&gt; 定义与 QMenu 定义,再按 addaction 展开层级
        /// </summary>
        MenuNodeMap     actions;
        /// <summary>
        /// 动作既可能挂在 &lt;ui&gt; 下,也可能挂在根 QMainWindow 下(Designer 两种写法都有)
        /// </summary>
        for (void* parent : { static_cast<void*>(uiNode),static_cast<void*>(widgetNode) })
        {
            for (void* a = childNode(parent,"action"); a != nullptr;
                 a = static_cast<rapidxml::xml_node<>*>(a)->next_sibling("action"))
            {
                const   char*   actionName  =   attr(a,"name");
                if (actionName != nullptr)
                    actions[actionName] =   a;
            }
        }
        MenuNodeMap     menus;
        collectMenus(widgetNode,menus);

        void*   menuBarNode =   nullptr;
        for (void* w = childNode(widgetNode,"widget"); w != nullptr;
             w = static_cast<rapidxml::xml_node<>*>(w)->next_sibling("widget"))
        {
            const   char*   cls     =   attr(w,"class");
            if (cls != nullptr && std::strcmp(cls,"QMenuBar") == 0)
            {
                menuBarNode =   w;
                break;
            }
        }
        if (menuBarNode != nullptr)
        {
            _menuBar    =   parseMenuItem(menuBarNode,actions,menus);
            /// 菜单栏本身没有 title,Page 的标题由各主菜单的 title 提供
            _menuBar->_text.clear();
        }

        _buffer.clear();
        _buffer.shrink_to_fit();
        return  _root != nullptr;
    }

    void*   FEUiDocument::parseWidget(void* node)
    {
        FEUiWidget*     widget  =   new FEUiWidget();
        const   char*   cls     =   attr(node,"class");
        const   char*   name    =   attr(node,"name");
        widget->_class  =   (cls != nullptr) ? cls : "QWidget";
        widget->_name   =   (name != nullptr) ? name : std::string();

        for (void* p = childNode(node,"property"); p != nullptr;
             p = static_cast<rapidxml::xml_node<>*>(p)->next_sibling("property"))
            parseProperty(widget,p);

        for (void* l = childNode(node,"layout"); l != nullptr;
             l = static_cast<rapidxml::xml_node<>*>(l)->next_sibling("layout"))
        {
            if (widget->_layout == nullptr)
                widget->_layout =   static_cast<FEUiLayout*>(parseLayout(l));
        }

        for (void* c = childNode(node,"widget"); c != nullptr;
             c = static_cast<rapidxml::xml_node<>*>(c)->next_sibling("widget"))
            widget->_children.push_back(static_cast<FEUiWidget*>(parseWidget(c)));

        /// <summary>
        /// QComboBox 的选项:<item><property name="text"><string>..</string>
        /// </summary>
        if (widget->_class == "QComboBox")
        {
            for (void* item = childNode(node,"item"); item != nullptr;
                 item = static_cast<rapidxml::xml_node<>*>(item)->next_sibling("item"))
            {
                const   char*   text    =   childText(childNode(item,"property"),"string");
                if (text != nullptr)
                    widget->_options.push_back(decodeEntities(text));
            }
        }
        return  widget;
    }

    void*   FEUiDocument::parseLayout(void* node)
    {
        FEUiLayout*     layout  =   new FEUiLayout();
        const   char*   cls     =   attr(node,"class");
        layout->_class  =   (cls != nullptr) ? cls : "QVBoxLayout";
        layout->_rowStretch =   parseIntList(attr(node,"rowstretch"));
        layout->_colStretch =   parseIntList(attr(node,"colstretch"));

        for (void* p = childNode(node,"property"); p != nullptr;
             p = static_cast<rapidxml::xml_node<>*>(p)->next_sibling("property"))
        {
            const   char*   name    =   attr(p,"name");
            if (name != nullptr && std::strcmp(name,"spacing") == 0)
                layout->_spacing    =   float(childInt(p,"number"));
        }

        for (void* itemNode = childNode(node,"item"); itemNode != nullptr;
             itemNode = static_cast<rapidxml::xml_node<>*>(itemNode)->next_sibling("item"))
        {
            FEUiItem&   item    =   layout->addItem();
            item._row       =   std::atoi(attr(itemNode,"row") ? attr(itemNode,"row") : "0");
            item._col       =   std::atoi(attr(itemNode,"column") ? attr(itemNode,"column") : "0");
            item._rowSpan   =   std::atoi(attr(itemNode,"rowspan") ? attr(itemNode,"rowspan") : "1");
            item._colSpan   =   std::atoi(attr(itemNode,"colspan") ? attr(itemNode,"colspan") : "1");

            void*   widgetNode  =   childNode(itemNode,"widget");
            void*   layoutNode  =   childNode(itemNode,"layout");
            if (widgetNode != nullptr)
                item._widget    =   static_cast<FEUiWidget*>(parseWidget(widgetNode));
            else if (layoutNode != nullptr)
                item._layout    =   static_cast<FEUiLayout*>(parseLayout(layoutNode));
        }
        return  layout;
    }

    void    FEUiDocument::parseProperty(FEUiWidget* widget,void* property)
    {
        const   char*   name    =   attr(property,"name");
        if (name == nullptr)
            return;

        if (std::strcmp(name,"text") == 0)
        {
            const   char*   text    =   childText(property,"string");
            if (text != nullptr)
                widget->_text   =   decodeEntities(text);
        }
        else if (std::strcmp(name,"checked") == 0)
        {
            const   char*   text    =   childText(property,"bool");
            widget->_checked    =   (text != nullptr) && (std::strcmp(text,"true") == 0);
        }
        else if (std::strcmp(name,"enabled") == 0)
        {
            const   char*   text    =   childText(property,"bool");
            widget->_enabled    =   (text == nullptr) || (std::strcmp(text,"true") == 0);
        }
        else if (std::strcmp(name,"currentIndex") == 0)
        {
            widget->_currentIndex   =   childInt(property,"number");
        }
        else if (std::strcmp(name,"geometry") == 0)
        {
            void*   rectNode    =   childNode(property,"rect");
            if (rectNode != nullptr)
            {
                widget->_geometry._x   =   float(childInt(rectNode,"x"));
                widget->_geometry._y   =   float(childInt(rectNode,"y"));
                widget->_geometry._w   =   float(childInt(rectNode,"width"));
                widget->_geometry._h   =   float(childInt(rectNode,"height"));
            }
        }
        else if (std::strcmp(name,"sizePolicy") == 0)
        {
            void*   policy  =   childNode(property,"sizepolicy");
            widget->_hStretch   =   childInt(policy,"horstretch");
            widget->_vStretch   =   childInt(policy,"verstretch");
        }
    }

    FEUiWidget* FEUiDocument::find(const std::string& name) const
    {
        std::vector<const FEUiWidget*>  stack;
        if (_root != nullptr)
            stack.push_back(_root);
        while (!stack.empty())
        {
            const   FEUiWidget*     widget  =   stack.back();
            stack.pop_back();
            if (widget->_name == name)
                return  const_cast<FEUiWidget*>(widget);
            if (widget->_layout != nullptr)
            {
                for (const auto& item : widget->_layout->_items)
                {
                    if (item._widget != nullptr)
                        stack.push_back(item._widget);
                }
            }
            for (const auto* child : widget->_children)
                stack.push_back(child);
        }
        return  nullptr;
    }

    FEUiWidget* FEUiDocument::findClass(const std::string& cls) const
    {
        std::vector<const FEUiWidget*>  stack;
        if (_root != nullptr)
            stack.push_back(_root);
        while (!stack.empty())
        {
            const   FEUiWidget*     widget  =   stack.back();
            stack.pop_back();
            if (widget->_class == cls)
                return  const_cast<FEUiWidget*>(widget);
            if (widget->_layout != nullptr)
            {
                for (const auto& item : widget->_layout->_items)
                {
                    if (item._widget != nullptr)
                        stack.push_back(item._widget);
                }
            }
            for (const auto* child : widget->_children)
                stack.push_back(child);
        }
        return  nullptr;
    }
}
