#pragma     once
#include    "FELog.hpp"
#include    "FEKeyValues.hpp"
#include    "FEFileFormat.hpp"
#include    "FEAllocator.hpp"
#include    "FEObject.h"
#include    "FENotify.hpp"
#include    "FEWindow.hpp"
#include    "FEAnchor.hpp"


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
    class   FEScene;
    class   FEWindow;
    using   Device  =   SharedPtr<FEDevice>;
    using   Scene   =   SharedPtr<FEScene>;
    using   Window  =   SharedPtr<FEWindow>;
    class   FEContext
    {
    public:
        friend  class   FEScene;
    public:
        FEContext();
        ~FEContext();
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
        FEDevice&   device()
        {
            return  *_device;
        }
        
        FEScene*    scene()
        {
            return  _scene.get();
        }

        FEAnchor&   anchor()
        {
            return  *_anchor;
        }
        const FEAnchor&   anchor() const
        {
            return  *_anchor;
        }
        /// <summary>
        /// 如果有窗口系统，返回窗口的宽度和高度
        /// 如果没有返回UintMax32
        /// </summary>
        /// <returns></returns>
        uint32      windowsWidth()  const
        {
            return  _window ? _window->width() : MaxUint32;
        }
        uint32      windowsHeight() const
        {
            return  _window ? _window->height() : MaxUint32;
        }
        void        setWindow(Window window)
        {
            _window =   window;
        }
    public:
        static  FFReader&   readers();
        static  FFWriter&   writers();
        static  FECreators& creators();
    protected:
        LogPtr      _log;
        Device      _device;
        Anchor      _anchor;
        Window      _window;    
        Scene       _scene;
        String      _workPath;
        String      _resourcePath;
    };
}


