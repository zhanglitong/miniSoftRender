#pragma     once

#include    "FEObject.h"
#include    "FENotify.hpp"
#include    "FEMath.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FECamera,"{D1810D63-8CC2-4272-BBFE-FE21BB7DAFF2}");

    class   FEContext;
    class   FECamera :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FECamera)
    public:
        using   CameraPtr   =   SharedPtr<FECamera>;
        using   ChangeEvent =   std::function<void(FECamera&)>;
    public:
        enum    CameraOption
        {
            FLAG_MOV_X  =   1<<0,
            FLAG_MOV_Y  =   1<<1,
            FLAG_MOV_Z  =   1<<2,
            FLAG_ROT_X  =   1<<3,
            FLAG_ROT_Y  =   1<<4,
            FLAG_ROT_Z  =   1<<5,
        };
        using   Flags   =   FEFlags<CameraOption,uint>;
    public:
        /// <summary>
        /// 基础信息
        /// </summary>
        real3       _eye;
        real3       _target;
        real3       _up;
        real3       _right;
        real3       _dir;
        real2       _viewSize;
        mat4r       _matView;
        mat4r       _matProj;
        
        ///正交投影模式使用
        real        _scaler;
        /// <summary>
        ///  相机速度
        /// </summary>
        real        _speed;
        real        _fov;
        real        _zNear;
        real        _zFar;
        /// 保存操作摄像机的一些状态信息
        Flags       _flag;
        ///是否正交投影
        bool        _isOrtho    =   false;
        bool        _freeze     =   false;  
        real        _rLeft      =   0;
        real        _rRight     =   0;
        real        _rTop       =   0;
        real        _rBottom    =   0;
        char        _name[FE_NAME_LENGTH] =   {0};
    public:
        /// <summary>
        /// 构造相机
        /// </summary>
        /// <param name="target">摄像机目标点位置</param>
        /// <param name="eye">摄像机眼睛位置</param>
        /// <param name="right">摄像机right方向</param>
        FECamera(FEContext& ctx,const real3& target = real3(0, 0, 0), const real3& eye = real3(0, 100, 100), const real3& right = real3(1, 0, 0));
        /// <summary>
        /// 析构
        /// </summary>
        ~FECamera();

        /// <summary>
        /// 设置相机的名称
        /// </summary>
        /// <param name="name"></param>
        inline  void    setName(const char* name)
        {
            strncpy(_name,name,sizeof(_name)-1);
        }
        /// <summary>
        /// 获取相机名称
        /// </summary>
        /// <returns></returns>
        inline  PCSTR   name() const
        {
            return  _name;
        }
        /// <summary>
        /// 像机是否支持修改
        /// </summary>
        /// <returns></returns>
        inline  bool    freeze() const
        {
            return  _freeze;
        }
        /// <summary>
        /// 像机是否支持修改
        /// </summary>
        /// <param name="flag"></param>
        inline  void    setFreeze(bool flag)
        {
            _freeze =   flag;
        }
        /// <summary>
        /// 设置为enable状态
        /// </summary>
        /// <param name="state"></param>
        inline  void    enable(CameraOption state)
        {
            _flag.addFlag(state);
        }
        /// <summary>
        /// 设置为disable状态
        /// </summary>
        /// <param name="state"></param>
        inline  void    disable(CameraOption state)
        {
            _flag.removeFlag(state);
        }
        inline  Flags   flags() const
        {
            return  _flag;
        }
        inline  Flags&  flags() 
        {
            return  _flag;
        }
        /// <summary>
        /// 获取摄像机眼睛位置
        /// </summary>
        /// <returns>摄像机眼睛位置</returns>
        const   real3&  getEye() const 
        { 
            return _eye;
        }
        /// <summary>
        /// 设置摄像机眼睛位置
        /// </summary>
        /// <param name="eye">摄像机眼睛位置</param>
        inline void     setEye(const real3& eye)
        { 
            _eye = eye;
        }
        /// <summary>
        /// 获取摄像机目标位置
        /// </summary>
        /// <returns>摄像机目标位置</returns>
        const   real3&  getTarget() const 
        { 
            return _target;
        }
        /// <summary>
        /// 设置摄像机目标位置
        /// </summary>
        /// <param name="target">摄像机目标位置</param>
        inline  void    setTarget(const real3& target) 
        { 
            _target = target;
        }
        /// <summary>
        /// 获取摄像机Right方向
        /// </summary>
        /// <returns>摄像机Right方向</returns>
        const   real3&  getRight() const
        {
            return  _right;
        }
        /// <summary>
        /// 设置摄像机Right方向
        /// </summary>
        /// <param name="right">摄像机Right方向</param>
        inline  void    setRight(const real3& right)
        {
            _right = right;
        }
        /// <summary>
        ///  获取摄像机up方向
        /// </summary>
        /// <returns>摄像机up方向</returns>
        const   real3&  getUp() const 
        { 
            return _up;
        }
        /// <summary>
        /// 设置摄像机up方向
        /// </summary>
        /// <param name="up">摄像机up方向</param>
        inline  void    setUp(const real3& up)
        {
            _up = up;
        }
        /// <summary>
        ///  获取摄像机dir方向
        /// </summary>
        /// <returns>摄像机dir方向</returns>
        const   real3&  getDir() const
        {
            return  _dir;
        }
        /// <summary>
        /// 设置摄像机dir方向
        /// </summary>
        /// <param name="dir">摄像机dir方向</param>
        inline  void    setDir(const real3& dir)
        {
            _dir    =   dir;
        }
        /// <summary>
        /// 通过摄像机当前的eye位置和target位置计算dir方向
        /// </summary>
        inline  void    calcDir()
        {
            _dir    =   FE::normalize(_target - _eye);
        }
        /// <summary>
        /// 设置正交投影缩放比例
        /// </summary>
        /// <param name="scaler">设置正交投影缩放比例</param>
        inline  void    setScaler(real scaler)
        {
            _scaler = scaler;
        }
        /// <summary>
        /// 获取正交投影缩放比例
        /// </summary>
        /// <returns>获取正交投影缩放比例</returns>
        inline  real    getScaler() const
        {
            return _scaler;
        }
        /// <summary>
        /// 设置正交投影模式标志
        /// </summary>
        /// <param name="b">true表示为正交投影模式,否则为透视模式</param>
        inline  void    setOrtho(bool b)
        {
            _isOrtho = b;
        }
        /// <summary>
        /// 获取正交投影模式标志
        /// </summary>
        /// <returns>true表示为正交投影模式,否则为透视模式</returns>
        inline  bool    isOrtho() const
        {
            return _isOrtho;
        }
        /// <summary>
        /// 设置view size
        /// </summary>
        /// <param name="viewSize">宽度,高度</param>
        inline  void    setViewSize(const real2& viewSize)
        {
            _viewSize   =   viewSize;
        }
        /// <summary>
        /// 设置view size
        /// </summary>
        /// <param name="x">宽度</param>
        /// <param name="y">高度</param>
        inline  void    setViewSize(real x,real y)
        {
            _viewSize   =   real2(x,y);
        }
        /// <summary>
        /// 获取view size
        /// </summary>
        /// <returns>view size</returns>
        inline real2    getViewSize() const
        {
            return  _viewSize;
        }
        template<class T>
        inline tvec2<T> viewSize() const
        {
            return  tvec2<T>(T(_viewSize.x),T(_viewSize.y));
        }

        template<class T>
        inline  T       viewportWidth() const
        {
            return  T(_viewSize.x);
        }
        template<class T>
        inline  T       viewportHeight() const
        {
            return  T(_viewSize.y);
        }
        /// <summary>
        /// 设置project矩阵
        /// </summary>
        /// <param name="proj">proj project矩阵</param>
        inline  void    setProject(const mat4r& proj)
        {
            _matProj    =   proj;
        }
        /// <summary>
        /// 获取project矩阵
        /// </summary>
        /// <returns>project矩阵</returns>
        const   mat4r&  getProject() const
        {
            return  _matProj;
        }
        /// <summary>
        /// 设置像机的远近裁剪面
        /// 设置后，需要重新计算投影矩阵 @see updatePersective()，数据才会生效
        /// </summary>
        /// <param name="nearFar"></param>
        inline  void    setNearFar(const real2& nearFar)
        {
            if (!_freeze)
            {
                _zNear  =   nearFar.x;
                _zFar   =   nearFar.y;
            }
        }
        /// <summary>
        /// 获取像机的远近裁剪面
        /// </summary>
        /// <returns></returns>
        inline  real2   getNearFar() const
        {
            return  real2(_zNear,_zFar);
        }
        /// <summary>
        /// 获取像机的近裁剪面 
        /// </summary>
        /// <returns></returns>
        inline  real    getNear() const
        {
            return  _zNear;
        }
        /// <summary>
        /// 设置像机的近裁剪面
        /// </summary>
        /// <returns></returns>
        inline  void    setNear(real dNear)
        {
            _zNear  =   dNear;
        }
        /// <summary>
        /// 获取像机的远裁剪面
        /// </summary>
        /// <returns></returns>
        inline  real    getFar() const
        {
            return  _zFar;
        }
        /// <summary>
        /// 设置像机的远裁剪面
        /// </summary>
        /// <returns></returns>
        inline  void    setFar(real dFar)
        {
            _zFar   =   dFar;
        }
        /// <summary>
        /// 相机视野角(Field of view, FOV),
        /// </summary>
        /// <returns></returns>
        inline  auto    getFov() const
        {
            return  _fov;
        }
        inline  auto    fov() const
        {
            return  _fov;
        }
        /// <summary>
        /// 设置相机视野角(Field of view, FOV),
        /// </summary>
        /// <returns></returns>
        inline  void    setFov(real fov)
        {
            _fov   =   fov;
        }
        inline  real    getLeft() const
        {
            return  _rLeft;
        }
        inline  void    setLeft(real var)
        {
            _rLeft   =   var;
        }

        inline  real    getRightVar() const
        {
            return  _rRight;
        }
        inline  void    setRightVar(real var)
        {
            _rRight   =   var;
        }

        inline  real    getTop() const
        {
            return  _rTop;
        }
        inline  void    setTop(real var)
        {
            _rTop   =   var;
        }

        inline  real    getBottom() const
        {
            return  _rBottom;
        }
        inline  void    setBottom(real var)
        {
            _rBottom   =   var;
        }

        inline  real    getSpeed() const
        {
            return _speed;
        }

        inline  void    setSpeed(real speed)
        {
            _speed  =   speed;
        }
        /// <summary>
        /// 更新函数，会计算相机的dir
        /// 相机的up,right，保证正确性
        /// 计算观察矩阵
        /// </summary>
        inline  void    update()
        {
            _dir        =   normalize(_target - _eye);
            _up         =   normalize(_up);
            _right      =   normalize(cross(_dir, _up));
            _matView    =   lookAt(_eye, _target, _up);
            _matProj    =   FE::perspective<real>(_fov,_viewSize.x/_viewSize.y,_zNear,_zFar);
            /// 标记需要更新
            flags().addFlag(FLAG_UPDATE);
        }
        /// <summary>
        /// 获取view矩阵
        /// </summary>
        /// <returns></returns>
        const   mat4r&  getView() const
        {
            return  _matView;
        }
        
        /// <summary>
        /// 获取相机模型的转换矩阵
        /// </summary>
        /// <returns></returns>
        inline mat4r    transform()
        {
            mat4r rMat(1.0);
            rMat[0] = real4(_right, 0.0);
            rMat[1] = real4(_up, 0.0);
            rMat[2] = real4(-_dir, 0.0);
            rMat[3] = real4(_eye, 1.0);
            return rMat;
        }
        /// <summary>
        /// 更新 project 矩阵为透视投影矩阵
        /// </summary>
        inline  void    updatePerspective()
        {
            _matProj    =   FE::perspective<real>(_fov,_viewSize.x/_viewSize.y,_zNear,_zFar);
            flags().addFlag(FLAG_UPDATE);
        }
        /// <summary>
        /// 更新 project 矩阵为正交投影矩阵
        /// </summary>
        /// <param name="sceneHalfSize">sceneHalfSize 场景包围盒 half size</param>
        /// <param name="orthoDistance"></param>
        inline  void    updateOrtho()
        {
            real    dx      =   (_rRight - _rLeft)  / (2.0 * _scaler);
            real    dy      =   (_rTop   - _rBottom)/ (2.0 * _scaler);
            real    cx      =   (_rRight + _rLeft)  / 2.0;
            real    cy      =   (_rTop   + _rBottom)/ 2.0;

            real    left    =   cx - dx;
            real    right   =   cx + dx;
            real    top     =   cy + dy;
            real    bottom  =   cy - dy;

            _matProj        =   FE::ortho(left,right,bottom,top,_zNear,_zFar);
            flags().addFlag(FLAG_UPDATE);
        }
        /// <summary>
        /// 根据参数生成正交投影矩阵
        /// </summary>
        /// <param name="left"></param>
        /// <param name="right"></param>
        /// <param name="bottom"></param>
        /// <param name="top"></param>
        /// <param name="zNear"></param>
        /// <param name="zFar"></param>
        inline  void    ortho( real left, real right, real bottom, real top, real zNear, real zFar )
        {
            /// 保存正交参数，序列化使用
            _rLeft      =   left;
            _rRight     =   right;
            _rTop       =   top;
            _rBottom    =   bottom;
            _zNear      =   zNear;
            _zFar       =   zFar;
            _matProj    =   FE::ortho(_rLeft,_rRight,_rBottom,_rTop,_zNear,_zFar);
            flags().addFlag(FLAG_UPDATE);
        }
        
        /// <summary>
        /// 根据参数生成透视投影矩阵
        /// </summary>
        /// <param name="fov"></param>
        /// <param name="aspect"></param>
        /// <param name="zNear"></param>
        /// <param name="zFar"></param>
        inline  void    perspective(real fov, real aspect, real zNear, real zFar)
        {
            (void)aspect;
            (void)zNear;
            (void)zFar;

            _fov       =    fov;
            _zNear      =   zNear;
            _zFar       =   zFar;
            flags().addFlag(FLAG_UPDATE);
        }
        
        /// <summary>
        /// 获取世界坐标与屏幕像素的比例
        /// </summary>
        /// <param name="position">世界坐标位置,作为参考点做计算</param>
        /// <returns>返回比例值</returns>
        inline  real    pixelU(const real3& position) const
        {
            real3   p0      =   position;
            real3   p1      =   p0 + _right;
            real2   s0      =   this->worldToScreen(p0);
            real2   s1      =   this->worldToScreen(p1);
            real    pixelU  =   real(1.0) / (s1.x - s0.x);
            return  pixelU;
        }
        /// <summary>
        /// 世界坐标转化为窗口坐标
        /// </summary>
        /// <param name="world">欲转换的世界坐标</param>
        /// <returns>转换结果,窗口坐标</returns>
        inline  real2   worldToScreen( const real3& world) const
        {
            real4  worlds(world.x,world.y,world.z,1);
            real4  screens;
            project(worlds,screens);
            return  real2(screens.x,screens.y);
        }
        /// <summary>
        /// 世界坐标转化为窗口坐标
        /// </summary>
        /// <param name="world">欲转换的世界坐标</param>
        /// <returns>转换结果,窗口坐标</returns>
        inline  int2    worldToScreenInt( const real3& world) const
        {
            real4  worlds(world.x,world.y,world.z,1);
            real4  screens;
            project(worlds,screens);
            return  int2((int)screens.x,(int)screens.y);
        }
        /// <summary>
        /// 窗口坐标转化为世界坐标
        /// </summary>
        /// <param name="screen">欲转换的窗口坐标</param>
        /// <returns>转换结果,世界坐标</returns>
        inline  real3   screenToWorld(const real2& screen) const
        {
            real4  screens(screen.x,screen.y,0,1);
            real4  world;
            unProject(screens,world);
            return  real3(world.x,world.y,world.z);
        }
        /// <summary>
        ///  窗口坐标转化为世界坐标
        /// </summary>
        /// <param name="x">欲转换的窗口坐标x值</param>
        /// <param name="y">欲转换的窗口坐标y值</param>
        /// <returns>转换结果,世界坐标</returns>
        inline  real3   screenToWorld(real x,real y) const
        {
            real4  screens(x,y,0,1);
            real4  world;
            unProject(screens,world);
            return  real3(world.x,world.y,world.z);
        }
        /// <summary>
        ///  使用窗口坐标创建射线
        /// </summary>
        /// <param name="x">窗口坐标x值</param>
        /// <param name="y">窗口坐标y值</param>
        /// <returns>射线对象</returns>
        inline  Ray     createRayFromScreen(int x,int y) const
        {
            real4  minWorld;
            real4  maxWorld;

            real4  screen(real(x),real(y),0,1);
            real4  screen1(real(x),real(y),1,1);

            unProject(screen,minWorld);
            unProject(screen1,maxWorld);
            Ray     ray;
            ray.setOrigin(real3(minWorld.x,minWorld.y,minWorld.z));

            real3  dir(maxWorld.x - minWorld.x,maxWorld.y - minWorld.y, maxWorld.z - minWorld.z);
            ray.setDirection(normalize(dir));
            return  ray;
        }
        /// <summary>
        /// 根据给定世界坐标点,计算该点对应的屏幕坐标与世界坐标的比例
        /// </summary>
        /// <param name="context">上下文对象</param>
        /// <param name="center">指定的世界坐标位置点</param>
        /// <returns>屏幕坐标与世界坐标的比例</returns>
        real        calcWowrldPScreen(const real3& center);
        /// <summary>
        /// 摄像机左移(方向为 -right)
        /// </summary>
        /// <param name="fElapsed">fElapsed 方向偏移量</param>
        void        moveLeft(real fElapsed);
        /// <summary>
        ///  摄像机右移(方向为 right)
        /// </summary>
        /// <param name="fElapsed">fElapsed 方向偏移量</param>
        void        moveRight(real fElapsed);
        /// <summary>
        ///  摄像机前移(方向为 dir)
        /// </summary>
        /// <param name="fElapsed"> fElapsed 方向偏移量</param>
        void        moveFront(real fElapsed);
        /// <summary>
        ///  摄像机后移(方向为 dir)
        /// </summary>
        /// <param name="fElapsed">fElapsed 方向偏移量</param>
        void        moveBack(real fElapsed);
        /// <summary>
        /// 摄像机上移(方向为 up)
        /// </summary>
        /// <param name="fElapsed">fElapsed 方向偏移量</param>
        void        moveUp(real fElapsed);
        /// <summary>
        /// 摄像机下移(方向为 -up)
        /// </summary>
        /// <param name="fElapsed">fElapsed 方向偏移量</param>
        void        moveDown(real fElapsed);
        /// <summary>
        /// 根据指定方向移动相机
        /// </summary>
        /// <param name="dir">移动方向(单位向量)</param>
        /// <param name="fElapsed">fElapsed 方向偏移量</param>
        void        moveDir(const real3& dir, real fElapsed);
        /// <summary>
        ///  绕自身Z轴(up)自转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        void        rotateEyeZ(real angle);
        /// <summary>
        /// 绕自身X轴(right)自转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        void        rotateEyeX(real angle);
        /// <summary>
        /// 绕任意轴自转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        /// <param name="axis">旋转轴(单位向量)</param>
        void        rotateEyeAxis(real angle, const real3& axis);
        /// <summary>
        /// 绕世界坐标系Y轴(0,1,0)并以自身target为旋转点公转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        void        rotateViewY(real angle);
        /// <summary>
        /// 绕自身X轴(right)并以自身target为旋转点公转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        void        rotateViewX(real angle);
        /// <summary>
        /// 绕自身X轴(right)并以指定点为旋转点公转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        /// <param name="pos">旋转点</param>
        void        rotateViewXByCenter(real angle, const real3& pos);
        /// <summary>
        /// 绕世界坐标系Y轴(0,1,0)并以指定点为旋转点公转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        /// <param name="pos">旋转点</param>
        void        rotateViewYByCenter(real angle, const real3& pos);
        /// <summary>
        ///  绕世界坐标系Z轴(0,0,1)并以指定点为旋转点公转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        /// <param name="pos">旋转点</param>
        void        rotateViewZByCenter(real angle, const real3& pos);
        /// <summary>
        ///  绕任意轴自转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        /// <param name="axis">旋转轴(单位向量)</param>
        void        rotateViewByAxis(real angle, const real3& axis);
        /// <summary>
        /// 绕指定轴和指定中心点公转
        /// </summary>
        /// <param name="angle">旋转角度</param>
        /// <param name="axis">旋转轴(单位向量)</param>
        /// <param name="pos">旋转点</param>
        void        rotateViewByAxis(real angle, const real3& axis, const real3& pos);
        /// <summary>
        ///  指定点推进相机
        /// </summary>
        /// <param name="pos">推进点</param>
        /// <param name="persent">偏移系数，大于1则远离推进点，小于1则接近推进点</param>
        void        scaleCameraByPos(const real3& pos, real persent);

        /// <summary>
        ///  世界坐标转化为窗口坐标
        /// </summary>
        /// <param name="world">欲转换的世界坐标</param>
        /// <param name="screen">转换结果，窗口坐标</param>
        /// <returns>是否转换成功</returns>
        bool        project(const real4& world, real4& screen) const
        {
            screen = (_matProj * _matView) * world;
            if (screen.w == 0.0f)
            {
                return false;
            }
            screen.x /= screen.w;
            screen.y /= screen.w;
            screen.z /= screen.w;

            // map to range 0 - 1
            screen.x = screen.x * 0.5f + 0.5f;
            screen.y = screen.y * 0.5f + 0.5f;
            screen.z = screen.z * 0.5f + 0.5f;

            // map to viewport
            screen.x = screen.x * _viewSize.x;
            screen.y = _viewSize.y - (screen.y * _viewSize.y);
            return  true;
        }
        /// <summary>
        ///  窗口坐标转化为世界坐标
        /// </summary>
        /// <param name="screen">欲转换的窗口坐标</param>
        /// <param name="world">转换结果，世界坐标</param>
        /// <returns>是否转换成功</returns>
        bool        unProject(const real4& screen, real4& world) const
        {
            real4 v;
            v.x = screen.x;
            v.y = screen.y;
            v.z = screen.z;
            v.w = 1.0;

            // map from viewport to 0 - 1
            v.x = (v.x) / _viewSize.x;
            v.y = (v.y) / _viewSize.y;
           // v.y = (v.y - _viewSize.y) / _viewSize.y;

            // map to range -1 to 1
            v.x = v.x * 2.0f - 1.0f;
            v.y = v.y * 2.0f - 1.0f;
            v.z = v.z * 2.0f - 1.0f;

            mat4r  inverse = FE::inverse(_matProj * _matView);

            v = inverse * v;
            if (v.w == 0.0f)
            {
                return false;
            }
            world = v / v.w;
            return true;
        }
        enum    FaceDir: uint
        {
            Top                 =   1 << 0,    
            /// 底部
            Bottom              =   1 << 1,
            /// 前
            Front               =   1 << 2,
            ///后
            Back                =   1 << 3,
            /// 左
            Left                =   1 << 4,
            /// 右
            Right               =   1 << 5,
            /// <summary>
            /// 目前支持的组合
            /// </summary>
            FrontRightTop       =   Front | Right  | Top,
            FrontLeftTop        =   Front | Left   | Top,
            BackRightTop        =   Back  | Right  | Top,
            BackLeftTop         =   Back  | Left   | Top,
            FrontRightBottom    =   Front | Right  | Bottom,
            FrontLeftBottom     =   Front | Left   | Bottom,
            BackRightBottom     =   Back  | Right  | Bottom,
            BackLeftBottom      =   Back  | Left   | Bottom,

            FrontTop            =   Front | Top,
            LeftTop             =   Left  | Top,
            RightTop            =   Right | Top,
            BackTop             =   Back  | Top,
            FrontBottom         =   Front | Bottom,
            LeftBottom          =   Left  | Bottom,
            RightBottom         =   Right | Bottom,
            BackBottom          =   Back  | Bottom,
            FrontLeft           =   Front | Left,
            FrontRight          =   Front | Right,
            BackLeft            =   Back  | Left,
            BackRight           =   Back  | Right,
            /// <summary>
            /// 设计器轴测图视角,六个方向组合的最大值为 (Right<<1) - 1
            /// 所以下面从(Right<<1) + 1 开始
            /// </summary>
            ISO1                =   (Right<<1) + 1,
            ISO2                =   (Right<<1) + 2,
            ISO3                =   (Right<<1) + 3,
            ISO4                =   (Right<<1) + 4,
        };
        using   FaceDirs    =   FEFlags<FaceDir,uint>;
        /// <summary>
        /// 固定相加查看位置
        /// </summary>
        /// <param name="aabb"></param>
        /// <param name="director"></param>
        void        lookAtTo(const aabb3dr& aabb,FaceDirs director = Front);
        /// <summary>
        /// 根据屏幕空间的矩形范围生成裁剪锥体(坐标单位0-1)
        /// 需要把屏幕像素坐标映射到0-1
        /// 典型应用场景:鼠标框选
        /// </summary>
        /// <param name="vMin">范围0-1</param>
        /// <param name="vMax">范围0-1</param>
        /// <param name="vCorner">不为nullptr,则输出视锥的顶点坐标，必须不能少于8个顶点的缓冲区
        /// 
        ///    7-----6
        ///   /|    /|
        ///  / |   / |
        /// 3-----2  |
        /// |  4--|--5
        /// | /   | /
        /// |/    |/
        /// 0-----1
        ///
        /// </param>
        /// <returns></returns>
        FrustumR    extract(const real2& vMin,const real2& vMax,real3* vCorner = nullptr);
        /// <summary>
        /// 屏幕坐标，单位像素,注意:窗口坐标左上角为(0,0),右下角(width,height)
        /// 系统坐标系:左下角为(0,0),右上角为(width,height)
        /// 需要把y进行反转下
        /// </summary>
        /// <param name="screenMin"></param>
        /// <param name="screenMax"></param>
        /// <param name="vCorner">不为nullptr,则输出视锥的顶点坐标，必须不能少于8个顶点的缓冲区
        /// 
        ///    7-----6
        ///   /|    /|
        ///  / |   / |
        /// 3-----2  |
        /// |  4--|--5
        /// | /   | /
        /// |/    |/
        /// 0-----1
        ///
        /// </param>
        /// <returns></returns>
        FrustumR    extract(const uint2& screenMin,const uint2& screenMax,real3* vCorner = nullptr)
        {
            /// 将屏幕坐标映射成0-1
            real2   vMin    =   real2(screenMin.x,screenMin.y)/_viewSize;
            real2   vMax    =   real2(screenMax.x,screenMax.y)/_viewSize;
            /// 保证范围
            vMin    =   FE::clamp(vMin,real2(0,0),real2(1,1));
            vMax    =   FE::clamp(vMax,real2(0,0),real2(1,1));

            return  extract(vMin,vMax,vCorner);
        }
