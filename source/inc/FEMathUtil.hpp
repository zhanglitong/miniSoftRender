#pragma     once


#include    "FEMath.hpp"

namespace   FE
{
    template< typename T>
    tmat4<T>    makeTransform(tvec3<T> const & position, tvec3<T> const& scale, const tquat<T>& orientation)
    {
        tmat3<T> rot3   =   glm::mat3_cast(orientation);

        return  tmat4<T>(   scale.x * rot3[0][0], scale.x * rot3[0][1], scale.x * rot3[0][2], 0,
                            scale.y * rot3[1][0], scale.y * rot3[1][1], scale.y * rot3[1][2], 0,
                            scale.z * rot3[2][0], scale.z * rot3[2][1], scale.z * rot3[2][2], 0,
                            position.x, position.y, position.z, 1);
    }

    template<typename T>
    void        decompose(tmat4<T> mat, tvec3<T> &pos, tvec3<T> &scale, tquat<T> &quat)
    {
        T*  pData   =   (T*)&mat;
        pos.x       =   pData[12];
        pos.y       =   pData[13];
        pos.z       =   pData[14];
        //
        T   sx      =   FE::length(tvec3<T>(pData[0], pData[1], pData[2]));
        T   sy      =   FE::length(tvec3<T>(pData[4], pData[5], pData[6]));
        T   sz      =   FE::length(tvec3<T>(pData[8], pData[9], pData[10]));
        if (FE::determinant(mat) < 0)
        {
            sx  =   -sx;
        }
        scale   =   tvec3<T>(sx, sy, sz);
        T   invSX   =   T(1.0f) / sx;
        T   invSY   =   T(1.0f) / sy;
        T   invSZ   =   T(1.0f) / sz;
        pData[0]    *=  invSX;
        pData[1]    *=  invSX;
        pData[2]    *=  invSX;
        pData[4]    *=  invSY;
        pData[5]    *=  invSY;
        pData[6]    *=  invSY;
        pData[8]    *=  invSZ;
        pData[9]    *=  invSZ;
        pData[10]   *=  invSZ;
        T   m11     =   pData[0]; T m12 = pData[4]; T m13 = pData[8];
        T   m21     =   pData[1]; T m22 = pData[5]; T m23 = pData[9];
        T   m31     =   pData[2]; T m32 = pData[6]; T m33 = pData[10];
        T   trace   =   m11 + m22 + m33;
        T   s;
        T   m32_m23   =   m32 - m23;
        T   m13_m31   =   m13 - m31;
        T   m21_m12   =   m21 - m12;
        T   m12_m21   =   m12 + m21;
        T   m13Am31   =   m13 + m31;
        T   m23Am32   =   m23 + m32;
        if (trace > 0) 
        {
            s       =   T(0.5f) / sqrt(trace + T(1.0f));
            quat.w  =   T(0.25f) / s;
            quat.x  =   T(m32_m23) * s;
            quat.y  =   T(m13_m31) * s;
            quat.z  =   T(m21_m12) * s;
        }
        else if (m11 > m22 && m11 > m33) 
        {
            s       =   T(2.0f) * sqrt(T(1.0f) + m11 - m22 - m33);
            quat.w  =   T(m32_m23) / s;
            quat.x  =   T(0.25f) * s;
            quat.y  =   T(m12_m21) / s;
            quat.z  =   T(m13Am31) / s;
        }
        else if (m22 > m33) 
        {
            s       =   T(2.0f) * sqrt(T(1.0f) + m22 - m11 - m33);
            quat.w  =   T(m13_m31) / s;
            quat.x  =   T(m12_m21) / s;
            quat.y  =   T(0.25f) * s;
            quat.z  =   T(m23Am32) / s;
        }
        else 
        {
            s       =   T(2.0f) * (T)sqrt(T(1.0f) + m33 - m11 - m22);
            quat.w  =   T(m21_m12) / s;
            quat.x  =   T(m13Am31) / s;
            quat.y  =   T(m23Am32) / s;
            quat.z  =   T(0.25f) * s;
        }
    }

    /// <summary>
    /// 向上对齐，例如 alignUp(3,4),返回 4;
    /// </summary>
    /// <typeparam name="T"></typeparam>
    /// <param name="n"></param>
    /// <param name="alignment">对齐值4,8,16,32...</param>
    /// <returns></returns>
    template<typename T>
    constexpr   T   alignUp(T n, T alignment) 
    {
        return (n + alignment - 1) & ~(alignment - 1);
    }
}

