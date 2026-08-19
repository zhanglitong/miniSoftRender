#pragma     once
#include    <functional>
#include    "FEMathUtil.hpp"
#include    "FEFlags.hpp"
namespace   FE
{
    
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
    public:
        virtual ~FEMessage()
        {}
        virtual MessageId   msgId() const   =   0;
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
        /// _old: 是上一次鼠标位置
        /// </summary>
        int2        _mouse;
        int2        _old;
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



    using   MsgNotify   =   std::function<void(const FEMessage&)>;
}
