#version 450

// ============ 简单纹理测试 Shader ============
// binding 0: MVP UBO
// binding 1: combined image sampler (sampler2D)

layout  (location = 0)  in  vec3    inPos;
layout  (location = 1)  in  vec2    inUV;

layout  (binding = 0)   uniform  MVPBlock
{
    mat4    mvp;
};

layout  (binding = 1)   uniform  sampler2D  uTex;

layout  (location = 0)  out  vec2   outUV;

void main()
{
    outUV       =   inUV;
    gl_Position =   mvp * vec4(inPos.xyz, 1.0);
}
