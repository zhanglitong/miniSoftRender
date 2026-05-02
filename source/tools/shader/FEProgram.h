#pragma     once
#include    <iostream>
#include    <stdio.h>
#include    <vector>
#include    <memory>

#include    "../../inc/define.h"

class   API_SHADER  IShader
{};

class   ILog;
using   ShaderPtr   =   std::shared_ptr<IShader>;
using   ShaderPtrs  =   std::vector<ShaderPtr>;
using   LogPtr      =   std::shared_ptr<ILog>;

class   API_SHADER  IProgram
{
public:
    using   ProgramPtr  =   std::shared_ptr<IProgram>;
public:
    static  ShaderPtr   createShader(const char* pSource,LogPtr log);
    static  ProgramPtr  link(const ShaderPtrs& shaders,LogPtr log);
};