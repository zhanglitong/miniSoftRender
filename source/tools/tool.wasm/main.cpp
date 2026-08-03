
#ifdef     IS_EMSCRIPTEN
    #include    <emscripten.h>
    #include    <emscripten/bind.h>
    #include    <emscripten/em_asm.h>
    #include    <emscripten/heap.h>
#else
    
#endif

#include    <functional>
#include    <string>
#include    <vector>

#include    "FEObject.h"

std::string greet(const std::string& name) {
    return "Hello, " + name + "!";
}

std::vector<int> get_numbers() {
    return {1, 2, 3, 4, 5};
}


#ifdef  IS_EMSCRIPTEN


using   namespace   FE;
using   namespace   emscripten;

EMSCRIPTEN_SYMBOL(call_with_typed_array);
EMSCRIPTEN_SYMBOL(call_with_memory_view);
EMSCRIPTEN_SYMBOL(Uint8Array);


namespace emscripten 
{
    template<typename T> 
    struct  smart_ptr_trait<FERef<T>> 
    {
        typedef FE::FERef<T> pointer_type;
        typedef T element_type;

        static sharing_policy get_sharing_policy() 
        {
            return sharing_policy::NONE;
        }

        static T* get(const FE::FERef<T>& p) 
        {
            return (T*)(p.get());
        }

        static FE::FERef<T> share(const FE::FERef<T>& r, T* ptr) 
        {
            return FE::FERef<T>(ptr);
        }

        static pointer_type* construct_null() 
        {
            return new pointer_type;
        }
    };

}

#define     EXP_CLASS2(className,Type)          \
    class_<className>(#className)               \
        .constructor<>()                        \
        .constructor<Type,Type>()               \
        .property("x", &className::x)           \
        .property("y", &className::y)           \

#define     EXP_CLASS3(className,Type)          \
    class_<className>(#className)               \
        .constructor<>()                        \
        .constructor<Type,Type,Type>()          \
        .property("x", &className::x)           \
        .property("y", &className::y)           \
        .property("z", &className::z)           \

#define     EXP_CLASS4(className,Type)          \
    class_<className>(#className)               \
        .constructor<>()                        \
        .constructor<Type,Type,Type,Type>()     \
        .property("x", &className::x)           \
        .property("y", &className::y)           \
        .property("z", &className::z)           \
        .property("w", &className::w)           \

EMSCRIPTEN_BINDINGS(my_module) 
{
    EXP_CLASS2(short2,  short);
    EXP_CLASS2(ushort2, uint16_t);
    EXP_CLASS2(int2,    int);
    EXP_CLASS2(uint2,   uint32_t);
    EXP_CLASS2(float2,  float);
    EXP_CLASS2(real2,   double);

    EXP_CLASS3(short3,  short);
    EXP_CLASS3(ushort3, uint16_t);
    EXP_CLASS3(int3,    int);
    EXP_CLASS3(uint3,   uint32_t);
    EXP_CLASS3(float3,  float);
    EXP_CLASS3(real3,   double);

    EXP_CLASS4(short4,  short);
    EXP_CLASS4(ushort4, uint16_t);
    EXP_CLASS4(int4,    int);
    EXP_CLASS4(uint4,   uint);
    EXP_CLASS4(float4,  float);
    EXP_CLASS4(real4,   double);

    EXP_CLASS4(quatf,   float);
    EXP_CLASS4(quatr,   double);

    emscripten::function("greet", &greet);
    emscripten::function("get_numbers", &get_numbers);

    // 注册 vector<int> 类型（Embind 自动处理）
    emscripten::register_vector<int>("VectorInt");
}

#endif



int main(int argc,char** argv)
{
    return  0;
}