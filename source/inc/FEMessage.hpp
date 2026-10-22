#pragma     once
#include    <functional>
#include    <map>
#include    <vector>
#include    <utility>
#include    "FEMathUtil.hpp"
#include    "FEFlags.hpp"
#include    "FEKeyCode.hpp"

namespace   FE
{
    class   FEViewer;
    enum    MessageId : uint32_t
    {
        MSG_CREATE          ,
        MSG_CLOSE           ,
        MSG_DESTROY         ,
        MSG_RESIZE          ,
        MSG_RESIZE_START    ,
        MSG_RESIZE_END      ,
        MSG_LBUTTON_DOWN    ,
        MSG_LBUTTON_UP      ,
        MSG_LBUTTON_DBCLICK ,
        MSG_RBUTTON_DOWN    ,
        MSG_RBUTTON_UP      ,
        MSG_RBUTTON_DBCLICK ,
        MSG_MBUTTON_DOWN    ,
        MSG_MBUTTON_UP      ,
        MSG_MBUTTON_DBCLICK ,
        MSG_MOUSE_MOVE      ,
        MSG_MOUSE_WHEEL     ,
        MSG_KEYDOWN         ,
        MSG_KEYUP           ,
        MSG_CHAR            ,
        MSG_TOUCH_DOWN      ,
        MSG_TOUCH_UP        ,
        MSG_TOUCH_MOVE      ,
        MSG_TOUCHPAD        ,
        MSG_TOUCHPAD_DOWN   ,
        MSG_TOUCHPAD_UP     ,
        MSG_TASK            ,

        MSG_UPDATE          ,
        MSG_RENDER          ,
        MSG_MAX             ,
    };
    /// <summary>
    /// 
    /// </summary>
    enum   ViewerUsage
    {     
        USAGE_Background =   (1<<0),
        USAGE_Shadow     =   (USAGE_Background)<<1,
        /// 水面
        USAGE_Water      =   (USAGE_Shadow)<< 1,
        /// bloom 
        USAGE_Bloom      =   (USAGE_Water)<< 1,
        /// 延迟渲染图层
        USAGE_Defferd    =   (USAGE_Bloom <<1),    
        /// 场景层
        USAGE_Scene      =   (USAGE_Defferd<<1),
        /// 贴画流程，在场景完成之后
        USAGE_Decal      =   (USAGE_Scene<<1),
        /// GUI
        USAGE_GUI        =   (USAGE_Decal << 1),
        /// 最上层
        USAGE_Overlay    =   (USAGE_GUI <<1),

        USAGE_Classic   =   USAGE_Background | USAGE_Scene | USAGE_Overlay | USAGE_GUI,
    };

    using   ViewerUsages =  FEFlags<ViewerUsage, uint32_t>;

    class   FEMessage
    {
    protected:
        /// <summary>
        /// 记录当前viewer,消息先到viewer,再向下分发
        /// </summary>
        FEViewer*   _viewer     =   nullptr;
    public:
        virtual ~FEMessage()
        {}
        virtual MessageId   msgId() const   =   0;
        inline  auto        viewer() const
        {
            return  _viewer;
        }
        inline  auto&       setViewer(FEViewer* viewer)
        {
            _viewer =   viewer;
            return  *this;
        }
    };

    class   FECloseInfo
    {};
    class   FECreateInfo
    {};
    class   FEDestroyInfo
    {};
    class   FEResizeInfo
    {
    public:
        FEResizeInfo(const uint2& size = uint2())
            :_size(size)
        {}
        FEResizeInfo(int w,int h)
            :_size(w,h)
        {}
        uint2   _size;
    };
    class   FEMouseInfo
    {
    public:
        enum    MouseState
        {
            LButtonPressed  =   1,
            MButtonPressed  =   2,
            RButtonPressed  =   4,
            XButton1Pressed =   8,
            XButton2Pressed =   16,
            CtrlPressed     =   32,
            AltPressed      =   64,
            ShiftPressed    =   128,
        };
        using   MouseStates     =   FEFlags<MouseState,uint32_t>;
    public:
        FEMouseInfo(const int2& mouse = int2(),MouseStates state = 0)
            :_mouse(mouse)
            ,_states(state)
        {}
        /// <summary>
        /// 如果是鼠标移动消息:
        /// _mouse 是当前消息
        /// _prev: 是上一次鼠标位置
        /// </summary>
        int2        _prev;
        int2        _mouse;
        MouseStates _states; 
        bool    lButtonPressed() const
        {
            return  _states.hasFlag(LButtonPressed);
        }
        bool    mButtonPressed() const
        {
            return  _states.hasFlag(MButtonPressed);
        }
        bool    rButtonPressed() const
        {
            return  _states.hasFlag(RButtonPressed);
        }
    };

