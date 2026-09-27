#pragma     once

#include    "../FEMath.hpp"
#include    "../FEFormat.hpp"
#include    "../FETransform.hpp"
#include    "FEGraphicEnums.h"

namespace   FE
{
    /// <summary>
    /// 该类是生成instance数据的工具类，是系统能提供instance的接口类
    /// </summary>
    struct  FEInstance
    {
        /// <summary>
        /// 前三个存储整数部分x,y,z;
        /// w 存储InstanceId
        /// 索引在做拾取的时候反馈使用，拾取后返回使用
        /// 并结合工厂Id,可以定位到拾取到的节点数据
        /// </summary>
        int     _intPartX;
        int     _intPartY;
        int     _intPartZ;
        int     _instanceId;
        /// <summary>
        /// 存储lod索引,默认值是-1,说明没有lod
        /// </summary>
        int     _lodIndex;
        uint32  _renderBits;
        /// <summary>
        /// 旋转信息
        /// </summary>
        float   _rotX;
        float   _rotY;
        float   _rotZ;
        float   _rotW;
        /// <summary>
        ///  包围球信息
        /// </summary>
        float   _boundSphereX;
        float   _boundSphereY;
        float   _boundSphereZ;
        float   _boundSphereR;
        float   _transX;
        float   _transY;
        float   _transZ;
        float   _scaleX;
        float   _scaleY;
        float   _scaleZ;
        Rgba8   _color;
#ifdef __cplusplus
        FEInstance()
        {
            _intPartX       =   0;
            _intPartY       =   0;
            _intPartZ       =   0;
            _instanceId     =   0;
            _lodIndex       =   -1;
            _renderBits     =   0;
            _rotX           =   0;
            _rotY           =   0;
            _rotZ           =   0;
            _rotW           =   1;
            _boundSphereX   =   0;
            _boundSphereY   =   0;
            _boundSphereZ   =   0;
            _boundSphereR   =   0;
            _transX         =   0;
            _transY         =   0;
            _transZ         =   0;
            _scaleX         =   1;
            _scaleY         =   1;
            _scaleZ         =   1;
        }
        inline  auto&   setLod(int lodId)
        {
            _lodIndex        =   lodId;
            return   *this;
        }
        /// <summary>
        /// 设置位置信息
        /// </summary>
        /// <param name="center">位置数据</param>
        /// <returns>自己</returns>
        inline  auto&   setTranslate(const real3& center)
        {
            _intPartX        =   int(center.x);
            _intPartY        =   int(center.y);
            _intPartZ        =   int(center.z);
            _transX          =   float(center.x - _intPartX);
            _transY          =   float(center.y - _intPartY);
            _transZ          =   float(center.z - _intPartZ);
            return   *this;
        }
        /// <summary>
        /// 设置位置信息
        /// </summary>
        /// <param name="center">位置数据</param>
        /// <returns>自己</returns>
        inline  auto&   setTranslate(const float3& center)
        {
            _intPartX   =   int(center.x);
            _intPartY   =   int(center.y);
            _intPartZ   =   int(center.z);
            _transX     =   float(center.x - _intPartX);
            _transY     =   float(center.y - _intPartY);
            _transZ     =   float(center.z - _intPartZ);
            return   *this;
        }
        /// <summary>
        /// 设置缩放数据
        /// </summary>
        /// <param name="scale"></param>
        /// <returns></returns>
        inline  auto&   setScale(const real3& scale)
        {
            _scaleX =   float(scale.x);
            _scaleY =   float(scale.y);
            _scaleZ =   float(scale.z);
            return  *this;
        }
        /// <summary>
        /// 设置缩放数据
        /// </summary>
        /// <param name="scale"></param>
        /// <returns></returns>
        inline  auto&   setScale(const float3& scale)
        {
            _scaleX =   scale.x;
            _scaleY =   scale.y;
            _scaleZ =   scale.z;
            return  *this;
        }
        inline  auto&   setRotation(const quatr& rot = quatr())
        {
            _rotX   =   float(rot.x);
            _rotY   =   float(rot.y);
            _rotZ   =   float(rot.z);
            _rotW   =   float(rot.w);
            return   *this;
        }
        inline  auto&   setRotation(const quatf& rot = quatf())
        {
            _rotX   =   rot.x;
            _rotY   =   rot.y;
            _rotZ   =   rot.z;
            _rotW   =   rot.w;
            return   *this;
        }
        /// <summary>
        /// 设置instance数据
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        inline  auto&   setInstance(uint index = 0)
        {
            _instanceId     =   index;
            return  *this;
        }
        /// <summary>
        /// 设置包围球数据
        /// </summary>
        /// <param name="box"></param>
        /// <returns></returns>
        inline  auto&   setAabb(const aabb3dr& box)
        {
            auto    vCenter =   box.getCenter();
            _boundSphereR   =   float(length(box.getSize()) * 0.5f);
            _boundSphereX   =   float(vCenter.x);
            _boundSphereY   =   float(vCenter.y);
            _boundSphereZ   =   float(vCenter.z);
            return  *this;
        }
        /// <summary>
        /// 设置包围球数据
        /// </summary>
        /// <param name="pos">中心点</param>
        /// <param name="radius">半径</param>
        /// <returns>自己</returns>
        inline  auto&   setBoundSphere(const float3& pos = float3(0),float radius = 1.0f)
        {
            _boundSphereX    =   pos.x;
            _boundSphereY    =   pos.y;
            _boundSphereZ    =   pos.z;
            _boundSphereR    =   radius;
            return  *this;
        }
        /// <summary>
        /// 使用给定 FETransform 填充 instance
        /// 位置拆分为整数部分(_intPart*)与小数部分(_trans*),
        /// 以减小大坐标下的浮点误差;缩放/旋转直接透传
        /// </summary>
        /// <param name="t">节点变换(位置+旋转+缩放)</param>
        /// <returns>自己</returns>
        inline  auto&   setTransform(const FETransform& t)
        {
            const real3&    pos =   t.position();
            /// 整数部分(向零取整,与相机偏移约定一致)
            _intPartX   =   int(pos.x);
            _intPartY   =   int(pos.y);
            _intPartZ   =   int(pos.z);
            /// 小数部分
            _transX     =   float(pos.x - _intPartX);
            _transY     =   float(pos.y - _intPartY);
            _transZ     =   float(pos.z - _intPartZ);
            return  setScale(t.scale()).setRotation(t.rotation());
        }
    public:
        
#endif
    };

#ifdef __cplusplus
    using   Instances   =   std::vector<FEInstance>;

