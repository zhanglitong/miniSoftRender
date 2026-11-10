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
    class   FENodeTree;
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
        /// 设置命令行参数
        /// </summary>
        /// <param name="argc"></param>
        /// <param name="argv"></param>
        void       setArg(int argc, char** argv)
        {
            _args.clear();
            for (int i = 0; i < argc; i++)
            {
                _args.push_back(argv[i]);
            }
            /// 参数就绪后按规则加载与应用程序同名的配置(如 test.scene.cfg)
            loadSetting();
        }
        /// <summary>
        /// 获取参数列表,用于解析命令行参数
        /// </summary>
        /// <returns></returns>
        auto&       args() const
        {
            return  _args;
        }
        const auto& settingPath() const
        {
            return  _settingPath;
        }
        /// <summary>
        /// 获取日志对象,用于输出调试/运行时信息
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
        /// <summary>
        /// 以资源根目录(resourcePath)为基准,把相对路径解析成绝对路径;
        /// 传入的已是绝对路径时直接规范化。
        /// weakly_canonical 不要求目标存在,所以引用未生成的文件也能得到完整路径
        /// </summary>
        /// <param name="relativePath">相对资源根目录的路径,如 "data/fonts/consola.ttf"</param>
        /// <returns>规范化后的绝对路径</returns>
        String      absolutePath(const String& relativePath) const
        {
            fs::path    rel(relativePath);
            fs::path    abs =   rel.is_absolute()
                                ?   rel
                                :   fs::path(_resourcePath) / rel;
            return  fs::weakly_canonical(abs).string();
        }
        FEDevice&   device();
        
        FEScene*    scene();
        /// <summary>
        /// 节点树:与 FEScene 平级,负责数据(节点层级/增删),
        /// FEScene 只负责显示;两者可以独立使用,需要显示的数据再由外部加入场景
        /// </summary>
        /// <returns></returns>
        FENodeTree& nodeTree();
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
        /// 按应用同名规则加载全局配置,setArg 之后调用
        /// </summary>
        void        loadSetting();
        /// <summary>
        /// 把全局配置写回应用同名文件,destroy 时调用
        /// </summary>
        void        saveSetting();
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
        /// <summary>
        /// 全局配置的落盘路径,空表示未启用持久化
        /// </summary>
        String      _settingPath;
        Object      _device;
        Object      _anchor;
        Object      _window;   
        Object      _scene;
        /// <summary>
        /// 节点树(数据侧),与 _scene 平级
        /// </summary>
        Object      _nodeTree;
        String      _workPath;
        String      _resourcePath;
        /// <summary>
        /// 应用程序启动时的命令行参数,第一个参数是程序本身路径
        /// </summary>
        Strings     _args;
    };
}


