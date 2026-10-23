#pragma     once

#include    "FEMath.hpp"
#include    "FEMathUtil.hpp"

namespace FE
{
    /// <summary>
    /// 节点变换：位置 + 旋转(四元数) + 缩放
    /// 提供：
    ///   - 成员访问：position() / rotation() / scale() (公开访问器)
    ///   - 设置器：setPosition / setRotation / setScale (链式)
    ///   - 欧拉角辅助：euler() / setEuler() / fromEuler()（按需从四元数换算,不另存）
    ///   - 矩阵生成：toMatrix() / toInverseMatrix() / fromMatrix()
    ///   - 数学方法：compose / inverted / operator*
    ///   - 混合方法：lerp / slerp / lerpTo
    ///   - 旋转中心点：绕本地点旋转(默认原点)
    /// </summary>
    class   FE_API  FETransform
    {
    public:
        friend  class   FENode;
        friend  class   FEAnimation;
    public:
        FETransform() = default;

        FETransform(const real3& pos, const quatf& rot, const float3& sc)
            : _position(pos)
            , _rotation(rot)
            , _scale(sc)
        {}

        /// <summary>
        /// 从欧拉角（单位：度）构造
        /// </summary>
        static  FETransform   fromEuler(const real3& pos, const float3& eulerDegrees, const float3& sc)
        {
            FETransform  t;
            t._position     =   pos;
            t._scale        =   sc;
            t._rotation     =   quatf(glm::radians(eulerDegrees));
            return  t;
        }

        /// <summary>
        /// 位置（只读引用）
        /// </summary>
        inline  const real3&   position() const     {   return _position;   }
        /// <summary>
        /// 旋转（只读引用，始终为有效四元数，计算与矩阵生成统一使用它）
        /// </summary>
        inline  const quatf&  rotation() const      {   return _rotation;   }
        /// <summary>
        /// 缩放（只读引用）
        /// </summary>
        inline  const float3& scale() const         {   return _scale;      }

        /// <summary>
        /// 旋转中心点(本地空间),几何体绕该点旋转,默认原点
        /// </summary>
        inline  const real3&   rotationCenter() const   {   return _pivot;  }
        /// <summary>
        /// 设置旋转中心点(本地空间),几何体将绕该点旋转
        /// </summary>
        inline  FETransform&   setRotationCenter(const real3& center)
        {
            _pivot  =   center;
            return  *this;
        }

        /// <summary>
        /// 设置位置，返回 *this 便于链式调用
        /// </summary>
        inline  FETransform&  setPosition(const real3& v)   { _position = v; return *this; }
        /// <summary>
        /// 设置旋转（四元数）
        /// </summary>
        inline  FETransform&  setRotation(const quatf& q)
        {
            _rotation   =   q;
            return  *this;
        }
        /// <summary>
        /// 设置缩放，返回 *this 便于链式调用
        /// </summary>
        inline  FETransform&  setScale(const float3& s)     { _scale = s;     return *this; }

        /// <summary>
        /// 返回欧拉角（单位：度），由四元数实时换算，范围 ±180
        /// </summary>
        inline  float3  euler() const
        {
            return  glm::degrees(glm::eulerAngles(_rotation));
        }

