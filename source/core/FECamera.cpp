
#include    "FECamera.hpp"


namespace FE
{
    /// <summary>
    /// 绕任意轴的旋转矩阵
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="angle">角度</param>
    /// <param name="v">轴</param>
    /// <returns></returns>
    template<typename T>
    static tmat3<T> makeRotation(T angle, const tvec3<T>& v)
    {
        const T a = (T)DEG2RAD(angle);
        const T c = (T)cos(a);
        const T s = (T)sin(a);

        tvec3<T> axis = FE::normalize(v);

        tvec3<T> temp = (T(1) - c) * axis;

        tmat3<T> res;
        res[0][0] = c + temp[0] * axis[0];
        res[0][1] = T(0) + temp[0] * axis[1] + s * axis[2];
        res[0][2] = T(0) + temp[0] * axis[2] - s * axis[1];

        res[1][0] = T(0) + temp[1] * axis[0] - s * axis[2];
        res[1][1] = c + temp[1] * axis[1];
        res[1][2] = T(0) + temp[1] * axis[2] + s * axis[0];

        res[2][0] = T(0) + temp[2] * axis[0] + s * axis[1];
        res[2][1] = T(0) + temp[2] * axis[1] - s * axis[0];
        res[2][2] = c + temp[2] * axis[2];

        return res;
    }
    FECamera::FECamera(FEContext& ctx,const real3 & target, const real3 & eye, const real3 & right)
        :FEObject(ctx)
    {
        _viewSize       =   real2(256, 256);
        _matView        =   mat4r(1);
        _matProj        =   mat4r(1);
       
        _speed          =   10;
        _flag           =   0xFFFFFFFF;
        _target         =   target;
        _eye            =   eye;
        _dir            =   normalize(_target - _eye);
        _right          =   right;
        _up             =   normalize(cross(_right, _dir));
        _fov           =   45.0f;
        _isOrtho        =   false;
        _scaler         =   1.0;
        _zNear          =   0.1;
        _zFar           =   1.0;
        sprintf(_name,"camera-%p",this);
    }
    FECamera::~FECamera()
    {}


    real    FECamera::calcWowrldPScreen(const real3& center)
    {
        real3   rightDir    =   getRight();
        real3   upDir       =   getUp();

        real3   pos         =   real3(center.x, center.y, center.z);
        real3   pos1        =   center + rightDir * 1000.0;

        real2   screen0     =   worldToScreen(pos);
        real2   screen1     =   worldToScreen(pos1);
        real    pixelU      =   1000.0 / (screen1.x - screen0.x);
        return  pixelU;
    }

    void    FECamera::moveLeft(real fElapsed)
    {
        _eye    -= normalize(_right) * _speed * fElapsed;
        _target -= normalize(_right) * _speed * fElapsed;
    }
    void    FECamera::moveRight(real fElapsed)
    {
        _eye    += normalize(_right) * _speed * fElapsed;
        _target += normalize(_right) * _speed * fElapsed;
    }
    void    FECamera::moveFront(real fElapsed)
    {
        _eye    += _dir * _speed * fElapsed;
        _target += _dir * _speed * fElapsed;
        _scaler -= _speed * fElapsed * 0.1;
    }
    void    FECamera::moveBack(real fElapsed)
    {
        _eye    -= _dir * _speed * fElapsed;
        _target -= _dir * _speed * fElapsed;
        _scaler += _speed * fElapsed* 0.1;
    }
    void    FECamera::moveUp(real fElapsed)
    {
        _eye    += _up * _speed * fElapsed;
        _target += _up * _speed * fElapsed;
    }
    void    FECamera::moveDown(real fElapsed)
    {
        _eye    -= _up * _speed * fElapsed;
        _target -= _up * _speed * fElapsed;
    }
    void    FECamera::moveDir(const real3& dir, real fElapsed)
    {
        _eye    += dir * _speed * fElapsed;
        _target += dir * _speed * fElapsed;

        _scaler += _speed * fElapsed * -dot(dir, _dir)* 0.1;
    }

