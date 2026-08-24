#pragma     once
#include    "FEObject.h"
#include    "FEMath.hpp"

namespace   FE
{
    enum    EulerOrder
    {
        EO_XYZ,
        EO_YXZ,
        EO_ZXY,
        EO_ZYX,
        EO_YZX,
        EO_XZY
    };

    template <class T>
    inline  T   tclamp(T value, T min, T max)
    {
        assert(min < max && "Invalid clamp range");
        return (std::min<T>)((std::max<T>)(value, min), max);
    }

    /// <summary>
    ///旋转矩阵转欧拉角
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="m">旋转矩阵</param>
    /// <param name="order">欧拉角</param>
    /// <returns></returns>
    template <class T>
    tvec3<T>    matrixToEuler(const tmat4<T>& m, EulerOrder order =  EO_ZYX)
    {
        T*  te      =   (T*)&m[0];
        T   m11     =   te[ 0 ]; T m12 = te[ 4 ]; T m13 = te[ 8 ];
        T   m21     =   te[ 1 ]; T m22 = te[ 5 ]; T m23 = te[ 9 ];
        T   m31     =   te[ 2 ]; T m32 = te[ 6 ]; T m33 = te[ 10 ];

        tvec3<T> euler(T(0));

        switch(order)
        {
        case EO_XYZ:
            {
                euler.y = asin(tclamp<T>( m13, T(-1), T(1)));
                //if (fabs( m13 ) < 0.99999f )
                if (T(1) - fabs(m13) > FLT_EPSILON)
                {
                    euler.x = atan2(-m23, m33);
                    euler.z = atan2(-m12, m11);
                }
                else 
                {
                    euler.x = atan2(m32, m22);
                    euler.z = T(0);
                }
            }
            break;
        case EO_YXZ:
            {
                euler.x = asin( -tclamp<T>(m23, T(-1), T(1)));

                //if ( fabs( m23 ) < 0.99999 ) 
                if (T(1) - fabs(m23) > FLT_EPSILON)
                {
                    euler.y = atan2(m13, m33);
                    euler.z = atan2(m21, m22);
                }
                else 
                {
                    euler.y = atan2(-m31, m11);
                    euler.z = 0;
                }
            }
            break;
        case EO_ZXY:
            {
                euler.x = asin(tclamp<T>(m32, T(-1), T(1)));

                //if ( fabs( m32 ) < 0.99999 ) 
                if (T(1) - fabs(m32) > FLT_EPSILON)
                {
                    euler.y = atan2(-m31, m33);
                    euler.z = atan2(-m12, m22);
                }
                else 
                {
                    euler.y = T(0);
                    euler.z = atan2(m21, m11);
                }
            }
            break;
        case EO_ZYX:
            {
                euler.y = asin(-tclamp<T>(m31, T(-1), T(1)));

                //if ( fabs( m31 ) < 0.99999 ) 
                if (T(1) - fabs(m31) > FLT_EPSILON)
                {
                    euler.x = atan2(m32, m33);
                    euler.z = atan2(m21, m11);
                }
                else
                {
                    euler.x = T(0);
                    euler.z = atan2(-m12, m22);
                }
            }
            break;
        case EO_YZX:
            {
                euler.z = asin(tclamp<T>(m21, T(-1), T(1)));

                //if ( fabs( m21 ) < 0.99999 )
                if (T(1) - fabs(m21) > FLT_EPSILON)
                {
                    euler.x = atan2(-m23, m22);
                    euler.y = atan2(-m31, m11);
                } 
                else 
                {
                    euler.x = T(0);
                    euler.y = atan2(m13, m33);
                }
            }
            break;
        case EO_XZY:
            {
                euler.z = asin(-tclamp<T>(m12, T(-1), T(1)));

                //if (fabs(m12) < 0.99999 )
                if (T(1) - fabs(m12) > FLT_EPSILON)
                {
                    euler.x = atan2(m32, m22);
                    euler.y = atan2(m13, m11);
                }
                else
                {
                    euler.x = atan2(-m23, m33);
                    euler.y = 0;
                }
            }
            break;
        default:
            assert("unsupport order" && 0);
            break;
        }
        T   tmpX    =   T(RAD2DEG(euler.x)) ;
        T   tmpY    =   T(RAD2DEG(euler.y)) ;
        T   tmpZ    =   T(RAD2DEG(euler.z)) ;


        euler.x     =   tmpX;
        euler.y     =   tmpY;
        euler.z     =   tmpZ;
        return  euler;
    }