protected:
        void        onChanged()
        {
        }
public:
        friend  bool    operator==(const FECamera& lhs, const FECamera& rhs)
        {
            return &lhs == &rhs;
        }
        friend  bool    operator<(const FECamera& lhs, const FECamera& rhs)
        {
            return &lhs < &rhs;
        }
        /// <summary>
        /// 
        /// </summary>
        enum    SplitMethod: uint
        {
            /// <summary>
            /// 默认，自动根据near/far范围计算，最多生成三级
            /// </summary>
            SM_Auto     =   0,
            /// <summary>
            /// 线性分级
            /// </summary>
            SM_Uniform  =   1,
            /// <summary>
            /// 对数分级
            /// </summary>
            SM_Log      =   2,
        };
        static  auto    nameOfEnum(SplitMethod type)
        {
            switch (type)
            {
            case SM_Auto:     return  "auto";
            case SM_Uniform:  return  "uniform";
            case SM_Log:      return  "log";
            }
            return  "auto";
        }
        static  auto    splitMethodFromName(const char* name)
        {
            if (_stricmp(nameOfEnum(SM_Auto),   name) == 0) return  SM_Auto;
            if (_stricmp(nameOfEnum(SM_Uniform),name) == 0) return  SM_Uniform;
            if (_stricmp(nameOfEnum(SM_Log),    name) == 0) return  SM_Log;
            return  SM_Auto;
        }

        /// <summary>
        /// 根据最近点与最远点计算相机级联数据
        /// </summary>
        /// <param name="nearFar">相机裁剪距离</param>
        /// <returns>返回级联数据</returns>
        static  real2s  calcCascade(const real2& nearFar,SplitMethod method = SM_Auto,size_t cnt= 3)
        {
            (void)SM_Auto;
            (void)cnt;
            return  calcCascade(nearFar.x,nearFar.y,method,cnt);
        }
        /// <summary>
        /// 根据最近点与最远点计算相机级联数据
        /// </summary>
        /// <param name="fNear">相机近裁剪距离</param>
        /// <param name="fFar">相机远裁剪距离</param>
        /// <param name="metod"></param>
        /// <returns>返回级联数据</returns>
        static  real2s  calcCascade(const real& fNear,const real& fFar,SplitMethod metod = SM_Auto,size_t cnt= 3)
        {
            switch (metod)
            {
            case SM_Uniform:
                return  uniformSplit(cnt,fNear,fFar);
            case SM_Log:
                return  logarithmicSplit(cnt,fNear,fFar);
            default:
                {
                    real    dMax    =   fFar - fNear;
                    real2s  results;
                    /// 超过 1000000.0 ,则分三级
                    if (dMax/1000000.0 > 1.0)
                    {
                        results.push_back(real2(0.0 + fNear,        1000.0 + fNear));
                        results.push_back(real2(1000.0,             1000000.0 + fNear));
                        results.push_back(real2(1000000.0,          dMax + fNear));
                    }
                    /// 超过 100000.0 ,则分2级
                    else if (dMax/100000.0 > 1.0)
                    {
                        results.push_back(real2(0.0 + fNear,        1000.0 + fNear));
                        results.push_back(real2(1000.0,             dMax + fNear));
                    }
                    /// 小于 100000 , 则分1级
                    else
                    {
                        results.push_back(real2(0.0 + fNear,      dMax + fNear));
                    }
                    return  results;
                }
                break;
            }
        }
        /// <summary>
        /// 用途:根据相机的最近与最远距离，生成级联数据
        /// </summary>
        /// <param name="amount">生成级联的个数，3可以适合大多数场景</param>
        /// <param name="neard">相机的最近点</param>
        /// <param name="fard">相机的最远点</param>
        /// <returns></returns>
        static  real2s  uniformSplit(size_t amount,real neard, real fard)
        {
            real2s  result;
            for ( size_t i = 1; i < amount; i ++ )
            {
                real    dBreak  =   ( neard + ( fard - neard ) * i / amount ) / fard;
                real    vNext   =   FE::lerp(neard, fard, dBreak );
                if (i == 1)
                    result.push_back(real2(neard,vNext));
                else
                    result.push_back(real2(result.back().y,vNext));
            }
            if (result.empty())
                result.push_back(real2(neard,fard));
            else
                result.push_back(real2(result.back().y,fard));
            return  result;
        }
        /// <summary>
        /// 用途:(对数法) 根据相机的最近与最远距离，生成级联数据
        /// </summary>
        /// <param name="amount">生成级联的个数，3可以适合大多数场景</param>
        /// <param name="neard">相机的最近点</param>
        /// <param name="fard">相机的最远点</param>
        /// <returns></returns>
        static  real2s  logarithmicSplit(size_t amount,real neard, real fard)
        {
            real2s  result;
            for ( size_t i = 1; i < amount; i ++ )
            {
                real    dBreak  =   (neard * pow( fard / neard , ( real(i) / real(amount)))) / fard;
                real    vNext   =   FE::lerp(neard, fard, dBreak );
                if (i == 1)
                    result.push_back(real2(neard,vNext));
                else
                    result.push_back(real2(result.back().y,vNext));
            }
            if (result.empty())
                result.push_back(real2(neard,fard));
            else
                result.push_back(real2(result.back().y,fard));
            return  result;
        }
    };

    using   Camera          =   SharedPtr<FECamera>;
    using   CameraList      =   std::vector<Camera>;
    using   Cameras         =   std::vector<Camera>;


}
