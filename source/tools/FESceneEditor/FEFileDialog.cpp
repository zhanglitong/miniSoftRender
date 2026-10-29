/// <summary>
/// 自绘文件选择对话框的实现:目录导航、扩展名过滤、文件类型下拉框与确认/取消。
/// 与 FEEditorUi 分离,便于独立维护和复用。
/// </summary>

#include    "FEFileDialog.hpp"

#include    "../../../inc/FEUnused.hpp"

#include    <algorithm>
#include    <cctype>
#include    <cstdlib>
#include    <cstring>
#include    <imgui.h>

namespace   FE
{
    void    FEFileDialog::open(Mode mode,const std::string& title,const std::vector<Filter>& filters,
                               const std::string& dir,const std::string& defaultName)
    {
        _mode   =   mode;
        _title  =   title;
        _filters    =   filters;
        _filterIndex    =   0;
        _ext    =   _filters.empty() ? std::string() : _filters.front()._ext;
        _notice.clear();
        _selected   =   -1;
        _nameBuf[0] =   '\0';
        if (!defaultName.empty())
        {
            strncpy(_nameBuf,defaultName.c_str(),sizeof(_nameBuf) - 1);
            _nameBuf[sizeof(_nameBuf) - 1]  =   '\0';
        }
        /// <summary>
        /// 初始目录不存在时回落到当前工作目录
        /// </summary>
        std::error_code ec;
        std::filesystem::path   start   =   std::filesystem::u8path(dir);
        if (dir.empty() || !std::filesystem::is_directory(start,ec))
            start   =   std::filesystem::current_path(ec);
        enter(start);
        buildQuickItems(dir);
        _firstFrame =   true;
    }

    void    FEFileDialog::close()
    {
        _mode   =   kNone;
        _entries.clear();
        _dirty  =   true;
    }

    void    FEFileDialog::buildQuickItems(const std::string& startDir)
    {
        _quickItems.clear();
        /// <summary>
        /// 只收录真实存在的目录,避免点到无效位置
        /// </summary>
        auto    add =   [this](const std::string& label,const std::filesystem::path& path)
        {
            std::error_code ec;
            if (!std::filesystem::is_directory(path,ec) || ec)
                return;
            _quickItems.push_back({label,path});
        };

        /// <summary>
        /// 用户主目录及其常用子目录(Windows 取 USERPROFILE,类 Unix 取 HOME)
        /// </summary>
        const   char*   home    =   std::getenv("USERPROFILE");
        if (home == nullptr || home[0] == '\0')
            home    =   std::getenv("HOME");
        if (home != nullptr && home[0] != '\0')
        {
            const   std::filesystem::path   homeDir(home);
            add("桌面",     homeDir / "Desktop");
            add("下载",     homeDir / "Downloads");
            add("文档",     homeDir / "Documents");
            add("主文件夹", homeDir);
        }

        /// <summary>
        /// 程序目录(与 Qt 版文件对话框的起始位置一致)
        /// </summary>
        if (!startDir.empty())
            add("程序目录",std::filesystem::u8path(startDir));

        /// <summary>
        /// 各盘符根:类 Unix 上 "X:\" 都不存在,循环自然为空,再补上根目录
        /// </summary>
        int rootCount   =   0;
        for (char c = 'A'; c <= 'Z'; ++c)
        {
            std::string root;
            root    +=  c;
            root    +=  ':';
            root    +=  std::filesystem::path::preferred_separator;
            const   std::filesystem::path   drive(root);
            std::error_code ec;
            if (!std::filesystem::is_directory(drive,ec) || ec)
                continue;
            _quickItems.push_back({std::string(1,c) + ":",drive});
            ++rootCount;
        }
        if (rootCount == 0)
            add("/",std::filesystem::path("/"));
    }

