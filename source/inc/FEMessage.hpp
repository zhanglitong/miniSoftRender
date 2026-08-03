#pragma     once
#include    <functional>
#include    "FEMathUtil.hpp"
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
        FEMouseInfo(const int2& mouse = int2())
            :_mouse(mouse)
        {}
        int2   _mouse;
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

    using   MsgUpdate           =   TFEMessage<MSG_UPDATE           ,FEMouseInfo>;
    using   MsgRender           =   TFEMessage<MSG_RENDER           ,FEMouseInfo>;



    using   MsgNotify   =   std::function<void(const FEMessage&)>;
}