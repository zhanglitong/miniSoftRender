#pragma     once

#include    "FEShader.h"

namespace   FE
{
   
    DEFINE_CLASS_UUID(FEProgram,"{06CE39E8-5615-44A8-A3C5-47A715E7FCD9}");
    class   FEProgram :public RSObject
    {
        IMPLEMENT_CLASS_REFLECT(FEProgram)
    public:
       
        struct  CreateInfo
        {
            ShaderPtr   _vs     =   nullptr;
            ShaderPtr   _ps     =   nullptr;

        };
        using   ProgramPtr  =   SharedPtr<FEProgram>;
    public:
        FEProgram(FEContext& ctx,FEDevice& device)
            :RSObject(ctx,device)
        {}
        FEProgram(const FEProgram& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        const auto& cInfo() const
        {
            return  _cInfo;
        }
        bool        create(const CreateInfo& info);
        bool        link(ShaderPtr vs,ShaderPtr ps);
    protected:
        CreateInfo  _cInfo;
    public:
        static  ProgramPtr  create(FEContext& ctx,const char* vs,const char* ps);
    
    };
    using   ProgramPtr  =   SharedPtr<FEProgram>;
    using   Programs    =   std::vector<ProgramPtr>;
}
