#pragma     once

#include    "FEConstraint.hpp"

namespace FE
{

    class   FEConstraintLookAt :public FEConstraint
    {
    public:
        /// <summary>
        /// 调用该函数，返回约束结果
        /// </summary>
        /// <param name="cur"></param>
        /// <param name="time"></param>
        /// <returns></returns>
        virtual FETransform   solve(const FETransform& currentWorld,const real&) const
        {
            (void)time;
            real3       forward =   normalize(targetPoint - currentWorld.position());
            real3       right   =   normalize(cross(upAxis, forward));
            real3       up      =   cross(forward, right);

            quatr       lookRot =   quatFromBasis(right, up, forward);
            FETransform desired =   currentWorld;
            desired.setRotation(quatf((lookRot * quatr(_offset.rotation())).normalized()));
            return      desired;
        }

    protected:
        /// <summary>
        /// 这是经典的 Shepperd 方法：根据矩阵迹（trace）的大小选择四个分支之一，避免除以接近零的数，数值上最稳定
        /// 由三个正交基向量构造四元数
        /// right, up, forward 为列向量，构成旋转矩阵 R
        /// </summary>
        /// <param name="right"></param>
        /// <param name="up"></param>
        /// <param name="forward"></param>
        /// <returns></returns>
        static  quatr   quatFromBasis(const real3& right, const real3& up, const real3& forward)
        {
            /// 矩阵元素（行主序）
            real    m00 = right.x,   m01 = up.x,   m02 = forward.x;
            real    m10 = right.y,   m11 = up.y,   m12 = forward.y;
            real    m20 = right.z,   m21 = up.z,   m22 = forward.z;

            real    trace = m00 + m11 + m22;
            quatr q;

            if (trace > 0.0f)
            {
                real s = std::sqrt(trace + 1.0f) * 2.0f;  // s = 4w
                q.w = 0.25f * s;
                q.x = (m21 - m12) / s;
                q.y = (m02 - m20) / s;
                q.z = (m10 - m01) / s;
            }
            else if (m00 > m11 && m00 > m22)
            {
                real s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;  // s = 4x
                q.w = (m21 - m12) / s;
                q.x = 0.25f * s;
                q.y = (m01 + m10) / s;
                q.z = (m02 + m20) / s;
            }
            else if (m11 > m22)
            {
                real s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;  // s = 4y
                q.w = (m02 - m20) / s;
                q.x = (m01 + m10) / s;
                q.y = 0.25f * s;
                q.z = (m12 + m21) / s;
            }
            else
            {
                real s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;  // s = 4z
                q.w = (m10 - m01) / s;
                q.x = (m02 + m20) / s;
                q.y = (m12 + m21) / s;
                q.z = 0.25f * s;
            }
            return q.normalized();
        }
    };
}