    void    FEFileDialog::enter(const std::filesystem::path& dir)
    {
        std::error_code ec;
        std::filesystem::path   target  =   std::filesystem::absolute(dir,ec);
        if (ec)
            target  =   dir;
        _dir    =   target.lexically_normal();
        _dirty  =   true;
        _selected   =   -1;
        setPathBuf(_dir.u8string());
    }

    void    FEFileDialog::setPathBuf(const std::string& utf8Path)
    {
        strncpy(_pathBuf,utf8Path.c_str(),sizeof(_pathBuf) - 1);
        _pathBuf[sizeof(_pathBuf) - 1]  =   '\0';
    }

    void    FEFileDialog::gotoPath(const std::string& text)
    {
        /// <summary>
        /// 去掉首尾空白与从资源管理器复制路径时携带的引号
        /// </summary>
        std::string     input   =   text;
        while (!input.empty() && (input.front() == ' ' || input.front() == '"'))
            input.erase(input.begin());
        while (!input.empty() && (input.back() == ' ' || input.back() == '"'))
            input.pop_back();
        if (input.empty())
        {
            setPathBuf(_dir.u8string());
            return;
        }

        std::filesystem::path   target  =   std::filesystem::u8path(input);
        /// <summary>
        /// 只输入盘符(如 D:)时补上分隔符,否则无法作为目录打开
        /// </summary>
        const   std::string     native  =   target.string();
        if (native.size() == 2 && native[1] == ':')
            target  +=  std::filesystem::path::preferred_separator;

        std::error_code ec;
        if (std::filesystem::is_directory(target,ec) && !ec)
        {
            enter(target);
            return;
        }
        ec.clear();
        /// <summary>
        /// 输入的是文件:跳到其所在目录并填入文件名
        /// </summary>
        if (std::filesystem::is_regular_file(target,ec) && !ec)
        {
            strncpy(_nameBuf,target.filename().u8string().c_str(),sizeof(_nameBuf) - 1);
            _nameBuf[sizeof(_nameBuf) - 1]  =   '\0';
            enter(target.parent_path());
            return;
        }
        /// <summary>
        /// 路径无效:提示并把地址栏恢复成当前目录,浏览位置不变
        /// </summary>
        _notice =   "路径不存在:" + input;
        setPathBuf(_dir.u8string());
    }

    std::string FEFileDialog::firstExt() const
    {
        const   size_t  end =   _ext.find(';');
        return  _ext.substr(0,end);
    }

    void    FEFileDialog::refresh()
    {
        _entries.clear();
        _dirty      =   false;
        _notice.clear();
        std::error_code ec;
        std::filesystem::directory_iterator   itr(_dir,ec);
        if (ec)
        {
            _notice =   "无法读取目录:" + ec.message();
            return;
        }
        /// <summary>
        /// 扩展名过滤支持以分号分隔多个扩展(如 "导入模型" 的 .gltf;.glb)
        /// </summary>
        std::string filter  =   _ext;
        std::transform(filter.begin(),filter.end(),filter.begin(),
                       [](unsigned char c) {   return  char(std::tolower(c));   });
        std::vector<std::string>    exts;
        size_t  begin   =   0;
        while (begin <= filter.size())
        {
            const   size_t  end =   filter.find(';',begin);
            const   std::string ext =   filter.substr(begin,end == std::string::npos
                                                           ? std::string::npos : end - begin);
            if (!ext.empty())
                exts.push_back(ext);
            if (end == std::string::npos)
                break;
            begin   =   end + 1;
        }
        for (const auto& item : itr)
        {
            std::error_code ecItem;
            const   bool    isDir   =   item.is_directory(ecItem);
            if (ecItem)
                continue;
            std::string     name    =   item.path().filename().u8string();
            if (isDir)
            {
                _entries.push_back({name,true});
                continue;
            }
            if (!exts.empty())
            {
                std::string ext     =   item.path().extension().u8string();
                std::transform(ext.begin(),ext.end(),ext.begin(),
                               [](unsigned char c) {   return  char(std::tolower(c));   });
                if (std::find(exts.begin(),exts.end(),ext) == exts.end())
                    continue;
            }
            _entries.push_back({name,false});
        }
        /// <summary>
        /// 目录在前、同类按名称升序(与系统文件对话框一致)
        /// </summary>
        std::sort(_entries.begin(),_entries.end(),[](const Entry& l,const Entry& r)
        {
            if (l._dir != r._dir)
                return  l._dir;
            return  l._name < r._name;
        });
    }

