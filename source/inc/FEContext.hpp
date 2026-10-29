#pragma     once
#include    "FELog.hpp"
#include    "FEKeyValues.hpp"
#include    "FEFileFormat.hpp"
#include    "FEAllocator.hpp"
#include    "FEObject.h"
#include    "FEWindow.hpp"
#include    "FEAnchor.hpp"
#include    "FESetting.hpp"


namespace FE
{
    /// <summary>
    /// File format reader
    /// </summary>
    using   FFReader            =   FEKeyValues<String,FEFileFormat>;
    /// <summary>
    /// File format writer
    /// </summary>
    using   FFWriter            =   FEKeyValues<String,FEFileFormat>;
    using   FECreators          =   FEKeyValues<FEUuid,FECreator>;

    #define LOG_DBG(fmt, ...)   _ctx.log().debug(fmt, ##__VA_ARGS__)
    #define LOG_EVT(fmt, ...)   _ctx.log().event(fmt, ##__VA_ARGS__)
    #define LOG_INF(fmt, ...)   _ctx.log().infor(fmt, ##__VA_ARGS__)
    #define LOG_WAR(fmt, ...)   _ctx.log().warning(fmt, ##__VA_ARGS__)
    #define LOG_ERR(fmt, ...)   _ctx.log().error(fmt, ##__VA_ARGS__)
    #define LOG_EXC(fmt, ...)   _ctx.log().exc(fmt, ##__VA_ARGS__)

    namespace   fs  =   std::filesystem;

    class   FEDevice;
    class   FECamera;
    class   FEWindow;
    class   FEViewer;
    class   FE_API  FEContext
    {
    public:
        friend  class   FEScene;
    protected:
        FEContext();
        ~FEContext();
    public:
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        FELog&      log()
        {
            assert(_log!=nullptr);
            return  *_log;
        }
        /// <summary>
        /// 全局配置
        /// </summary>
        auto&       setting()
        {
            return  _setting;
        }
        auto&       setting() const
        {
            return  _setting;
        }
        /// <summary>
        /// 设置工作目录，一般是跟exe同一个目录
        /// </summary>
        /// <param name="path"></param>
        /// <returns></returns>
        auto&       setWorkPath(const String& path)
        {
            fs::path    abs =   std::filesystem::weakly_canonical(path);
            _workPath       =   abs.string();
            return  *this;
        }
        /// <summary>
        /// 资源目录，一般是exe/../
        /// </summary>
        /// <param name="path"></param>
        /// <returns></returns>
        auto&       setResourcePath(const String& path)
        {
            fs::path    abs = std::filesystem::weakly_canonical(path);
            _resourcePath   =   abs.string();
            return  *this;
        }
        /// <summary>
        /// 当前工作路径
        /// </summary>
        /// <returns></returns>
        String      workPath() const
        {
            return  _workPath;
        }
        /// <summary>
        /// 资源路径根目录
        /// </summary>
        /// <returns></returns>
        String      resourcePath() const
        {
            return  _resourcePath;
        }
        FEDevice&   device();
        
        FEScene*    scene();
        /// <summary>
        /// 获取当前活动的摄像机
        /// </summary>
        /// <returns></returns>
        FECamera&   activeCamera();
        /// <summary>
        /// 请求下一帧渲染
        /// </summary>
        void        requireNextFrame();
        /// <summary>
        /// 获取当前摄像机的投影视图矩阵
        /// </summary>
        /// <returns></returns>
        mat4r       mvp();

        FEAnchor&   anchor()
        {
            return  *_anchor->as<FEAnchor>();
        }
        const FEAnchor&   anchor() const
        {
            return  *_anchor->as<FEAnchor>();
        }
        /// <summary>
        /// 如果有窗口系统，返回窗口的宽度和高度
        /// 如果没有返回UintMax32
        /// </summary>
        /// <returns></returns>
        uint32      windowsWidth()  const;
        uint32      windowsHeight() const;
        void        setWindow(FEWindow* window)
        {
            _window =   window;
        }
        /// <summary>
        /// 平台窗口(未创建时为空),用于取原生窗口句柄等
        /// </summary>
        FEWindow*   window()
        {
            return  _window ? _window->as<FEWindow>() : nullptr;
        }
        /// <summary>
        /// 获取一帧花费的时间
        /// </summary>
        /// <returns></returns>
        real        deltaTime() const
        {
            return  _deltaTime;
        }
        /// <summary>
        /// 设置帧时间
        /// </summary>
        /// <param name="tm"></param>
        /// <returns></returns>
        auto&       setDeltaTime(const real& tm)
        {
            _deltaTime  =   tm;
            return  *this;
        }
        /// <summary>
        /// 单调递增帧序号(每帧 onFrameStart 递增):供材质等按帧缓存/节流查询使用
        /// </summary>
        uint64      frameIndex() const
        {
            return  _frameIndex;
        }
        /// <summary>
        /// 帧序号递增(FEScene::onFrameStart 每帧调用)
        /// </summary>
        void        nextFrame()
        {
            ++_frameIndex;
        }

        void    destroy();
    public:
        static  FEContext&  instance();
        static  FFReader&   readers();
        static  FFWriter&   writers();
        static  FECreators& creators();
    protected:
        /// <summary>
        /// 一帧花费的时间，第一帧肯定是0，
        /// </summary>
        real        _deltaTime  =   0;
        /// <summary>
        /// 单调递增帧序号
        /// </summary>
        uint64      _frameIndex =   0;
        LogPtr      _log;
        FESetting   _setting;
        Object      _device;
        Object      _anchor;
        Object      _window;   
        Object      _scene;
        String      _workPath;
        String      _resourcePath;
    };
}