        /// <summary>
        /// 用欧拉角（单位：度）设置旋转
        /// </summary>
        inline  FETransform&   setEuler(const float3& eulerDegrees)
        {
            _rotation   =   quatf(glm::radians(eulerDegrees));
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
        /// 生成 TRS 逆矩阵
        /// </summary>
        inline  mat4r   toInverseMatrix() const
        {
            return  FE::inverse(toMatrix());
        }

        /// <summary>
        /// 从矩阵分解出 Transform
        /// </summary>
        /// <param name="mat"></param>
        /// <returns></returns>
        static  FETransform   fromMatrix(const mat4r& mat)
        {
            real3   pos;
            real3   scl;
            quatr   rot;
            FE::decompose<real>(mat, pos, scl, rot);
            FETransform  t;
            t._position =   pos;
            t._scale    =   float3(scl);
            t._rotation =   quatf(rot);
            return  t;
        }
#if 1
        /// <summary>
        /// 均匀缩放（最常见，可精确解析）s.x == s.y == s.z
        /// 设 A = (Pa, Ra, Sa), B = (Pb, Rb, Sb)，且 Sa、Sb 为均匀缩放
        /// 
        /// 复合 C = A * B （先应用 B 再应用 A，标准矩阵乘法顺序）：
        ///     C.position = Pa + Ra * (Sa * Pb)        /// A 变换 B 的原点
        ///     C.rotation = Ra * Rb                    /// 旋转直接相乘
        ///     C.scale    = Sa * Sb                    /// 均匀缩放直接相乘
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline FETransform compose(const FETransform& other) const
        {
            /// 存在旋转中心点时无法解析复合,退化为矩阵运算(结果中心点归零)
            if (_pivot != real3(0) || other._pivot != real3(0))
                return  fromMatrix(toMatrix() * other.toMatrix());
            // this = A, other = B，返回标准矩阵积 A * B
            FETransform r;
            r._rotation =   this->_rotation * other._rotation;
            r._scale    =   this->_scale * other._scale;          // 分量乘（均匀时正确）
            r._position =   this->_position + quatr(this->_rotation) * (real3(this->_scale) * other._position);
            return r;
        }
#else
        /// <summary>
        /// 复合：先应用 this，再应用 other (返回 other * this 的分解)
        /// </summary>
        inline  FETransform   compose(const FETransform& other) const
        {
            return  fromMatrix(other.toMatrix() * this->toMatrix());
        }
#endif 
        /// <summary>
        /// 逆变换
        /// </summary>
        inline  FETransform   inverted() const
        {
            return  fromMatrix(toInverseMatrix());
        }

        /// <summary>
        /// 线性混合：位置/缩放/中心点用 lerp
        /// 旋转统一走四元数 slerp(最短弧,无万向节死锁)
        /// </summary>
        static  FETransform   lerp(const FETransform& a, const FETransform& b, real t)
        {
            FETransform  r;
            r._position =   a._position + (b._position - a._position) * t;
            r._scale    =   a._scale + (b._scale - a._scale) * float(t);
            r._pivot    =   a._pivot + (b._pivot - a._pivot) * real(t);
            r._rotation =   FE::slerp(a._rotation, b._rotation, float(t));
            return  r;
        }

        /// <summary>
        /// 球面线性混合（与 lerp 等价，命名用于语义清晰）
        /// </summary>
        static  FETransform   slerp(const FETransform& a, const FETransform& b, real t)
        {
            return  lerp(a, b, t);
        }

        /// <summary>
        /// 从 this 混合到 other，参数 t
        /// </summary>
        inline  FETransform   lerpTo(const FETransform& other, real t) const
        {
            return  lerp(*this, other, t);
        }

        /// <summary>
        /// 复合运算符：标准矩阵乘法 a * b（先应用 b 再应用 a）
        /// </summary>
        inline  FETransform   operator*(const FETransform& other) const
        {
            return  compose(other);
        }

        inline  FETransform&  operator*=(const FETransform& other)
        {
            *this   =   compose(other);
            return  *this;
        }

        inline  bool   operator==(const FETransform& other) const
        {
            return  _position == other._position
                &&  _rotation == other._rotation
                &&  _scale    == other._scale
                &&  _pivot    == other._pivot;
        }

        inline  bool   operator!=(const FETransform& other) const
        {
            return  !(*this == other);
        }

    protected:
        /// <summary>
        /// 位置属性
        /// </summary>
        real3   _position   =   real3(0);
        /// <summary>
        /// 旋转中心点（本地空间），几何体绕该点旋转，默认原点
        /// </summary>
        real3   _pivot      =   real3(0);
        /// <summary>
        /// 旋转（四元数，始终有效，计算与矩阵生成统一使用它）
        /// </summary>
        quatf   _rotation   =   quatf(1, 0, 0, 0);
        /// <summary>
        /// 缩放属性
        /// </summary>
        float3  _scale      =   float3(1);
    };

}
