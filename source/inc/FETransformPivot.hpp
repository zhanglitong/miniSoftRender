#pragma     once

#include    "FETransform.hpp"

namespace FE
{
    /// <summary>
    /// 带旋转中心点的变换：位置 + 旋转(四元数) + 缩放 + 旋转中心
    /// 在 FETransform 基础上增加 _pivot,几何体绕本地旋转中心点旋转
    /// 未设置中心点(默认原点)时计算与 FETransform 完全一致
    /// </summary>
    class   FE_API  FETransformPivot : public FETransform
    {
    public:
        friend  class   FENode;
        friend  class   FEAnimation;
    public:
        FETransformPivot() = default;

        FETransformPivot(const real3& pos, const quatf& rot, const float3& sc)
            : FETransform(pos, rot, sc)
        {}

        /// <summary>
        /// 从欧拉角（单位：度）构造
        /// </summary>
        static  FETransformPivot   fromEuler(const real3& pos, const float3& eulerDegrees, const float3& sc)
        {
            FETransformPivot  t;
            t._position     =   pos;
            t._scale        =   sc;
            t.setEuler(eulerDegrees);
            return  t;
        }

        /// <summary>
        /// 旋转中心点(本地空间),几何体绕该点旋转,默认原点
        /// </summary>
        inline  const real3&   rotationCenter() const   {   return _pivot;  }
        /// <summary>
        /// 设置旋转中心点(本地空间),几何体将绕该点旋转
        /// </summary>
        inline  FETransformPivot&   setRotationCenter(const real3& center)
        {
            _pivot  =   center;
            return  *this;
        }

        /// <summary>
        /// 设置位置，返回 *this 便于链式调用
        /// </summary>
        inline  FETransformPivot&  setPosition(const real3& v)   { _position = v;    return *this; }
        /// <summary>
        /// 设置旋转（四元数）。朝向发生变化时,累计欧拉角账本失效,退回 ±180° 换算;
        /// 朝向未变时保留账本(与 FETransform 语义一致)
        /// </summary>
        inline  FETransformPivot&  setRotation(const quatf& q)
        {
            if (q != _rotation)
                _eulerValid     =   false;
            _rotation       =   q;
            return  *this;
        }
        /// <summary>
        /// 设置缩放，返回 *this 便于链式调用
        /// </summary>
        inline  FETransformPivot&  setScale(const float3& s)     { _scale = s;       return *this; }
        /// <summary>
        /// 用欧拉角（单位：度）设置旋转;同时写入累计欧拉角账本,不做 ±180° 归一化,
        /// 因此传入 720° 时账本保留 720°(四元数仍是等价朝向)
        /// </summary>
        inline  FETransformPivot&  setEuler(const float3& eulerDegrees)
        {
            _rotation   =   quatf(glm::radians(eulerDegrees));
            _eulerDeg   =   eulerDegrees;
            _eulerValid =   true;
            return  *this;
        }

        /// <summary>
        /// 生成 TRS 矩阵 (Translation * Rotation * Scale)
        /// </summary>
        inline  mat4r   toMatrix() const
        {
            if (_pivot.x == real(0) && _pivot.y == real(0) && _pivot.z == real(0))
                return  FE::makeTransform<real>(_position, _scale, _rotation);
            /// 绕本地旋转中心点旋转: T(position) * T(pivot) * R * T(-pivot) * S
            mat4r   mat =   glm::translate(mat4r(1), _position);
            mat         =   glm::translate(mat, _pivot);
            mat         =   mat * glm::mat4_cast(quatr(_rotation));
            mat         =   glm::translate(mat, -_pivot);
            mat         =   glm::scale(mat, real3(_scale));
            return  mat;
        }

        /// <summary>
        /// 生成 TRS 逆矩阵(以中心点旋转后的矩阵求逆)
        /// </summary>
        inline  mat4r   toInverseMatrix() const
        {
            return  FE::inverse(toMatrix());
        }

        /// <summary>
        /// 从矩阵分解出 Transform
        /// </summary>
        static  FETransformPivot   fromMatrix(const mat4r& mat)
        {
            real3   pos;
            real3   scl;
            quatr   rot;
            FE::decompose<real>(mat, pos, scl, rot);
            FETransformPivot  t;
            t._position =   pos;
            t._scale    =   float3(scl);
            t._rotation =   quatf(rot);
            return  t;
        }

        /// <summary>
        /// 复合: 均匀缩放时解析计算,存在旋转中心点时退化为矩阵运算
        /// 设 A = (Pa, Ra, Sa), B = (Pb, Rb, Sb)
        ///     C.position = Pa + Ra * (Sa * Pb)
        ///     C.rotation = Ra * Rb
        ///     C.scale    = Sa * Sb
        /// </summary>
        inline FETransformPivot compose(const FETransformPivot& other) const
        {
            if (_pivot != real3(0) || other._pivot != real3(0))
                return  fromMatrix(toMatrix() * other.toMatrix());
            FETransformPivot r;
            r._rotation =   this->_rotation * other._rotation;
            r._scale    =   this->_scale * other._scale;
            r._position =   this->_position + quatr(this->_rotation) * (real3(this->_scale) * other._position);
            return  r;
        }

        /// <summary>
        /// 逆变换
        /// </summary>
        inline  FETransformPivot   inverted() const
        {
            return  fromMatrix(toInverseMatrix());
        }

        /// <summary>
        /// 线性混合：位置/缩放/中心点用 lerp
        /// 旋转统一走四元数 slerp(最短弧,无万向节死锁)
        /// </summary>
        static  FETransformPivot   lerp(const FETransformPivot& a, const FETransformPivot& b, real t)
        {
            FETransformPivot  r;
            r._position =   a._position + (b._position - a._position) * t;
            r._scale    =   a._scale + (b._scale - a._scale) * float(t);
            r._pivot    =   a._pivot + (b._pivot - a._pivot) * real(t);
            r._rotation =   FE::slerp(a._rotation, b._rotation, float(t));
            return  r;
        }

        /// <summary>
        /// 球面线性混合（与 lerp 等价，命名用于语义清晰）
        /// </summary>
        static  FETransformPivot   slerp(const FETransformPivot& a, const FETransformPivot& b, real t)
        {
            return  lerp(a, b, t);
        }

        /// <summary>
        /// 从 this 混合到 other，参数 t
        /// </summary>
        inline  FETransformPivot   lerpTo(const FETransformPivot& other, real t) const
        {
            return  lerp(*this, other, t);
        }

        /// <summary>
        /// 复合运算符：标准矩阵乘法 a * b（先应用 b 再应用 a）
        /// </summary>
        inline  FETransformPivot   operator*(const FETransformPivot& other) const
        {
            return  compose(other);
        }

        inline  FETransformPivot&  operator*=(const FETransformPivot& other)
        {
            *this   =   compose(other);
            return  *this;
        }

        inline  bool   operator==(const FETransformPivot& other) const
        {
            return  _position == other._position
                &&  _rotation == other._rotation
                &&  _scale    == other._scale
                &&  _pivot    == other._pivot;
        }

        inline  bool   operator!=(const FETransformPivot& other) const
        {
            return  !(*this == other);
        }

    protected:
        /// <summary>
        /// 旋转中心点（本地空间），几何体绕该点旋转，默认原点
        /// </summary>
        real3   _pivot      =   real3(0);
    };

}