    class   FECmdBuffer;
    class   FEFramInfo
    {
    public:
        FEFramInfo(ViewerUsages usages = 0,FECmdBuffer* cmdBuffer = nullptr)
            :_usages(usages)
            ,_cmdBuffer(cmdBuffer)
        {}
        ViewerUsages    _usages;
        FECmdBuffer*    _cmdBuffer = nullptr;  
    };

    class   FEKeyInfo
    {
    public:
        FEKeyInfo(uint key = 0)
        {
            _key    =   key;
        }
    public:
        uint    _key;
    };
    class   FEMouseWheel
    {
    public:
        
    public:
        FEMouseWheel(const int2& mouse = int2(),int zDelta = 0)
            :_mouse(mouse)
            ,_zDelta(zDelta)
        {}
        int2    _mouse;
        int     _zDelta;
    };
    template<MessageId MsgId,typename TInfo>
    class   TFEMessage : public FEMessage
    {
    public:
        TFEMessage(const TInfo& info = {})
            :_info(info)
        {}
        virtual MessageId   msgId() const 
        {
            return  MsgId;
        }
    public:
        TInfo   _info;
    };

    using   MsgCreate           =   TFEMessage<MSG_CREATE           ,FECreateInfo   >;
    using   MsgClose            =   TFEMessage<MSG_CLOSE            ,FECloseInfo    >;
    using   MsgDestroy          =   TFEMessage<MSG_DESTROY          ,FEDestroyInfo  >;
    using   MsgResize           =   TFEMessage<MSG_RESIZE           ,FEResizeInfo   >;
    using   MsgResizeStart      =   TFEMessage<MSG_RESIZE_START     ,FEResizeInfo   >;
    using   MsgResizeEnd        =   TFEMessage<MSG_RESIZE_END       ,FEResizeInfo   >;
    using   MsgLButtonDown      =   TFEMessage<MSG_LBUTTON_DOWN     ,FEMouseInfo>;
    using   MsgLButtonUp        =   TFEMessage<MSG_LBUTTON_UP       ,FEMouseInfo>;
    using   MsgLButtonDbClick   =   TFEMessage<MSG_LBUTTON_DBCLICK  ,FEMouseInfo>;
    using   MsgRButtonDown      =   TFEMessage<MSG_RBUTTON_DOWN     ,FEMouseInfo>;
    using   MsgRButtonUp        =   TFEMessage<MSG_RBUTTON_UP       ,FEMouseInfo>;
    using   MsgRButton_DbClick  =   TFEMessage<MSG_RBUTTON_DBCLICK  ,FEMouseInfo>;
    using   MsgMButtonDown      =   TFEMessage<MSG_MBUTTON_DOWN     ,FEMouseInfo>;
    using   MsgMButtonUp        =   TFEMessage<MSG_MBUTTON_UP       ,FEMouseInfo>;
    using   MsgMButtonDbClick   =   TFEMessage<MSG_MBUTTON_DBCLICK  ,FEMouseInfo>;
    using   MsgMouseMove        =   TFEMessage<MSG_MOUSE_MOVE       ,FEMouseInfo>;
    using   MsgMouseWheel       =   TFEMessage<MSG_MOUSE_WHEEL      ,FEMouseWheel>;
    using   MsgKeyDown          =   TFEMessage<MSG_KEYDOWN          ,FEKeyInfo>;
    using   MsgKeyUp            =   TFEMessage<MSG_KEYUP            ,FEKeyInfo>;
    using   MsgChar             =   TFEMessage<MSG_CHAR             ,FEMouseInfo>;
    using   MsgTouchDowns       =   TFEMessage<MSG_TOUCH_DOWN       ,FEMouseInfo>;
    using   MsgTouchUp          =   TFEMessage<MSG_TOUCH_UP         ,FEMouseInfo>;
    using   MsgTouchMove        =   TFEMessage<MSG_TOUCH_MOVE       ,FEMouseInfo>;
    using   MsgTouchpad         =   TFEMessage<MSG_TOUCHPAD         ,FEMouseInfo>;
    using   MsgTouchpadDown     =   TFEMessage<MSG_TOUCHPAD_DOWN    ,FEMouseInfo>;
    using   MsgTouchpadUp       =   TFEMessage<MSG_TOUCHPAD_UP      ,FEMouseInfo>;
    using   MsgTask             =   TFEMessage<MSG_TASK             ,FEMouseInfo>;