    void    FECamera::rotateEyeZ(real angle)
    {
        if (!(_flag & FLAG_ROT_Z))
        {
            return;
        }
        real    len(0);

        mat4r   mat =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), _up);
        /// mat.rotate(angle, _up);
        _dir    =   mat * real4(_dir,1.0);
        _right  =   normalize(cross(_dir, _up));
        len     =   length(_eye - _target);
        _target = _eye + _dir * len;
        update();
    }
    void    FECamera::rotateEyeX(real angle)
    {
        if (!(_flag & FLAG_ROT_X))
        {
            return;
        }
        real    len(0);
        mat4r   mat =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), _right);
        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up ,1.0);        
        len     =   length(_eye - _target);
        _target =   _eye + _dir * len;
        update();
    }
    void    FECamera::rotateEyeAxis(real angle, const real3 & axis)
    {
        real    len(0);
        mat4r   mat =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), axis);
        _up     =   mat * real4(_up ,   1.0);
        _right  =   mat * real4(_right, 1.0);
        _dir    =   normalize(cross(_up, _right));
        len     =   length(_eye - _target);
        _target =   _eye + _dir * len;
        update();
    }
    void    FECamera::rotateViewY(real angle)
    {
        if (!(_flag & FLAG_ROT_Y))
        {
            return;
        }
        real    len(0);
        mat4r   mat =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), real3(0, 1, 0));
        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up ,1.0);
        _right  =   normalize(cross(_dir, _up));
        len     =   length(_eye - _target);
        _eye    =   _target - _dir * len;
        update();
    }
    void    FECamera::rotateViewX(real angle)
    {
        if (!(_flag & FLAG_ROT_X))
        {
            return;
        }
        real    len(0);
        mat4r   mat =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), _right);
        /// mat.rotate(angle, _right);
        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up ,1.0);
        _right  =   normalize(cross(_dir, _up));
        len     =   length(_eye - _target);
        _eye    =   _target - _dir * len;
        update();
    }
    void    FECamera::rotateViewXByCenter(real angle, const real3 & pos)
    {
        if (!(_flag & FLAG_ROT_X))
        {
            return;
        }
        //! 计算眼睛到鼠标点的方向
        real3   vDir    =   pos - _eye;
        real    len1    =   length(vDir);
        real    len     =   0;
                vDir    =   normalize(vDir);
        mat4r   mat     =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), _right);
        /// mat.rotate(angle, _right);

        vDir    =   mat * real4(vDir    ,1.0);
        _eye    =   pos - vDir * len1;

        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up, 1.0);
        _right  =   normalize(cross(_dir, _up));
        _up     =   normalize(cross(_right, _dir));
        len     =   length(_eye - _target);

        _target = _eye + _dir * len;
        update();

    }
    void    FECamera::rotateViewYByCenter(real angle, const real3 & pos)
    {
        if (!(_flag & FLAG_ROT_Y))
        {
            return;
        }
        real    len(0);
        real    len1(0);
        mat4r   mat     =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), real3(0, 1, 0));

        real3   vDir    =   pos - _eye;

        len1    =   length(vDir);
        vDir    =   normalize(vDir);
        vDir    =   mat * real4(vDir,1.0);
        _eye    =   pos - vDir * len1;

        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up, 1.0);
        _right  =   normalize(cross(_dir, _up));
        _up     =   normalize(cross(_right, _dir));
        len     =   length(_eye - _target);
        _target =   _eye + _dir * len;
        update();
    }
    void    FECamera::rotateViewZByCenter(real angle, const real3 & pos)
    {
        if (!(_flag & FLAG_ROT_Z))
        {
            return;
        }
        real    len(0);
        real    len1(0);
        mat4r   mat     =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), real3(0, 0, 1));
        real3   vDir    =   pos - _eye;

        len1    =   length(vDir);
        vDir    =   normalize(vDir);
        vDir    =   mat * real4(vDir,1.0);
        _eye    =   pos - vDir * len1;

        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up,1.0);
        _right  =   normalize(cross(_dir, _up));
        _up     =   normalize(cross(_right, _dir));
        len     =   length(_eye - _target);
        _target =   _eye + _dir * len;
        update();
    }
    void    FECamera::rotateViewByAxis(real angle, const real3 & axis)
    {
        real    len(0);
        mat4r   mat =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), axis);

        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up ,1.0);
        _right  =   normalize(cross(_dir, _up));
        _up     =   normalize(cross(_right, _dir));
        len     =   length(_eye - _target);
        _target =   _eye + _dir * len;
        update();
    }
    void    FECamera::rotateViewByAxis(real angle, const real3 & axis, const real3 & pos)
    {
        real    len     =   (0);
        real    len1    =   (0);
        mat4r   mat     =   FE::rotate(mat4r(1), (real)DEG2RAD(angle), axis);
      
        real3   vDir = pos - _eye;

        len1    =   length(vDir);
        vDir    =   normalize(vDir);
        vDir    =   mat * real4(vDir,1.0);

        len     =   length(_eye - _target);

        _eye    =   pos - vDir * len1;

        _dir    =   mat * real4(_dir,1.0);
        _up     =   mat * real4(_up, 1.0);
        _right  =   normalize(cross(_dir, _up));
        _up     =   normalize(cross(_right, _dir));
        _target =   _eye + _dir * len;
        update();
    }

    void    FECamera::scaleCameraByPos(const real3 & pos, real persent)
    {
        real3   dir     =   normalize(pos - _eye);
        real    dis     =   length(pos - _eye) * persent;
        real3   dirCam  =   normalize(_target - _eye);
        real    disCam  =   length(_target - _eye) * persent;
        _eye            =   pos - dir * dis;
        _target         =   _eye + dirCam * disCam;
        _scaler         *=  persent;
        update();
    }

    void    FECamera::lookAtTo(const aabb3dr& aabb,FaceDirs dirs)
    {
        real3 center    =   aabb.getCenter();

        real3 back      =   real3(0, 1, 0);
        real3 right     =   real3(1, 0, 0);
        real3 top       =   real3(0, 0, 1);

        real3 front     =   -back;
        real3 left      =   -right;
        real3 bottom    =   -top;
        ///从各个方向去看时的 摄像机dir
        real3 frontDir  =   back;
        real3 backDir   =   front;
        real3 leftDir   =   right;
        real3 rightDir  =   left;
        real3 topDir    =   bottom;
        real3 bottomDir =   top;

        //默认为正前方
        real3 dir       =   frontDir;
        real3 up        =   top;

        switch (dirs.data())
        {
        case FECamera::Top:
            {
                dir     =   topDir;
                up      =   back;
            }
            break;
        case FECamera::Bottom:
            {
                dir     =   bottomDir;
                up      =   front;
            }
            break;
        case FECamera::Front:
            {
                dir     =   frontDir;
                up      =   top;
            }
            break;
        case FECamera::Back:
            {
                dir     =   backDir;
                up      =   top;
            }
            break;
        case FECamera::Left:
            {
                dir     =   leftDir;
                up      =   top;
            }
            break;
        case FECamera::Right:
            {
                dir     =   rightDir;
                up      =   top;
            }
            break;
        /// 前 右 上 三面的交点
        case FrontRightTop:
            {
                dir =   normalize(frontDir + rightDir + topDir);
                up  =   normalize(frontDir + rightDir + bottomDir);
            }
            break;
#pragma region 
                    
        /// 前 左 上 三面的交点
        case FrontLeftTop:
            {
                dir =   normalize(frontDir + leftDir + topDir);
                up  =   normalize(frontDir + leftDir + bottomDir);
            }
            break;
        /// 后 右 上 三面的交点
        case BackRightTop:
            {
                dir =   normalize(backDir + rightDir + topDir);
                up  =   normalize(backDir + rightDir + bottomDir);
            }
            break;
        /// 后 左 上 三面的交点
        case BackLeftTop:
            {
                dir =   normalize(backDir + leftDir + topDir);
                up  =   normalize(backDir + leftDir + bottomDir);
            }
            break;
        /// 前 右 下 三面的交点
        case FrontRightBottom:
            {
                dir =   normalize(frontDir + rightDir + bottomDir);
                up  =   normalize(backDir + leftDir + bottomDir);
            }
            break;
        /// 前 左 下 三面的交点
        case FrontLeftBottom:
            {
                dir =   normalize(frontDir + leftDir + bottomDir);
                up  =   normalize(backDir + rightDir + bottomDir);
            }
            break;
        /// 后 右 下 三面的交点
        case BackRightBottom:
            {
                dir =   normalize(backDir + rightDir + bottomDir);
                up  =   normalize(frontDir + leftDir +bottomDir);
            }
            break;
        /// 后 左 下 三面的交点
        case BackLeftBottom:
            {
                dir =   normalize(backDir + leftDir + bottomDir);
                up  =   normalize(frontDir + rightDir + bottomDir);
            }
            break;
#pragma endregion
#pragma region
        /// 前 上 两面的交线
        case FrontTop:
            {
                dir =   normalize(frontDir + topDir);
                up  =   normalize(frontDir + bottomDir);
            }
            break;
        /// 左 上 两面的交线
        case LeftTop:
            {
                dir =   normalize(leftDir + topDir);
                up  =   normalize(leftDir + bottomDir);
            }
            break;
        /// 右 上 两面的交线
        case RightTop:
            {
                dir =   normalize(rightDir + topDir);
                up  =   normalize(rightDir + bottomDir);
            }
            break;
        /// 后 上 两面的交线
        case BackTop:
            {
                dir =   normalize(backDir + topDir);
                up  =   normalize(backDir + bottomDir);
            }
            break;
        /// 前 下 两面的交线
        case FrontBottom:
            {
                dir =   normalize(frontDir + bottomDir);
                up  =   normalize(backDir + bottomDir);
            }
            break;
        /// 左 下 两面的交线
        case LeftBottom:
            {
                dir =   normalize(leftDir + bottomDir);
                up  =   normalize(rightDir + bottomDir);
            }
            break;
        /// 右 下 两面的交线
        case RightBottom:
            {
                dir =   normalize(rightDir + bottomDir);
                up  =   normalize(leftDir +bottomDir);
            }
            break;
        /// 后 下 两面的交线
        case BackBottom:
            {
                dir =   normalize(backDir + bottomDir);
                up  =   normalize(frontDir + bottomDir);
            }
            break;
        /// 前 左 两面的交线
        case FrontLeft:
            {
                dir =   normalize(frontDir + leftDir);
                up  =   top;
            }
            break;
        /// 前 右 两面的交线
        case FrontRight:
            {
                dir =   normalize(frontDir + rightDir);
                up  =   top;
            }
            break;
        /// 后 左 两面的交线
        case BackLeft:
            {
                dir =   normalize(backDir + leftDir);
                up  =   top;
            }
            break;
        /// 后 右 两面的交线
        case BackRight:
            {
                dir =   normalize(backDir + rightDir);
                up  =   top;
            }
            break;
#pragma endregion
        /// 下面算法来自设计器
        case FECamera::ISO1:
            {
                real3   axisA       =   real3(+0, -1, +0);
                real3   axisB       =   real3(-1, +0, +0);
                real3   axisC       =   real3(+0, +0, -1);
                real    angleA      =   45;
                real    angleB      =   35;

                real3   axisUp0     =   FE::normalize(cross(axisA, axisB));
                mat3r   rotMat0     =   makeRotation(angleA, axisUp0);

                real3   axisLeft1   =   normalize(rotMat0 * axisA);

                real3   axisUp1     =   FE::normalize(cross(axisLeft1, axisC));
                mat3r   rotMat1     =   makeRotation(angleB, axisUp1);

                dir         =   rotMat1 * axisLeft1;
                up          =   real3(0.0, 0.0, 1.0);
                auto tRight =   cross(dir, up);
                up          =   FE::normalize(cross(tRight, dir));
            }
            break;
        case FECamera::ISO2:
            {
                real3   axisA       =   real3(-1, +0, +0);
                real3   axisB       =   real3(+0, +1, +0); 
                real3   axisC       =   real3(+0, +0, -1);
                real    angleA      =   45;
                real    angleB      =   35;

                real3   axisUp0     =   FE::normalize(cross(axisA, axisB));
                mat3r   rotMat0     =   makeRotation(angleA, axisUp0);

                real3   axisLeft1   =   FE::normalize(rotMat0 * axisA);

                real3   axisUp1     =   FE::normalize(cross(axisLeft1, axisC));
                mat3r   rotMat1     =   makeRotation(angleB, axisUp1);

                dir         =   rotMat1 * axisLeft1;
                up          =   real3(0.0, 0.0, 1.0);
                auto tRight =   cross(dir, up);
                up          =   FE::normalize(cross(tRight, dir));
            }
            break;
        case FECamera::ISO3:
            {
                real3   axisA       =   real3(+1, +0, +0);
                real3   axisB       =   real3(+0, +1, +0); 
                real3   axisC       =   real3(+0, +0, -1);
                real    angleA      =   45;
                real    angleB      =   35;

                real3   axisUp0     =   FE::normalize(cross(axisA, axisB));
                mat3r   rotMat0     =   makeRotation(angleA, axisUp0);

                real3   axisLeft1   =   FE::normalize(rotMat0 * axisA);

                real3   axisUp1     =   FE::normalize(cross(axisLeft1, axisC));
                mat3r   rotMat1     =   makeRotation(angleB, axisUp1);

                dir         =   rotMat1 * axisLeft1;
                up          =   real3(0.0, 0.0, 1.0);
                auto tRight =   cross(dir, up);
                up          =   FE::normalize(cross(tRight, dir));
            }
            break;
        case FECamera::ISO4:
            {
                real3   axisA       =   real3(+0, -1, +0);
                real3   axisB       =   real3(+1, +0, +0); 
                real3   axisC       =   real3(+0, +0, -1);
                real    angleA      =   45;
                real    angleB      =   35;

                real3   axisUp0     =   FE::normalize(cross(axisA, axisB));
                mat3r   rotMat0     =   makeRotation(angleA, axisUp0);

                real3   axisLeft1   =   normalize(rotMat0 * axisA);

                real3   axisUp1     =   FE::normalize(cross(axisLeft1, axisC));
                mat3r   rotMat1     =   makeRotation(angleB, axisUp1);

                dir         =   rotMat1 * axisLeft1;
                up          =   real3(0.0, 0.0, 1.0);
                auto tRight =   cross(dir, up);
                up          =   FE::normalize(cross(tRight, dir));
            }
            break;
        
        }
        real    len     =   length(aabb.getSize()) * 0.5;
        real    ang     =   (PI/180) * (_fov) * 0.5f;
        real3   eye     =   center - dir * (len / tan(ang));
        real3   target  =   center;

        real4   temp    =   real4(eye, 1.0);
        eye             =   real3(temp.x, temp.y, temp.z);
        temp            =   real4(target, 1.0);
        target          =   real3(temp.x, temp.y, temp.z);
        temp            =   real4(up, 0.0);
        up              =   normalize(real3(temp.x, temp.y, temp.z));

        setEye(eye);
        setTarget(target);
        setUp(up);
        update();
    }
    FrustumR    FECamera::extract()
    {
        FrustumR    result;
        mat4r       vp  =   _matProj * _matView;
        result.loadFrustum(vp);
        return      result;
    }


    FrustumR    FECamera::extract(const real2& vMin,const real2& vMax,real3* vOutCorner)
    {
        FrustumR    result;
        /// 将屏幕坐标映射成0-1
        /// real2   vMin    =   real2(screenMin.x,screenMin.y)/_viewSize;
        /// real2   vMax    =   real2(screenMax.x,screenMax.y)/_viewSize;
        /// 保证范围
        /// vMin.clamp(real2(0,0),real2(1,1));
        /// vMax.clamp(real2(0,0),real2(1,1));

        /// 1. 定义 NDC 空间的 8 个角点 (z 为 -1 表示近平面，1 表示远平面)
        /// OpenGL 标准
        real    minX        =   vMin.x * 2.0 - 1.0;
        real    maxX        =   vMax.x * 2.0 - 1.0;

        real    minY        =   vMin.y * 2.0 - 1.0;
        real    maxY        =   vMax.y * 2.0 - 1.0;
        real    minZ        =   -1.0;
        real    maxZ        =   +1.0;

        /// NDC 空间坐标
        real4   ndcCorners[8] =
        {
            /// 近平面 (z=0)
            {minX, minY, minZ, 1}, {maxX, minY, minZ, 1}, {maxX, maxY, minZ, 1},{minX, maxY, minZ, 1},
            /// 远平面 (z=1)
            {minX, minY, maxZ, 1}, {maxX, minY, maxZ, 1}, {maxX, maxY, maxZ, 1},{minX, maxY, maxZ, 1}, 
        };

        mat4r   matVP       =   _matProj * _matView ;
        mat4r   invVP       =   FE::inverse(matVP);
        real3   corners[8];
        /// 3. 变换到世界空间
        for (int i = 0; i < 8; ++i)
        {
            const real4 worldPt =   invVP * ndcCorners[i];
            const real  wInv    =   real(1.0)/worldPt.w;
            corners[i]          =   worldPt * wInv;
        }
        if (vOutCorner)
        {
            memcpy(vOutCorner,corners,sizeof(corners));
        }
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
        /// 4. 构建 6 个平面 (确点序保法线指向 Frustum 内部)
        /// 使用右手定则：手指按点序弯曲，大拇指指向内部
        result._planes[FrustumR::FRUSTUM_LEFT]      = PlaneR(corners[0],corners[4], corners[7]); 
        result._planes[FrustumR::FRUSTUM_RIGHT]     = PlaneR(corners[1],corners[2], corners[6]); 
        result._planes[FrustumR::FRUSTUM_BOTTOM]    = PlaneR(corners[0],corners[1], corners[5]); 
        result._planes[FrustumR::FRUSTUM_TOP]       = PlaneR(corners[2],corners[3], corners[7]); 
        result._planes[FrustumR::FRUSTUM_NEAR]      = PlaneR(corners[0],corners[3], corners[2]); 
        result._planes[FrustumR::FRUSTUM_FAR]       = PlaneR(corners[4],corners[5], corners[6]); 

        return  result;
    }
}
