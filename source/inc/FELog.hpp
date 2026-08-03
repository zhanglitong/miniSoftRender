#pragma     once
#include    <stdarg.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdint.h>
#include    <assert.h>
#include    <functional>
#include    <filesystem>

#include    "FEDefine.h"
#include    "FEDateTime.hpp"
#include    "FEMutex.hpp"
#include    "FEObject.h"

namespace   FE
{
    /// <summary>
    /// FELog log;
    /// log.open("xxx.xml");
    /// log.error("error message,code: %d",100) == log.write("error", "message,code: 100");
    /// log.write("error", "message data");
    /// ...
    /// log.close();
    /// </summary>
    class   FELog :public FEObject
    {
    public:
        using   uint    =   uint32_t;
        /// <summary>
        /// 输出格式定义
        /// </summary>
        static  constexpr   const char* LOG_FORMAT      =   "[%-10s],[%-8s],[%-24s],log=%s\n";
        /// <summary>
        /// 默认的模块名称定义
        /// </summary>
        static  constexpr   const char* DEF_MODULE_NAME =   "default";  
    public:
        /// <summary>
        /// 写日志之前回调通知
        /// 第一个参数: 模块，可能为nullptr
        /// 第二个参数: 类型
        /// 第三个参数: 日志内容
        /// 第四个参数: 时间
        /// </summary>
        using   NotifyEvent =   std::function<void(const char*,const char* ,const char*,const FEDateTime&  )>;
    public:
        enum    LEVEL :uint32_t
        {
            /// 适合开发用，用来记录函数调用的事件
            LEVEL_DEBUG     =   0,
            /// 事件，例如鼠标，键盘，用户的动作
            LEVEL_EVENT     =   1,
            /// 通知日志，记录一些系统发生的事情
            LEVEL_INFOR     =   2,
            /// 警告信息
            LEVEL_WARNING   =   3,
            /// 错误信息
            LEVEL_ERROR     =   4,
            /// 异常信息
            LEVEL_EXCEPTION =   5,
            /// 最大支持级别
            LEVEL_MAX       =   6,
        };
        enum    LevelBit : uint32_t
        {
            /// <summary>
            /// 默认写日志控制位
            /// </summary>
            LOG_Bits    =     (1 << LEVEL_EVENT)
                            | (1 << LEVEL_INFOR)
                            | (1 << LEVEL_WARNING)
                            | (1 << LEVEL_ERROR)
                            | (1 << LEVEL_EXCEPTION),
            /// <summary>
            /// 调试输出控制位
            /// </summary>
            DBG_Bits    =     (1 << LEVEL_DEBUG)
                            | (1 << LEVEL_EVENT)
                            | (1 << LEVEL_INFOR)
                            | (1 << LEVEL_WARNING)
                            | (1 << LEVEL_ERROR)
                            | (1 << LEVEL_EXCEPTION),
        };
        static  auto    nameOfEnum(LEVEL level)
        {
            switch (level)
            {
            case FELog::LEVEL_DEBUG:        return  "debug";
            case FELog::LEVEL_EVENT:        return  "event";
            case FELog::LEVEL_INFOR:        return  "infor";
            case FELog::LEVEL_WARNING:      return  "warning";
            case FELog::LEVEL_ERROR:        return  "error";
            case FELog::LEVEL_EXCEPTION:    return  "exception";
            default:                        return  "infor";
            }
        }
        static  auto    enumFromName(const char* name)
        {
            if (_stricmp(nameOfEnum(LEVEL_DEBUG),name) == 0)
                return  LEVEL_DEBUG;
            else if (_stricmp(nameOfEnum(LEVEL_EVENT),name) == 0)
                return  LEVEL_EVENT;
            else if (_stricmp(nameOfEnum(LEVEL_INFOR),name) == 0)
                return  LEVEL_INFOR;
            else if (_stricmp(nameOfEnum(LEVEL_WARNING),name) == 0)
                return  LEVEL_WARNING;
            else if (_stricmp(nameOfEnum(LEVEL_ERROR),name) == 0)
                return  LEVEL_ERROR;
            else if (_stricmp(nameOfEnum(LEVEL_EXCEPTION),name) == 0)
                return  LEVEL_EXCEPTION;
            else
                return  LEVEL_INFOR;
        }

