#version 450

layout  (location = 0)  in  vec2    inUV;
layout  (location = 0)  out vec4    outColor;

// binding 1: combined image sampler (与 vert 里的 sampler2D 对应)
// 这里只声明 fragment 使用的那一端 —— 反射会自动合并成一个 DT_COMBINED_IMAGE_SAMPLER
layout  (binding = 1)   uniform  sampler2D  uTex;

void main()
{
    outColor =   texture(uTex, inUV);
}