    class   FEInstanceHelper
    {
    public:
        struct  InputDesc
        {
            /// <summary>
            /// 格式
            /// </summary>
            FEFormat    format  =   FMT_R32G32B32A32_FLOAT;
            /// <summary>
            /// 槽
            /// </summary>
            FEInputSlot slot    =   IS_VERTEX_POS;
            /// <summary>
            /// 当前类型的字节数量
            /// </summary>
            uint16      bytes   =   1;
        };
    public:
        inline  static  InputDesc   instanceInputs[]    =   
        {
            {FMT_R32G32B32_SINT         ,   IS_INSTANCE_ITRANSLATE  ,   FEFormatHelper::sizeOf(FMT_R32G32B32_SINT    )},
            {FMT_R32G32B32_FLOAT        ,   IS_INSTANCE_TRANSLATE   ,   FEFormatHelper::sizeOf(FMT_R32G32B32_FLOAT   )},
            {FMT_R32G32B32_FLOAT        ,   IS_INSTANCE_SCALE       ,   FEFormatHelper::sizeOf(FMT_R32G32B32_FLOAT   )},
            {FMT_R32G32B32A32_FLOAT     ,   IS_INSTANCE_ROTATE      ,   FEFormatHelper::sizeOf(FMT_R32G32B32A32_FLOAT )},
            {FMT_R32G32B32A32_FLOAT     ,   IS_INSTANCE_BOUNDSPHERE ,   FEFormatHelper::sizeOf(FMT_R32G32B32A32_FLOAT)},
            {FMT_R32_SINT               ,   IS_INSTANCE_INSTANCE    ,   FEFormatHelper::sizeOf(FMT_R32_SINT          )},
            {FMT_R32_SINT               ,   IS_INSTANCE_LOD_INDEX   ,   FEFormatHelper::sizeOf(FMT_R32_SINT          )},
            {FMT_R32_UINT               ,   IS_INSTANCE_FLAG        ,   FEFormatHelper::sizeOf(FMT_R32_UINT          )},
            {FMT_R8G8B8A8_UNORM         ,   IS_INSTANCE_COLOR       ,   FEFormatHelper::sizeOf(FMT_R8G8B8A8_UNORM)    }, 
            
        };
        inline  static  uint64      offsets[]   =   
        {
            offsetof(FEInstance,    _intPartX       ),
            offsetof(FEInstance,    _transX         ),
            offsetof(FEInstance,    _scaleX         ),
            offsetof(FEInstance,    _rotX           ),
            offsetof(FEInstance,    _boundSphereX   ),
            offsetof(FEInstance,    _instanceId     ),
            offsetof(FEInstance,    _lodIndex       ),
            offsetof(FEInstance,    _renderBits     ),
            offsetof(FEInstance,    _color          ),
        };
    };
#endif
}
