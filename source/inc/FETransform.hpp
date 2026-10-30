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
    ///   - 欧拉角辅助：euler() / setEuler() / fromEuler()
    ///     （除四元数外另存一份“累计欧拉角账本”:四元数只表示朝向,分不清 0°/720°,
    ///       账本用于属性面板显示、打帧与多圈插值;渲染/矩阵仍只以四元数为准）
    ///   - 矩阵生成：toMatrix() / toInverseMatrix() / fromMatrix()
    ///   - 数学方法：compose / inverted / operator*
    ///   - 混合方法：lerp / slerp / lerpTo
    /// 需要旋转中心点时使用 FETransformPivot
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
            t.setEuler(eulerDegrees);
            return  t;
        }

        /// <summary>
        /// 把角度按 360° 的整数倍平移,取离 reference 最近的等价角
        /// (用于把四元数换算出的 ±180° 结果展开到累计角度附近)
        /// </summary>
        static  real    unwrapAngle(real angle,real reference)
        {
            return  angle + 360.0 * std::round((reference - angle) / 360.0);
        }

        /// <summary>
        /// 位置（只读引用）
        /// </summary>
        inline  const real3&    position() const     {   return _position;   }
        /// <summary>
        /// 旋转（只读引用，始终为有效四元数，计算与矩阵生成统一使用它）
        /// </summary>
        inline  const quatf&    rotation() const      {   return _rotation;   }
        /// <summary>
        /// 缩放（只读引用）
        /// </summary>
        inline  const float3&   scale() const         {   return _scale;      }

        /// <summary>
        /// 设置位置，返回 *this 便于链式调用
        /// </summary>
        inline  FETransform&    setPosition(const real3& v)   { _position = v; return *this; }
        /// <summary>
        /// 设置旋转（四元数）。朝向发生变化时,累计欧拉角账本失效,退回 ±180° 换算;
        /// 朝向未变时保留账本,避免每帧无意义地丢失累计圈数
        /// </summary>
        inline  FETransform&    setRotation(const quatf& q)
        {
            if (q != _rotation)
                _eulerValid     =   false;
            _rotation       =   q;
            return  *this;
        }
        /// <summary>
        /// 用四元数设置旋转,同时把欧拉角账本展开到 reference 附近(保留累计圈数)。
        /// 适合抓手拖拽、任意轴增量旋转:位姿由四元数保证,角度由账本累计
        /// </summary>
        inline  FETransform&    setRotationUnwrapped(const quatf& q,const float3& reference)
        {
            _rotation       =   q;
            float3  e       =   glm::degrees(glm::eulerAngles(q));
            e.x             =   float(unwrapAngle(e.x,reference.x));
            e.y             =   float(unwrapAngle(e.y,reference.y));
            e.z             =   float(unwrapAngle(e.z,reference.z));
            _eulerDeg       =   e;
            _eulerValid     =   true;
            return  *this;
        }
        /// <summary>
        /// 设置缩放，返回 *this 便于链式调用
        /// </summary>
        inline  FETransform&    setScale(const float3& s)     { _scale = s;     return *this; }

        /// <summary>
        /// 返回欧拉角（单位：度）。账本有效时返回累计角(可超过 ±180°、可多圈),
        /// 否则由四元数实时换算(范围 ±180°)
        /// </summary>
        inline  float3          euler() const
        {
            return  _eulerValid ? _eulerDeg : glm::degrees(glm::eulerAngles(_rotation));
        }
        /// <summary>
        /// 累计欧拉角账本是否有效(与当前四元数一致)
        /// </summary>
        inline  bool            eulerValid() const  {   return  _eulerValid;   }

        /// <summary>
        /// 用欧拉角（单位：度）设置旋转;同时写入账本,不做 ±180° 归一化,
        /// 因此传入 720° 时账本保留 720°(四元数仍是等价朝向)
        /// </summary>
        inline  FETransform&    setEuler(const float3& eulerDegrees)
        {
            _rotation   =   quatf(glm::radians(eulerDegrees));
            _eulerDeg   =   eulerDegrees;
            _eulerValid =   true;
            return  *this;
        }

        /// <summary>
        /// 只同步欧拉角账本,不改变四元数。调用方需保证 eulerDegrees 与当前朝向等价
        /// (采样回写时使用:位姿已由四元数写入,这里把累计角一并带过去)
        /// </summary>
        inline  FETransform&    adoptEuler(const float3& eulerDegrees, bool valid = true)
        {
            _eulerDeg   =   eulerDegrees;
            _eulerValid =   valid;
            return  *this;
        }

        /// <summary>
        /// 生成 TRS 矩阵 (Translation * Rotation * Scale)
        /// </summary>
        inline  mat4r   toMatrix() const
        {
            return  FE::makeTransform<real>(_position, _scale, _rotation);
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
        static  FETransform     fromMatrix(const mat4r& mat)
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
        inline FETransform      compose(const FETransform& other) const
        {
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
        /// 线性混合：位置/缩放用 lerp
        /// 旋转统一走四元数 slerp(最短弧,无万向节死锁)
        /// </summary>
        static  FETransform   lerp(const FETransform& a, const FETransform& b, real t)
        {
            FETransform  r;
            r._position =   a._position + (b._position - a._position) * t;
            r._scale    =   a._scale + (b._scale - a._scale) * float(t);
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
                &&  _scale    == other._scale;
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
        /// 旋转（四元数，始终有效，计算与矩阵生成统一使用它）
        /// </summary>
        quatf   _rotation   =   quatf(1, 0, 0, 0);
        /// <summary>
        /// 缩放属性
        /// </summary>
        float3  _scale      =   float3(1);
        /// <summary>
        /// 累计欧拉角账本(单位:度,可超过 ±180°、可多圈)。
        /// 只有 _eulerValid 时有效,且必须与 _rotation 等价(允许相差 360° 整数倍);
        /// 渲染、矩阵、组合运算一律只用 _rotation
        /// </summary>
        float3  _eulerDeg   =   float3(0);
        /// <summary>
        /// 账本是否与当前四元数一致
        /// </summary>
        bool    _eulerValid =   false;
    };

}
