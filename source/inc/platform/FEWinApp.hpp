#pragma     once
#include    "../FEApp.hpp"
#include    "../FEPath.hpp"
#include    <windows.h>
#include    <windowsx.h>
#include    <fcntl.h>
#include    <io.h>
#include    <ShellScalingAPI.h>
#include    <filesystem>
namespace   FE
{
    class   FEWinApp:public FEApp
    {
    public:
        FEWinApp(FEContext& ctx)
            :FEApp(ctx)
        {
            setupDPIAwareness();
        }
        FEWinApp(const FEApp& other)
            :FEApp(other)
        {
        }
        virtual ~FEWinApp()
        {}
    protected:
        bool        _resizing   =   false;
        int2        _mouse      =   int2(0,0);
    public:
        virtual RectU32 rect() const override 
        {
            RECT    rect    =   {};
            GetClientRect((HWND)_cInfo._window,&rect);

            RectU32 result(rect.left,rect.top,rect.right,rect.bottom);
            return  result;
        }
        /// 
        /// </summary>
        /// <param name="cInf"></param>
        virtual bool    setup(const CreateInfo& settings) override
        {
            LOG_DBG("FEWinApp.setup");
            _cInfo  =   settings;
            bool    result  =   false;
            if (!_cInfo._notify)
                _cInfo._notify  =   [](const FEMessage& ){};
            if (_cInfo._window == nullptr)
                result  =   setupCreateWindow();
            else
                result  =   setupUsingWindow();
            if (!result)    
                return  false;
            /// 设置上下文相关
            _ctx.setWindow(this);
            _ctx.setWorkPath(path());
            /// 直接传绝对路径:exe 所在目录的上级,避免 "../" 这类相对片段
            _ctx.setResourcePath(std::filesystem::path(path()).parent_path().string());

            return  true;
        }
        /// <summary>
        /// 
        /// </summary>
        virtual void    run() override
        {
            MSG     msg     =   {};
            bool    quit    =   false;
            while (!quit) 
            {
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    if (msg.message == WM_QUIT) 
                    {
                        quit = true;
                        break;
                    }
                }
                if (!quit && !IsIconic((HWND)_cInfo._window)) 
                {
                    MsgUpdate   msgUpdate   =   {};
                    MsgRender   msgRender   =   {};
                    /// 更新消息
                    onMessage(msgUpdate);
                    /// 绘制消息
                    onMessage(msgRender);
                }
            }
        }
        /// <summary>
        /// 
        /// </summary>
        virtual void    destroy()  override
        {
            LOG_INF("%s","_app:destroy!");
        }
        virtual void    renderOneFrame() override
        {}