    /// <summary>
    /// 欧拉角转旋转矩阵
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="euler">欧拉角</param>
    /// <param name="order"></param>
    /// <returns>旋转矩阵</returns>
    template<class T>
    tmat4<T>    eulerToMatrix(const tvec3<T>& euler,EulerOrder order =  EO_ZYX)
    {
        tmat4<T>    m;
        T*  te  =  (T*)&m[0];

        T   x   =  (T)DEG2RAD(euler.x), y = (T)DEG2RAD(euler.y), z = (T)DEG2RAD( euler.z); 
        T   a   =    cos( x ), b = sin( x );
        T   c   =    cos( y ), d = sin( y );
        T   e   =    cos( z ), f = sin( z );

        switch(order)
        {
        case EO_XYZ:
            {
                T ae = a * e, af = a * f, be = b * e, bf = b * f;

                te[ 0 ] = c * e;
                te[ 4 ] = - c * f;
                te[ 8 ] = d;

                te[ 1 ] = af + be * d;
                te[ 5 ] = ae - bf * d;
                te[ 9 ] = - b * c;

                te[ 2 ] = bf - ae * d;
                te[ 6 ] = be + af * d;
                te[ 10 ] = a * c;
            }
            break;
        case EO_YXZ:
            {
                T ce = c * e, cf = c * f, de = d * e, df = d * f;

                te[ 0 ] = ce + df * b;
                te[ 4 ] = de * b - cf;
                te[ 8 ] = a * d;

                te[ 1 ] = a * f;
                te[ 5 ] = a * e;
                te[ 9 ] = - b;

                te[ 2 ] = cf * b - de;
                te[ 6 ] = df + ce * b;
                te[ 10 ] = a * c;
            }
            break;
        case EO_ZXY:
            {
                T ce = c * e, cf = c * f, de = d * e, df = d * f;

                te[ 0 ] = ce - df * b;
                te[ 4 ] = - a * f;
                te[ 8 ] = de + cf * b;

                te[ 1 ] = cf + de * b;
                te[ 5 ] = a * e;
                te[ 9 ] = df - ce * b;

                te[ 2 ] = - a * d;
                te[ 6 ] = b;
                te[ 10 ] = a * c;
            }
            break;
        case EO_ZYX:
            {
                T ae = a * e, af = a * f, be = b * e, bf = b * f;

                te[ 0 ] = c * e;
                te[ 4 ] = be * d - af;
                te[ 8 ] = ae * d + bf;

                te[ 1 ] = c * f;
                te[ 5 ] = bf * d + ae;
                te[ 9 ] = af * d - be;

                te[ 2 ] = - d;
                te[ 6 ] = b * c;
                te[ 10 ] = a * c;
            }
            break;
        case EO_YZX:
            {
                T ac = a * c, ad = a * d, bc = b * c, bd = b * d;

                te[ 0 ] = c * e;
                te[ 4 ] = bd - ac * f;
                te[ 8 ] = bc * f + ad;

                te[ 1 ] = f;
                te[ 5 ] = a * e;
                te[ 9 ] = - b * e;

                te[ 2 ] = - d * e;
                te[ 6 ] = ad * f + bc;
                te[ 10 ] = ac - bd * f;
            }
            break;
        case EO_XZY:
            {
                T ac = a * c, ad = a * d, bc = b * c, bd = b * d;

                te[ 0 ] = c * e;
                te[ 4 ] = - f;
                te[ 8 ] = d * e;

                te[ 1 ] = ac * f + bd;
                te[ 5 ] = a * e;
                te[ 9 ] = ad * f - bc;

                te[ 2 ] = bc * f - ad;
                te[ 6 ] = b * e;
                te[ 10 ] = bd * f + ac;
            }
            break;
        }
        // last column
        te[ 3 ] = 0;
        te[ 7 ] = 0;
        te[ 11 ] = 0;

        // bottom row
        te[ 12 ] = 0;
        te[ 13 ] = 0;
        te[ 14 ] = 0;
        te[ 15 ] = 1;

        return m;
    }
    template <typename T>
    tvec3<T>    quatToEuler(const tquat<T>& q, EulerOrder mode = EO_ZYX)
    {
        return matrixToEuler(quatToMat4(q), mode);
    }

    template <typename T>
    tquat<T>    eulerToQuat(const tvec3<T>& euler)
    {
        return  mat4ToQuat(eulerToMatrix(euler));
    }

    DEFINE_CLASS_UUID(FEEulerObject, "{C43557EC-47BA-41B6-8C60-9044E59758DA}");
    class   FEEulerObject : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEEulerObject)
    public:
        FEEulerObject(FEContext& ctx)
            :FEObject(ctx)
        {
            /// _euler      =   quatToEuler(quat);
            _isChanged  =   false;
        }
        void    update(const quatf& quat)
        {
            _euler  =   quatToEuler(quat);
        }
    private:
        float3      _euler;
        bool        _isChanged;
    };

    using EulerHelperPtr = SharedPtr<FEEulerObject>;
}
