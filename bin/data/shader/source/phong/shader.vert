#version 450


#define     float2  vec2
#define     float3  vec3
#define     float4  vec4

#define     int2    ivec2
#define     int3    ivec3
#define     int4    ivec4

#define     uint2   uvec2
#define     uint3   uvec3
#define     uint4   uvec4
#define     quatf   vec4
#define     matrix4 mat4

layout  (location = 0)  in  vec3    aPos;
layout  (location = 1)  in  vec3    aNormal;
layout  (location = 2)  in  mat4    aModel;




struct  CameraInfo
{
    /// �۲���� view
    mat4        _v;
    /// ͶӰ���� projection
    mat4        _p;
    /// mv
    mat4        _vp;
    /// Offset MVP
    mat4        _offsetVp;
    /// <summary>
    /// camera position
    /// </summary>
    float4      _position;
    /// <summary>
    /// camera postion int part
    /// </summary>
    int4        _offset;
    /// <summary>
    /// up dir
    /// </summary>
    float4      _upDir;
    /// <summary>
    /// right dir
    /// </summary>
    float4      _rightDir;
};


layout (binding = 0) uniform UBOCamera 
{
	CameraInfo  _camera;
};

layout (location = 0) out vec3 outWorld;
layout (location = 1) out vec3 outNormal;


out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	vec4    world   =   aModel * vec4(aPos, 1.0);
    outNormal       =   mat3(transpose(inverse(aModel))) * aNormal;  
    outWorld        =   world.xyz;
	gl_Position     =   _camera._p * _camera._v  * world;
}