    bool    FEFileDialog::draw(ImFont* iconFont,std::string& resultPath)
    {
        UNUSED(iconFont);
        if (_mode == kNone)
            return  false;
        if (_dirty)
            refresh();

        bool    confirmed   =   false;
        bool    cancelled   =   false;
        std::string picked;
        bool    opened      =   true;
        /// <summary>
        /// 对话框居中,首次绘制定位置与大小,之后允许用户拖动
        /// </summary>
        const   ImGuiIO&    io  =   ImGui::GetIO();
        if (_firstFrame)
        {
            const   ImVec2  dlgSize =   ImVec2(std::min(720.0f,io.DisplaySize.x - 40.0f),
                                               std::min(460.0f,io.DisplaySize.y - 40.0f));
            ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - dlgSize.x) * 0.5f,
                                          (io.DisplaySize.y - dlgSize.y) * 0.5f),ImGuiCond_Always);
            ImGui::SetNextWindowSize(dlgSize,ImGuiCond_Always);
        }
        if (ImGui::Begin(_title.c_str(),&opened,ImGuiWindowFlags_NoCollapse))
        {
            const   float   rowH    =   ImGui::GetFrameHeightWithSpacing();
            /// <summary>
            /// 高度预留:位置栏 + 文件名 + 底部按钮(多过滤器时再加一行文件类型)
            /// </summary>
            const   float   listH   =   std::max(80.0f,ImGui::GetWindowHeight()
                                                 - rowH * (_filters.size() > 1 ? 5.5f : 4.5f));

            /// <summary>
            /// 位置栏:上级目录 + 可编辑地址栏(可直接输入路径或盘符,回车跳转) + 转到
            /// </summary>
            const   ImGuiStyle& style   =   ImGui::GetStyle();
            if (ImGui::Button("上级目录") && _dir.has_parent_path())
                enter(_dir.parent_path());
            ImGui::SameLine();
            const   float   upW     =   ImGui::CalcTextSize("上级目录").x + style.FramePadding.x * 2.0f;
            const   float   goW     =   ImGui::CalcTextSize("转到").x + style.FramePadding.x * 2.0f;
            ImGui::PushItemWidth(ImGui::GetWindowWidth() - upW - goW - style.ItemSpacing.x * 3.0f
                                 - style.WindowPadding.x * 2.0f);
            const   bool    go      =   ImGui::InputText("##fesr_fd_path",_pathBuf,sizeof(_pathBuf),
                                                         ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("转到") || go)
                gotoPath(_pathBuf);
            ImGui::Separator();

            /// <summary>
            /// 左侧导航栏 + 右侧文件列表(仿资源管理器的双栏布局)
            /// </summary>
            const   float   navW    =   132.0f;
            const   float   listW   =   std::max(200.0f,ImGui::GetWindowWidth() - navW
                                                 - style.ItemSpacing.x - style.WindowPadding.x * 2.0f);
            ImGui::BeginChild("##fesr_fd_nav",ImVec2(navW,listH),true);
            for (const auto& item : _quickItems)
            {
                if (ImGui::Selectable(item._label.c_str(),item._path == _dir))
                    enter(item._path);
            }
            ImGui::EndChild();
            ImGui::SameLine();

            /// <summary>
            /// 文件与目录列表:单击选中,双击进入目录或确认文件
            /// </summary>
            ImGui::BeginChild("##fesr_fd_list",ImVec2(listW,listH),true);
            for (int i = 0; i < int(_entries.size()); ++i)
            {
                const   Entry&  entry   =   _entries[size_t(i)];
                ImGui::PushID(i);
                const   std::string label   =   entry._dir ? ("[ 目录 ] " + entry._name) : entry._name;
                if (ImGui::Selectable(label.c_str(),_selected == i,
                                      ImGuiSelectableFlags_AllowDoubleClick))
                {
                    _selected   =   i;
                    if (entry._dir)
                    {
                        if (ImGui::IsMouseDoubleClicked(0))
                            enter(_dir / std::filesystem::u8path(entry._name));
                    }
                    else
                    {
                        strncpy(_nameBuf,entry._name.c_str(),sizeof(_nameBuf) - 1);
                        _nameBuf[sizeof(_nameBuf) - 1]  =   '\0';
                        if (ImGui::IsMouseDoubleClicked(0))
                            confirmed   =   true;
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
            if (!_notice.empty())
                ImGui::TextDisabled("%s",_notice.c_str());

            /// <summary>
            /// 文件名输入框:回车等同确认
            /// </summary>
            ImGui::PushItemWidth(-1.0f);
            if (ImGui::InputText("##fesr_fd_name",_nameBuf,sizeof(_nameBuf),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
                confirmed   =   true;
            ImGui::PopItemWidth();

            /// <summary>
            /// 文件类型下拉框:仅在提供多个过滤器时显示,切换后按新扩展名重新扫描目录
            /// </summary>
            if (_filters.size() > 1)
            {
                std::vector<const char*>    labels;
                labels.reserve(_filters.size());
                for (const auto& item : _filters)
                    labels.push_back(item._label.c_str());
                ImGui::TextUnformatted("文件类型");
                ImGui::SameLine();
                ImGui::PushItemWidth(-1.0f);
                if (ImGui::Combo("##fesr_fd_type",&_filterIndex,labels.data(),int(labels.size())))
                {
                    _ext        =   _filters[size_t(_filterIndex)]._ext;
                    _selected   =   -1;
                    _dirty      =   true;
                }
                ImGui::PopItemWidth();
            }

            /// <summary>
            /// 底部按钮:确定 / 取消(文件名为空时"确定"淡显且不可用)
            /// </summary>
            const   bool    hasName =   (_nameBuf[0] != '\0');
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,hasName ? 1.0f : 0.45f);
            if (ImGui::Button(_mode == kSave ? "保存" : "打开",ImVec2(96.0f,0.0f)) && hasName)
                confirmed   =   true;
            ImGui::PopStyleVar();
            ImGui::SameLine();
            if (ImGui::Button("取消",ImVec2(96.0f,0.0f)))
                cancelled   =   true;

            /// <summary>
            /// 确认:输入的是文件夹则进入;否则拼接完整路径,
            /// 另存为时缺扩展名自动补上
            /// </summary>
            if (confirmed && hasName)
            {
                std::filesystem::path   file    =   std::filesystem::u8path(_nameBuf);
                std::error_code         ec;
                if (std::filesystem::is_directory(_dir / file,ec) && !ec)
                {
                    enter(_dir / file);
                }
                else
                {
                    const   std::string ext =   firstExt();
                    if (_mode == kSave && !ext.empty() && file.extension() != ext)
                        file    +=  ext;
                    picked  =   (_dir / file).u8string();
                }
            }
        }
        ImGui::End();
        _firstFrame =   false;

        /// <summary>
        /// 取消按钮 / 标题栏关闭按钮:结束对话框,不产出结果
        /// </summary>
        if (cancelled || !opened)
        {
            close();
            return  true;
        }
        /// <summary>
        /// 未确认(或确认时文件名为空):保持打开,继续浏览
        /// </summary>
        if (!confirmed || picked.empty())
            return  false;
        /// <summary>
        /// 确认:产出完整路径并结束
        /// </summary>
        resultPath  =   picked;
        close();
        return  true;
    }
}