        class   Module
        {
        public:
            FELog&          _log;
            char            _name[64];
            /// <summary>
            /// 控制哪些数据被写入到日志文件中
            /// </summary>
            uint            _logBits    =   LOG_Bits;
            /// <summary>
            /// 控制哪些会输出到控制台
            /// </summary>
            uint            _dbgBits    =   DBG_Bits;
        public:
            Module(FELog& log,const char* name)
                :_log(log)                
            {
                if (name)
                    strncpy_s(_name,name,sizeof(_name) - 1);
                else
                    strncpy_s(_name,"user",sizeof(_name) - 1); 
            }
            inline  auto&   setDebugBits(uint bits)
            {
                _dbgBits    =   bits;
                return  *this;
            }
            inline  auto&   setLogBits(uint bits)
            {
                _logBits    =   bits;
                return  *this;
            }
            /// <summary>
            /// 事件日志,记录所有操作信息,用来调试
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   debug(const char*  format,Args&&... args)
            {
                return  log(LEVEL_DEBUG,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 事件日志,记录所有操作信息,用来调试
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   event(const char*  format,Args&&... args)
            {
                return  log(LEVEL_EVENT,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 写信息输出,一些重要的信息
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   infor(const char*  format,Args&&... args)
            {
                return  log(LEVEL_INFOR,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 写警告信息
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   warning(const char*  format,Args&&... args)
            {
                return  log(LEVEL_WARNING,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 写错误信息
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   error(const char*  format,Args&&... args)
            {
                return  log(LEVEL_ERROR,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 写异常日志
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   exception(const char*  format,Args&&... args)
            {
                return  log(LEVEL_EXCEPTION,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 写异常日志
            /// </summary>
            /// <param name="format">格式化字符串</param>
            /// <returns>返回写入状态</returns>
            template<typename... Args>
            inline  auto&   exc(const char*  format,Args&&... args)
            {
                return  log(LEVEL_EXCEPTION,format,std::forward<Args>(args)...);
            }
            /// <summary>
            /// 通用接口，可以自定义日志类型
            /// </summary>
            /// <param name="type"></param>
            /// <param name="format"></param>
            /// <param name=""></param>
            /// <returns></returns>
            template<typename... Args>
            inline  auto&   log(LEVEL lev,const char*  format,Args&&... args)
            {
                _log.write(_name,lev,_logBits,_dbgBits,format, std::forward<Args>(args)...);
                return   *this;
            }
        };
    protected:
        /// <summary>
        /// 通知，应用层可以截获日志事件
        /// 典型应用场景，在界面上输出日志
        /// </summary>
        NotifyEvent     _notify;
        /// <summary>
        /// 用来实时输出到调试器窗口中
        /// </summary>
        NotifyEvent     _notifyDebug;
        /// <summary>
        /// 控制哪些数据被写入到日志文件中
        /// </summary>
        uint            _logBits    =   LOG_Bits;
        /// <summary>
        /// 控制哪些会输出到控制台
        /// </summary>
        uint            _dbgBits    =   DBG_Bits;
        FEMutex         _mutex;
        FILE*           _file       =   nullptr;
    public:
        FELog(FEContext& ctx,const char* fileName = nullptr)
            :FEObject(ctx)
        {
            if (fileName)
            {
                open(fileName);
            }
            /// 第一个参数: 模块，可能为nullptr
            /// 第二个参数: 类型
            /// 第三个参数: 日志内容
            /// 第四个参数: 时间
            _notify     =   [](const char* moduleName,const char* type ,const char* log,const FEDateTime&)
            {
                (void)moduleName;
                (void)type;
                (void)log;
            };
            /// 默认关联打印到控制台
            _notifyDebug=   [](const char* moduleName,const char* type ,const char* log,const FEDateTime& dateTime)
            {
                char    szTime[128] =   {0};
                dateTime.toDateTimeString(szTime);
                printf( LOG_FORMAT
                        , moduleName ? moduleName : DEF_MODULE_NAME
                        , type
                        , szTime
                        , log);
            };
        }
        virtual ~FELog()
        {}
        /// <summary>
        /// 设置调试数据输出控制位,控制数据是否输出到控制台
        /// log().setDebugBits(1<<LEVEL_DEBUG).debug("输出debug信息);
        /// </summary>
        /// <param name="bits"></param>
        /// <returns></returns>
        inline  auto&   setDebugBits(uint bits)
        {
            _dbgBits    =   bits;
            return  *this;
        }
        /// <summary>
        /// 获取调试输出控制位
        /// </summary>
        /// <returns></returns>
        inline  uint    debugBits() const
        {
            return  _dbgBits;
        }
        /// <summary>
        /// 设置日志输出控制位置，控制输出是否输出到日志
        /// </summary>
        /// <param name="bits"></param>
        /// <returns></returns>
        inline  auto&   setLogBits(uint bits)
        {
            _logBits    =   bits;
            return  *this;
        }
        /// <summary>
        /// 获取日志控制位
        /// </summary>
        /// <returns></returns>
        inline  uint    logBits() const
        {
            return  _logBits;
        }
        /// <summary>
        /// 获取模块，增加日志字段.
        /// 使用方式  log().module("server").infor("start server %s",ipaddr.c_str());
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        inline  Module  module(const char* name = "default")
        {
            return  Module(*this,name);
        }
        /// <summary>
        /// 返回值写日志通知回调函数
        /// 例如用来实时显示日志信息到界面上
        /// </summary>
        /// <returns>日志回调事件</returns>
        inline  auto&   notify()
        {
            return  _notify;
        }
        /// <summary>
        /// 返回值写日志通知回调函数(只读)
        /// 例如用来实时显示日志信息到界面上
        /// </summary>
        /// <returns>日志回调事件</returns>
        const   auto&   notify() const
        {
            return      _notify;
        }
        /// <summary>
        /// 用来输出到调试器
        /// </summary>
        /// <returns></returns>
        inline  auto&   notifyDebug()
        {
            return  _notifyDebug;
        }
        /// <summary>
        /// 用来输出到调试器(只读)
        /// </summary>
        /// <returns></returns>
        const   auto&   notifyDebug() const
        {
            return      _notifyDebug;
        }
        /// <summary>
        /// 控制日志的写到文件中的级别,如果是true输出，否则不输出，即使调用了函数也不做任何动作
        /// </summary>
        /// <param name="lev">级别</param>
        /// <param name="openFlag">是否打开</param>
        inline  auto&   setOption(LEVEL lev,bool openFlag)
        {
            if (openFlag)
                _logBits    |=  1<<(uint32_t)lev;
            else
                _logBits    &=  ~(1<<(uint32_t)lev);
            return  *this;
        }
        /// <summary>
        /// 如果是true输出，说明该级别日志可以输出，否则不输出
        /// </summary>
        /// <param name="lev"></param>
        /// <returns></returns>
        inline  bool    getOption(LEVEL lev) const
        {
            return  (_logBits  & (1<<(uint32_t)lev)) ? true : false;
        }
        static  bool    getOption(uint bits,LEVEL lev)
        {
            return  (bits  & (1<<(uint32_t)lev)) ? true : false;
        }
        /// <summary>
        /// 启用指定级别的日志(写日志)
        /// </summary>
        /// <param name="lev">日志级别</param>
        inline  auto&   enable(LEVEL lev)
        {
            return  setOption(lev,true);
        }
        /// <summary>
        /// 禁用指定级别的日志(不写日志)
        /// </summary>
        /// <param name="lev">日志级别</param>
        inline  auto&   disable(LEVEL lev)
        {
            return  setOption(lev,false);
        }
        /// <summary>
        /// 指定级别的日志是否启用(查询接口)
        /// </summary>
        /// <param name="lev">日志级别</param>
        /// <returns>true,启用 false 未启用</returns>
        inline  bool    isEnable(LEVEL lev) const
        {
            return  getOption(lev);
        }

        template<typename... Args>
        inline  FELog&  debug(const char*  format,Args&&... args)
        {
            return  log(LEVEL_DEBUG,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 事件日志,记录所有操作信息,用来调试
        /// </summary>
        /// <param name="format">格式化字符串</param>
        /// <returns>返回写入状态</returns>
        template<typename... Args>
        inline  FELog&  event(const char*  format,Args&&... args)
        {
            return  log(LEVEL_EVENT,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 写信息输出,一些重要的信息
        /// </summary>
        /// <param name="format">格式化字符串</param>
        /// <returns>返回写入状态</returns>
        template<typename... Args>
        inline  FELog&  infor(const char*  format,Args&&... args)
        {
            return  log(LEVEL_INFOR,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 写警告信息
        /// </summary>
        /// <param name="format">格式化字符串</param>
        /// <returns>返回写入状态</returns>
        template<typename... Args>
        inline  FELog&  warning(const char*  format,Args&&... args)
        {
            return  log(LEVEL_WARNING,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 写错误信息
        /// </summary>
        /// <param name="format">格式化字符串</param>
        /// <returns>返回写入状态</returns>
        template<typename... Args>
        inline  FELog&  error(const char*  format,Args&&... args)
        {
            return  log(LEVEL_ERROR,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 写异常日志
        /// </summary>
        /// <param name="format">格式化字符串</param>
        /// <returns>返回写入状态</returns>
        template<typename... Args>
        inline  FELog&  exception(const char*  format,Args&&... args)
        {
            return  log(LEVEL_EXCEPTION,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 写异常日志
        /// </summary>
        /// <param name="format">格式化字符串</param>
        /// <returns>返回写入状态</returns>
        template<typename... Args>
        inline  FELog&  exc(const char*  format,Args&&... args)
        {
            return  log(LEVEL_EXCEPTION,format,std::forward<Args>(args)...);
        }
        /// <summary>
        /// 通用接口，可以自定义日志类型
        /// </summary>
        /// <param name="type"></param>
        /// <param name="format"></param>
        /// <param name=""></param>
        /// <returns></returns>
        template<typename... Args>
        inline  FELog&  log(LEVEL lev,const char*  format,Args&&... args)
        {
            write(DEF_MODULE_NAME,lev,_logBits,_dbgBits, format,std::forward<Args>(args)...);
            return   *this;
        }
    public:
        /// <summary>
        /// 打开日志文件: d:/data/log.txt"
        /// </summary>
        /// <returns>是否打开成功</returns>
        virtual bool    open(const char* fileName) 
        {
            FEMutex::ScopeLock  lk(_mutex);
            if (_file)
            {
                fclose(_file);
            }
            _file   =   fopen(fileName,"wt");
            return  _file != nullptr;
        }
        /// <summary>
        /// 日志系统状态是否是打开
        /// </summary>
        /// <returns>true:false</returns>
        virtual bool    isOpen() const
        {
            return  _file != nullptr;
        }
        /// <summary>
        /// 关闭日志
        /// </summary>
        virtual void    close()
        {
            FEMutex::ScopeLock  lk(_mutex);
            if (_file)
            {
                fclose(_file);
                _file   =   nullptr;
            }
        }
        /// <summary>
        /// 自定义写日志信息,不受LEVEL控制
        /// </summary>
        /// <param name="module">例如 "player","plugin.format.vue",每一个动态库可以定义成一个模块 </param>
        /// <param name="type">例如,debug,event,infor,warning,error,exception,也可以是任意字符串</param>
        /// <param name="data">数据</param>
        /// <returns>返回写入状态</returns>
        virtual bool    write(const char* module,const char* type,const char* data) 
        {
            FEMutex::ScopeLock lk(_mutex);
            FEDateTime    dt;
            if (!isOpen())
                return  false;
            char    szTime[128] =   {0};
            dt.toDateTimeString(szTime);
            auto    result  =   fprintf(_file, LOG_FORMAT,module ? module : DEF_MODULE_NAME, type, szTime, data);
            fflush(_file);
            return  result >= 0;
        }
    protected:
        /// <summary>
        /// 自定义写日志信息,不受LEVEL控制
        /// </summary>
        /// <param name="module">例如 "player","plugin.format.vue",每一个动态库可以定义成一个模块 </param>
        /// <param name="level">例如,debug,event,infor,warning,error,exception,也可以是任意字符串</param>
        /// <param name="format">格式字符串</param>
        /// <param name=""></param>
        /// <returns>返回写入状态</returns>
        template<class ...Args>
        inline  bool    write(const char* module,LEVEL level,uint logBits,uint dbgBits,const char*  format,Args&& ... args)
        {
            auto        type            =   nameOfEnum(level);
            auto        len             =   std::snprintf(nullptr, 0, format, std::forward<Args>(args)...);
            char        szTemp[1024]    =   {0};
            FEDateTime  dt;
            if (len < sizeof(szTemp))
            {
                std::snprintf(szTemp, sizeof(szTemp), format, std::forward<Args>(args)...);
                if (_notify)
                    _notify(module,type,szTemp,dt);
                if (_notifyDebug && getOption(dbgBits,level))
                    _notifyDebug(module,type,szTemp,dt);
                if (level < LEVEL_MAX)
                {
                    if(!getOption(logBits,level))
                        return  false;
                    else
                        return  write(module,type,  szTemp);
                }
                else
                {
                    return  write(module,"user",szTemp);
                } 
            }
            else
            {
                std::unique_ptr<char[]> buffer(new char[len + 1]);
                std::snprintf(buffer.get(), len + 1, format, std::forward<Args>(args)...);
                if (_notify)
                    _notify(module,type,buffer.get(),dt);
                if (_notifyDebug && getOption(dbgBits,level))
                    _notifyDebug(module,type,buffer.get(),dt);
                if (level < LEVEL_MAX)
                {
                    if(!getOption(logBits,level))
                        return  false;
                    else
                        return  write(module,type,  buffer.get());
                }
                else
                {
                    return  write(module,"user",buffer.get());
                } 
            }
        }
    };

    using   LogPtr  =   SharedPtr<FELog>;
}