        /// <summary>
        /// 获取当前应用的全路径
        /// </summary>
        /// <returns></returns>
        virtual String  filePathName() override
        {
            /// exe 路径统一由平台层提供(宽字符 API,与 FEPath.cpp 共用一处实现)
            return  exePath().string();
        }
        virtual String  path() override
        {
            std::filesystem::path filePath(filePathName());
            return  filePath.parent_path().string();
        }
    public:
        virtual bool    setupCreateWindow()
        {
            WNDCLASSEXW wndClass         =   {};
            {
                wndClass.cbSize         =   sizeof(WNDCLASSEXW);
                wndClass.style          =   CS_HREDRAW | CS_VREDRAW;
                wndClass.lpfnWndProc    =   wndProcEx;
                wndClass.cbClsExtra     =   0;
                wndClass.cbWndExtra     =   0;
                wndClass.hInstance      =   nullptr;
                wndClass.hIcon          =   LoadIcon(nullptr, IDI_APPLICATION);
                wndClass.hCursor        =   LoadCursor(NULL, IDC_ARROW);
                wndClass.hbrBackground  =   (HBRUSH)GetStockObject(BLACK_BRUSH);
                wndClass.lpszMenuName   =   nullptr;
                wndClass.lpszClassName  =   L"FEEngine";
                wndClass.hIconSm        =   LoadIcon(NULL, IDI_WINLOGO);
            }
            /// <summary>
            /// 必须注册 Unicode 窗口类:ANSI 窗口下输入法提交的中文经 WM_IME_CHAR/WM_CHAR
            /// 只传 DBCS 字节,ImGui 无法得到 Unicode 码点,中文输入会失效
            /// </summary>
            if (!RegisterClassExW(&wndClass))
            {
                LOG_ERR("FEWinApp.setup/RegisterClassExW");
                return  false;
            }
            auto    width           =    _cInfo._width;
            auto    height          =    _cInfo._height;
            auto    screenWidth     =    GetSystemMetrics(SM_CXSCREEN);
            auto    screenHeight    =    GetSystemMetrics(SM_CYSCREEN);

            if (_cInfo._fullscreen)
            {
                if ((width != screenWidth) && (height != screenHeight))
                {
                    DEVMODEA dmScreenSettings       =   {};
                    dmScreenSettings.dmSize         =   sizeof(dmScreenSettings);
                    dmScreenSettings.dmPelsWidth    =   width;
                    dmScreenSettings.dmPelsHeight   =   height;
                    dmScreenSettings.dmBitsPerPel   =   32;
                    dmScreenSettings.dmFields       =   DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
                    if (ChangeDisplaySettingsA(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
                    {
                        if (MessageBoxA(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                        {
                            return  false;
                        }
                        else
                        {
                            return  false;
                        }
                    }
                    screenWidth     =   width;
                    screenHeight    =   height;
                }
            }

            DWORD   dwExStyle   =   {};
            DWORD   dwStyle     =   {};

            if (_cInfo._fullscreen)
            {
                dwExStyle   =   WS_EX_APPWINDOW;
                dwStyle     =   WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
            }
            else
            {
                dwExStyle   =   WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
                dwStyle     =   WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
            }

            RECT windowRect     =   {};
            windowRect.left     =   0;
            windowRect.top      =   0;
            windowRect.right    =   _cInfo._fullscreen ? (long)screenWidth    : (long)width;
            windowRect.bottom   =   _cInfo._fullscreen ? (long)screenHeight   : (long)height;

            AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

            auto    windowTitle =   L"FEWindow";
            auto    window      =   CreateWindowExW(0,
                L"FEEngine",
                windowTitle,
                dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                0,
                0,
                windowRect.right  - windowRect.left,
                windowRect.bottom - windowRect.top,
                nullptr,
                nullptr,
                nullptr,
                this);

            if (!window)
            {
                LOG_ERR("FEWinApp.setup/CreateWindowExW");
                return false;
            }

            if (!_cInfo._fullscreen)
            {
                // Center on screen
                uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
                uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
                SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }

            ShowWindow(window, SW_SHOW);
            SetForegroundWindow(window);
            SetFocus(window);
            _cInfo._window  =   window;
            LOG_DBG("FEWinApp._cInfo{width:%d,height:%d,fullscreen:%s}",width,height,_cInfo._fullscreen ? "true" : "false");
            return  true;
        }
        virtual bool    setupUsingWindow()
        {
            return  true;
        }
        /// <summary>
        /// 鼠标消息坐标:lParam 是客户区坐标,须按有符号 16 位解析;
        /// 拖拽出窗口(隐式捕获)时会出现负坐标,不能用无符号 LOWORD/HIWORD
        /// </summary>
        static  int2    mouseClientPos(LPARAM lParam)
        {
            return  int2(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
        }
        /// <summary>
        /// 滚轮消息 lParam 是屏幕坐标,先转成客户区坐标再派发
        /// </summary>
        static  int2    mouseWheelClientPos(HWND hWnd,LPARAM lParam)
        {
            POINT   pt  =   { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd,&pt);
            return  int2(pt.x,pt.y);
        }
        /// <summary>
        /// 鼠标消息 wParam 只有 MK_* 位(没有 Alt),修饰键统一查键盘状态
        /// </summary>
        static  uint32  mouseModifierFlags()
        {
            uint32  flags   =   0;
            if (GetKeyState(VK_CONTROL) & 0x8000)   flags   |=  FEMouseInfo::CtrlPressed;
            if (GetKeyState(VK_SHIFT)   & 0x8000)   flags   |=  FEMouseInfo::ShiftPressed;
            if (GetKeyState(VK_MENU)    & 0x8000)   flags   |=  FEMouseInfo::AltPressed;
            return  flags;
        }
        virtual LRESULT onMessage(MSG* pMsg)
        {
            HWND    hWnd    =   pMsg->hwnd;
            UINT    uMsg    =   pMsg->message;
            WPARAM  wParam  =   pMsg->wParam;
            LPARAM  lParam  =   pMsg->lParam;
            switch (uMsg)
            {
            case WM_CREATE:
                onMessage(MsgCreate());
                break;
            case WM_DESTROY:
                onMessage(MsgDestroy());
                /// 统一在销毁时投递退出消息:即使窗口不是经 WM_CLOSE 关闭,
                /// run() 的消息循环也能结束,不会在无窗口状态下空转
                PostQuitMessage(0);
                return  0;
            case WM_CLOSE:
                onMessage(MsgClose());
                DestroyWindow(hWnd);
                /// 窗口已销毁,不能再落到 DefWindowProc 的默认关闭处理
                return  0;
            case WM_PAINT:
                ValidateRect(hWnd, NULL);
                return  0;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                /// Alt 组合键走 WM_SYSKEYDOWN,同样要派发,
                /// 否则 ImGui 收不到按键也不会更新 io.KeyAlt
                onMessage(MsgKeyDown((uint)wParam));
                break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                onMessage(MsgKeyUp((uint)wParam));
                break;
            case WM_CHAR:
                /// 字符输入(含中文):UTF-16 码元,ImGui 的文本控件依赖此消息
                onMessage(MsgChar(FECharInfo((unsigned int)wParam)));
                break;
            case WM_LBUTTONDOWN:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._states.addFlag(FEMouseInfo::LButtonPressed);
                    info._states.addFlags(mouseModifierFlags());
                    _mouse      =   info._mouse;
                    onMessage(MsgLButtonDown(info));
                }
                break;
            case WM_RBUTTONDOWN:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._states.addFlag(FEMouseInfo::RButtonPressed);
                    info._states.addFlags(mouseModifierFlags());
                    _mouse      =   info._mouse;
                    onMessage(MsgRButtonDown(info));
                }
                break;
            case WM_MBUTTONDOWN:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._states.addFlag(FEMouseInfo::MButtonPressed);
                    info._states.addFlags(mouseModifierFlags());
                    _mouse      =   info._mouse;
                    onMessage(MsgMButtonDown(info));
                }
                break;
            case WM_LBUTTONUP:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._states.addFlags(mouseModifierFlags());
                    _mouse      =   info._mouse;
                    onMessage(MsgLButtonUp(info));
                }
                break;
            case WM_RBUTTONUP:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._states.addFlags(mouseModifierFlags());
                    _mouse      =   info._mouse;
                    onMessage(MsgRButtonUp(info));
                }
                break;
            case WM_MBUTTONUP:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._states.addFlags(mouseModifierFlags());
                    _mouse      =   info._mouse;
                    onMessage(MsgMButtonUp(info));
                }
                break;
            case WM_MOUSEWHEEL:
                {
                    /// 滚轮 lParam 是屏幕坐标,先转客户区再派发并同步 _mouse
                    int2    mouse   =   mouseWheelClientPos(hWnd,lParam);
                    _mouse          =   mouse;
                    onMessage(MsgMouseWheel(FEMouseWheel(mouse,GET_WHEEL_DELTA_WPARAM(wParam))));
                }
                break;
            case WM_MOUSEMOVE:
                {
                    FEMouseInfo info;
                    info._mouse =   mouseClientPos(lParam);
                    info._prev  =   _mouse;
                    _mouse      =   info._mouse;

                    if (wParam & MK_LBUTTON)    info._states.addFlag(FEMouseInfo::LButtonPressed);
                    if (wParam & MK_MBUTTON)    info._states.addFlag(FEMouseInfo::MButtonPressed);
                    if (wParam & MK_RBUTTON)    info._states.addFlag(FEMouseInfo::RButtonPressed);

                    if (wParam & MK_XBUTTON1)   info._states.addFlag(FEMouseInfo::XButton1Pressed);
                    if (wParam & MK_XBUTTON2)   info._states.addFlag(FEMouseInfo::XButton2Pressed);

                    info._states.addFlags(mouseModifierFlags());

                    onMessage(MsgMouseMove(info));
                }

                break;
            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED)
                {
                    if (_resizing || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
                    {
                        auto    destWidth   = LOWORD(lParam);
                        auto    destHeight  = HIWORD(lParam);
                        _cInfo._width   =   destWidth;
                        _cInfo._height  =   destHeight;
                        onMessage(MsgResize(uint2(destWidth, destHeight)));
                    }
                }
                break;
            case WM_GETMINMAXINFO:
                {
                    LPMINMAXINFO minMaxInfo         =   (LPMINMAXINFO)lParam;
                    minMaxInfo->ptMinTrackSize.x    =   64;
                    minMaxInfo->ptMinTrackSize.y    =   64;
                    break;
                }
            case WM_ENTERSIZEMOVE:
                _resizing   =   true;
                break;
            case WM_EXITSIZEMOVE:
                _resizing   =   false;
                break;
            }
            return  DefWindowProcW(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
        }

