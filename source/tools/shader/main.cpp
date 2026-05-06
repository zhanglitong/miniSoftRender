
#include    <iostream>
#include    <stdio.h>
#include    "FEShaderRT.h"
#include    "SampleVS.h"
void    print(const char* msg)
{
    printf("%s",msg);
    /// 通过一下方法获取 name 与接口
    const auto  name    =   FE::ExportShader<SampleVS>::name();
    const auto  api     =   FE::ExportShader<SampleVS>::shaderInterface();
    /// 可以加入到渲染系统中
}