    using   MsgUpdate           =   TFEMessage<MSG_UPDATE           ,FEFramInfo>;
    using   MsgRender           =   TFEMessage<MSG_RENDER           ,FEFramInfo>;



    using   MsgNotify           =   std::function<void(const FEMessage&)>;

    /// <summary>
    /// 按消息类型的多代理通知总线:
    /// 订阅者只注册自己关心的 MessageId,同一类型可挂多个代理(按注册顺序派发)。
    /// FEApp::onMessage 在默认处理(_notify / 场景)之前派发总线——
    /// 订阅 MSG_CLOSE 可抢在 FEScene::destroy() 销毁 device 之前,释放
    /// 场景之外持有的 GPU 资源(材质/纹理/缓冲等),避免悬空 device 访问。
    /// 消息均在主线程派发;dispatch 时拷贝订阅列表,处理中增删订阅不影响本次派发。
    /// </summary>
    class   FEMessageBus
    {
    public:
        using   Handler     =   std::function<void(const FEMessage&)>;
        /// <summary>
        /// 订阅 id:高 32 位为消息类型,低 32 位为该类型内的自增序号——
        /// 取消订阅时先按消息类型定位分组,再在组内按序号查找
        /// </summary>
        using   HandlerId   =   uint64;
        /// <summary>
        /// 同一消息类型的订阅列表(按注册顺序)
        /// </summary>
        using   Handlers    =   std::vector<std::pair<HandlerId,Handler>>;
        /// <summary>
        /// 消息类型 -> 订阅列表
        /// </summary>
        using   HandlerMap  =   std::map<MessageId,Handlers>;
        /// <summary>
        /// 消息类型 -> 下一个自增序号
        /// </summary>
        using   SeqMap      =   std::map<MessageId,uint32>;
    public:
        /// <summary>
        /// 合成订阅 id:高 32 位消息类型,低 32 位类型内自增序号
        /// </summary>
        static  constexpr   HandlerId   makeId(MessageId id,uint32 seq)
        {
            return  (static_cast<HandlerId>(id) << 32)  |   seq;
        }
        /// <summary>
        /// 从订阅 id 解出消息类型
        /// </summary>
        static  constexpr   MessageId   msgIdOf(HandlerId id)
        {
            return  static_cast<MessageId>(id >> 32);
        }
    public:
        /// <summary>
        /// 订阅:同一消息类型可多次订阅(多代理),按注册顺序派发
        /// </summary>
        /// <param name="id">订阅的消息类型</param>
        /// <param name="handler">消息处理代理</param>
        /// <returns>订阅 id(消息类型+自增号合成),用于 unsubscribe</returns>
        HandlerId   subscribe(MessageId id,Handler handler)
        {
            uint32& seq =   _nextSeq[id];
            seq++;
            _handlers[id].push_back({makeId(id,seq),std::move(handler)});
            return  makeId(id,seq);
        }
        /// <summary>
        /// 取消订阅:先按消息类型定位分组,再在组内按订阅 id 查找
        /// </summary>
        /// <param name="id">subscribe 返回的订阅 id</param>
        /// <returns>是否成功取消订阅</returns>
        bool    unsubscribe(HandlerId id)
        {
            auto    it  =   _handlers.find(msgIdOf(id));
            if (it == _handlers.end())
                return  false;
            for (auto    i   =   it->second.begin();i != it->second.end();++i)
            {
                if (i->first == id)
                {
                    it->second.erase(i);
                    return  true;
                }
            }
            return  false;
        }
        /// <summary>
        /// 派发:按注册顺序调用该消息类型的全部订阅者
        /// </summary>
        /// <param name="msg"></param>
        void    dispatch(const FEMessage& msg)
        {
            auto    it  =   _handlers.find(msg.msgId());
            if (it == _handlers.end() || it->second.empty())
                return;
            auto    handlers    =   it->second;     /// 拷贝,防处理中增删订阅
            for (auto& h : handlers)
                h.second(msg);
        }
    protected:
        HandlerMap  _handlers;
        SeqMap      _nextSeq;
    };
}
