#pragma     once

#include    "../FEInput.hpp"
#include    "../FECamera.hpp"
#include    "../FETimestamp.hpp"
#include    "../FEApp.hpp"
#include    "FEFrameBuffer.h"

namespace   FE
{
    /// <summary>
    /// 可以理解为一个场景的渲染器，负责管理场景中的所有对象，处理输入事件，并与渲染系统进行交互。它继承自FEObject和FEInput类，具有对象管理和输入处理的能力。
    /// 典型应用场景: 
    /// 1.实现一个小地图
    /// 2.实现一个场景的预览窗口
    /// 3.后期效果预渲染
    /// 4.ui界面渲染
    /// 5.背景渲染
    /// 
    /// 每一个viewer 可以有自己的相机
    /// 如果没有指定，则使用系统默认的相机
    /// </summary>
    class   FEViewer 
        :public FEObject
        ,public FEInput
    {
    public:
        enum    ViewerFlag:uint32_t
        {
            FLAG_RESIZABLE      =    FlagBit::FLAG_LAST,
        };
        using   ViewerUsages    =   FEFlags<ViewerFlag, uint32_t>;    
    public:
        friend  class   FEContext;
    public:
        FEViewer(FEContext& ctx,Camera camera,FrameBuffer fbo,ViewerUsages usages)
            :FEObject(ctx)
            ,_usages(usages)
            ,_camera(camera)
            ,_fbo(fbo)
        {}
        FEViewer(const FEViewer& other)
            :FEObject(other)
            , _usages(other._usages)
            , _camera(other._camera)
            , _fbo(other._fbo)
        {}

        virtual ~FEViewer() = default;
        /// <summary>
        /// 初始化viewer,如果没有指定宽高(MaxUint32) ,则使用系统默认的宽高(一般是窗口的宽高)，如果没有窗口则使用默认的宽高(一般是1920x1080)   
        /// </summary>
        /// <param name="w"></param>
        /// <param name="h"></param>
        /// <param name="usages"></param>
        virtual bool    setup(const uint2& off,uint w = MaxUint32, uint h = MaxUint32, ViewerUsages usages = USAGE_Scene);

        virtual void    destroy();
        /// <summary>
        /// 设置视口
        /// </summary>
        /// <param name="rect"></param>
        inline  void    setViewPort(const RectU32& rect)
        {
            _rect   =   rect;
        }
        const   auto&   usages() const
        {
            return  _usages;
        }
        inline  auto&   usages()
        {
            return  _usages;
        }
        inline  auto    camera() const
        {
            return  _camera;
        } 
        inline  void    setCamera(Camera cam)
        {
            _camera =   cam;
        }
        inline  auto    fbo() const
        {
            return  _fbo;
        }
        inline  void    setFbo(FrameBuffer fbo)
        {
            _fbo    =   fbo;
        }
        /// <summary>
        /// 用作显示，调试使用，或者标记viewer的用途
        /// </summary>
        /// <returns></returns>
        const   auto&   name() const
        {
            return  _name;
        }
        inline  void    setName(const String& name)
        {
            _name   =   name;
        }
        /// <summary>
        /// 窗口客户区坐标转换成 相对于 viewer 的坐标
        /// </summary>
        inline  int2    clientToViewer(const int2& client) const
        {
            return  client + int2(_rect.left(),_rect.top());
        }
        /// <summary>
        /// 处理消息事件，通常是输入事件，例如鼠标点击、键盘输入等。
        /// 该函数会根据传入的消息类型进行相应的处理，可以调用回调函数或者修改相机状态等。
        /// </summary>
        /// <param name="msg"></param>
        virtual void    onMessage(const FEMessage& msg) override;
    protected:
        void    onUpdate(const MsgUpdate& msg);
        void    onRender(const MsgRender& msg);
        void    onResize(const MsgResize& msg);
        void    onLButtonDown(const MsgLButtonDown& msg);
        void    onRButtonDown(const MsgRButtonDown& msg);
        void    onMouseMove(const MsgMouseMove& msg);
        void    onMouseWheel(const MsgMouseWheel&);
        void    onKeyDown(const MsgKeyDown&);
        void    onKeyUp(const MsgKeyUp&);

    private:
        RectU32         _rect;
        ViewerUsages    _usages =   0;
        Camera          _camera =   nullptr;
        FrameBuffer     _fbo    =   nullptr;
        String          _name;
        aabb3dr         _aabb;   
    };
    using   Viewer =   SharedPtr<FEViewer>;
}
