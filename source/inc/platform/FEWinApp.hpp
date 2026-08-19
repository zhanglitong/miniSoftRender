#pragma     once
#include    "../FEApp.hpp"
#include    <windows.h>
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
            if (!_cInfo._notify)
            {
                _cInfo._notify  =   [](const FEMessage& ){};
            }

            WNDCLASSEXA wndClass         =   {};
            {
                wndClass.cbSize         =   sizeof(WNDCLASSEX);
                wndClass.style          =   CS_HREDRAW | CS_VREDRAW;
                wndClass.lpfnWndProc    =   wndProcEx;
                wndClass.cbClsExtra     =   0;
                wndClass.cbWndExtra     =   0;
                wndClass.hInstance      =   nullptr;
                wndClass.hIcon          =   LoadIcon(nullptr, IDI_APPLICATION);
                wndClass.hCursor        =   LoadCursor(NULL, IDC_ARROW);
                wndClass.hbrBackground  =   (HBRUSH)GetStockObject(BLACK_BRUSH);
                wndClass.lpszMenuName   =   nullptr,
                wndClass.lpszClassName  =   "FEEngine";
                wndClass.hIconSm        =   LoadIcon(NULL, IDI_WINLOGO);
            }
            if (!RegisterClassExA(&wndClass))
            {
                LOG_ERR("FEWinApp.setup/RegisterClassExA");
                return  false;
            }
            auto    width           =    settings._width;
            auto    height          =    settings._height;
            auto    screenWidth     =    GetSystemMetrics(SM_CXSCREEN);
            auto    screenHeight    =    GetSystemMetrics(SM_CYSCREEN);

            if (settings._fullscreen)
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
                            return nullptr;
                        }
                    }
                    screenWidth     =   width;
                    screenHeight    =   height;
                }
            }

            DWORD   dwExStyle   =   {};
            DWORD   dwStyle     =   {};

            if (settings._fullscreen)
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
            windowRect.right    =   settings._fullscreen ? (long)screenWidth    : (long)width;
            windowRect.bottom   =   settings._fullscreen ? (long)screenHeight   : (long)height;
            
            AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

            auto    windowTitle =   "FEWindow";
            auto    window      =   CreateWindowExA(0,
                                                    "FEEngine",
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
                LOG_ERR("FEWinApp.setup/CreateWindowExA");
                return false;
            }

            if (!settings._fullscreen)
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
            LOG_DBG("FEWinApp.settings{width:%d,height:%d,fullscreen:%s}",width,height,settings._fullscreen ? "true" : "false");
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
            char    szTemp[2048]    =   {};
            auto    result  =   GetModuleFileNameA(nullptr,szTemp,sizeof(szTemp));
            UNUSED(result);
            assert(result < sizeof(szTemp));
            return  szTemp;
        }
        virtual String  path() override
        {
            std::filesystem::path filePath(filePathName());
            return  filePath.parent_path().string();
        }
    public:
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
                break;
            case WM_CLOSE:
                onMessage(MsgClose());
                DestroyWindow(hWnd);
                PostQuitMessage(0);
                break;
            case WM_PAINT:
                ValidateRect((HWND)_cInfo._window, NULL);
                break;
            case WM_KEYDOWN:
                onMessage(MsgKeyDown((uint)wParam));
                break;
            case WM_KEYUP:
                onMessage(MsgKeyUp((uint)wParam));
                break;
            case WM_LBUTTONDOWN:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));
                    info._states.addFlag(FEMouseInfo::LButtonPressed);

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    onMessage(MsgLButtonDown(info));
                }
                break;
            case WM_RBUTTONDOWN:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));
                    info._states.addFlag(FEMouseInfo::RButtonPressed);

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    onMessage(MsgRButtonDown(info));
                }
                break;
            case WM_MBUTTONDOWN:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));
                    info._states.addFlag(FEMouseInfo::MButtonPressed);

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    onMessage(MsgMButtonDown(info));
                }
                break;
            case WM_LBUTTONUP:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    onMessage(MsgLButtonUp(info));
                }
                break;
            case WM_RBUTTONUP:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    onMessage(MsgRButtonUp(info));
                }
                break;
            case WM_MBUTTONUP:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    onMessage(MsgMButtonUp(info));
                }
                break;
            case WM_MOUSEWHEEL:
                onMessage(MsgMouseWheel(FEMouseWheel(int2(LOWORD(lParam), HIWORD(lParam)),GET_WHEEL_DELTA_WPARAM(wParam))));
                break;
            case WM_MOUSEMOVE:
                {
                    FEMouseInfo info;
                    info._mouse =   int2(LOWORD(lParam), HIWORD(lParam));
                    info._old   =   _mouse;
                    _mouse      =   info._mouse;

                    if (wParam & MK_LBUTTON)    info._states.addFlag(FEMouseInfo::LButtonPressed);
                    if (wParam & MK_MBUTTON)    info._states.addFlag(FEMouseInfo::MButtonPressed);
                    if (wParam & MK_RBUTTON)    info._states.addFlag(FEMouseInfo::RButtonPressed);

                    if (wParam & MK_XBUTTON1)   info._states.addFlag(FEMouseInfo::XButton1Pressed);
                    if (wParam & MK_XBUTTON2)   info._states.addFlag(FEMouseInfo::XButton2Pressed);

                    if (wParam & MK_CONTROL)    info._states.addFlag(FEMouseInfo::CtrlPressed);
                    if (wParam & MK_SHIFT)      info._states.addFlag(FEMouseInfo::ShiftPressed);
                    if (wParam & VK_MENU)       info._states.addFlag(FEMouseInfo::AltPressed);
                    
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
            return  DefWindowProcA(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
        }

        virtual void    onMessage(const FEMessage& msg) override 
        {
            if (_cInfo._notify)
            {
                _cInfo._notify(msg);
            }
        }
    public:
        static LRESULT CALLBACK  wndProcEx(HWND hWnd, UINT msgId, WPARAM wParam, LPARAM lParam)
        {
            MSG msg;
            msg.hwnd    =   hWnd;
            msg.message =   msgId;
            msg.wParam  =   wParam;
            msg.lParam  =   lParam;
#define GWL_USERDATA (-21)
            if (WM_CREATE == msgId)
            {
                CREATESTRUCT*   create_struct   =   (CREATESTRUCT*)lParam;
                LPVOID          lpCreateParams  =   create_struct->lpCreateParams;
                if (lpCreateParams)
                {
                    ::SetWindowLongPtr(hWnd, GWL_USERDATA, (DWORD_PTR)lpCreateParams);
                    FEWinApp*   pThis   =   (FEWinApp*)lpCreateParams;
                    return      pThis->onMessage(&msg);
                }
            }
            else
            {
                FEWinApp*   pThis   = (FEWinApp*)GetWindowLongPtr(hWnd, GWL_USERDATA);
                if (pThis)
                {
                    return      pThis->onMessage(&msg);
                }
            }
            return ::DefWindowProcA(hWnd, msgId, wParam, lParam);
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