        /// <summary>
        /// onMessage 继承 FEApp 基类实现:先派发消息总线,再调 _notify
        /// using 声明:取消 onMessage(MSG*) 对基类 onMessage(const FEMessage&) 的名字隐藏
        /// </summary>
        using   FEApp::onMessage;
    public:
        static LRESULT CALLBACK  wndProcEx(HWND hWnd, UINT msgId, WPARAM wParam, LPARAM lParam)
        {
            MSG msg =   {};
            msg.hwnd    =   hWnd;
            msg.message =   msgId;
            msg.wParam  =   wParam;
            msg.lParam  =   lParam;
            FEWinApp*   pThis   =   nullptr;
            if (WM_NCCREATE == msgId)
            {
                /// lpCreateParams 只在 WM_NCCREATE 可用,必须在此挂上对象指针:
                /// 否则随后同步派发的 WM_GETMINMAXINFO/WM_CREATE 无法路由到本对象
                CREATESTRUCT*   create_struct   =   (CREATESTRUCT*)lParam;
                pThis   =   (FEWinApp*)create_struct->lpCreateParams;
                if (pThis)
                {
                    ::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
                    /// WM_CREATE/WM_SIZE 在 CreateWindowExW 内同步派发,
                    /// 此处置好句柄,保证 MsgCreate/MsgResize 里 rect() 有效
                    pThis->_cInfo._window   =   hWnd;
                }
            }
            else
            {
                pThis   =   (FEWinApp*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            }
            if (pThis)
            {
                return      pThis->onMessage(&msg);
            }
            return ::DefWindowProcW(hWnd, msgId, wParam, lParam);
        }

        static  void    setupDPIAwareness()
        {
            typedef HRESULT *(__stdcall *SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

            HMODULE shCore = LoadLibraryA("Shcore.dll");
            if (shCore)
            {
                SetProcessDpiAwarenessFunc setProcessDpiAwareness =
                    (SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

                if (setProcessDpiAwareness != nullptr)
                {
                    setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                }

                FreeLibrary(shCore);
            }
        }
    };
    using   WinApp      =   SharedPtr<FEWinApp>;
}
