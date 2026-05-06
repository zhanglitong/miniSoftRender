#pragma     once
#include    <iostream>
#include    <stdio.h>
#include    <vector>
#include    <memory>

#include    "../../inc/FEDefine.h"
#include    "../../inc/FELog.h"
#include    "../../inc/FEObject.h"
#include	"../../inc/FESmallVector.h"
#include    "../../inc/FEMath.hpp"

/// <summary>
/// shader 与 渲染系统之间的接口粘合剂
/// </summary>
/// 
namespace   FE
{
    struct  FEShaderRT;
}
extern  "C"
{
    struct  IShader
    {
        FE::FEShaderRT* (*createShader )(void);
        void            (*destroyShader)(FE::FEShaderRT*   pThis);
        void            (*invokeShader )(FE::FEShaderRT*   pThis);
    };
}

namespace   FE
{
    const   unsigned    C_NUM_BINDING   =  8;
    const   unsigned    C_NUM_INPUT     =  8;
    const   unsigned    C_NUM_OUTPUT    =  8;
    const   unsigned    C_NUM_UNIFORM   =  8;
    const   unsigned    C_NUM_BUILDIN   =  4;

    enum    ShaderBuiltin :uint32_t
    {
        SB_POSITION     =   0,
        SB_FRAG_COORD   =   1,
        SB__NUM_BUILTINS
    };
    template <typename T>
    struct  Interface
    {
        enum
        {
            ArraySize   =   1,
            Size        =   sizeof(T)
        };

        Interface()
            : ptr(nullptr)
        {
        }
        T&  get()
        {
            assert(ptr);
            return *ptr;
        }

        T*  ptr;
    };

    template <typename T>
    struct  IInput : Interface<T>
    {};
    template <typename T>
    struct  IOutput : Interface<T>
    {};
    template <typename T>
    struct  IUniform : Interface<T>
    {};
    struct	FEShaderRT
	{
        struct   Pointers
        {
        	Pointers()
        		: ptr(nullptr)
        		, size(0)
        	{}
            void**  ptr;
            size_t  size;
        };
        Pointers    _resource[C_NUM_BINDING];
        Pointers    _input  [C_NUM_INPUT];
        Pointers    _output [C_NUM_OUTPUT];
        Pointers    _uniform[C_NUM_UNIFORM];
        Pointers    _builtin[C_NUM_BUILDIN];

        template <typename U>
        void    regBuiltin(ShaderBuiltin builtin, const U &value)
        {
            assert(!_builtin[builtin].ptr);

            _builtin[builtin].ptr   =   (void **)&value.ptr;
            _builtin[builtin].size  =   sizeof(*value.ptr) * U::ArraySize;
        }
        template <typename U>
        void    regInput  (const IInput<U>&  value, unsigned location)
        {
            assert(location < C_NUM_INPUT);
            assert(!_input[location].ptr);

            _input[location].ptr  = (void **)&value.ptr;
            _input[location].size = IInput<U>::Size;
        }
        template <typename U>
        void    regOutput (const IOutput<U>& value, unsigned location)
        {
            assert(location < C_NUM_OUTPUT);
            assert(!_output[location].ptr);

            _output[location].ptr   =   (void **)&value.ptr;
            _output[location].size  =   IOutput<U>::Size;
        }
        template <typename U>
        void    regUniform(const IUniform<U>&value, unsigned location)
        {
            assert(location < C_NUM_UNIFORM);
            assert(!_uniform[location].ptr);

            _uniform[location].ptr  =   (void **)&value.ptr;
            _uniform[location].size =   IUniform<U>::Size;
        }

        void    setResource(unsigned binding,void **data, size_t size)
        {
            assert(binding < C_NUM_BINDING);
            assert(_resource[binding].ptr);
            assert(size >= _resource[binding].size);
            _resource[binding].ptr  =   data;
        }

        void    setInput  (unsigned location, void*data, size_t size)
        {
            assert(_input[location].ptr);
            assert(size >= _input[location].size);
            *_input[location].ptr   =   data;
        }
        void    setOutput (unsigned location, void*data, size_t size)
        {
            assert(_output[location].ptr);
            assert(size >= _output[location].size);
            *_output[location].ptr  =   data;
        }
        void    setuniform(unsigned location, void*data, size_t size)
        {
            assert(_uniform[location].ptr);
            assert(size >= _uniform[location].size);
            *_uniform[location].ptr =   data;
        }
        void    setBuiltin(ShaderBuiltin builtin, void* data, size_t size)
        {
            assert(_builtin[builtin].ptr);
            assert(size >= _builtin[builtin].size);
            *_builtin[builtin].ptr  =   data;
        }
	};

    template <typename T>
    struct  TShader : public FEShaderRT
    {
        void    invoke()
        {
            static_cast<T*>(this)->main();
        }
    };

    struct  VSResource
    {
        IInput<float4>  gl_Position;

        void    init(FEShaderRT& rt)
        {
            rt.regBuiltin(SB_POSITION, gl_Position);
        }
    };

    struct  FSResource
    {
        IOutput<float4> gl_FragCoord;
        void    init(FEShaderRT&   rt)
        {
            rt.regBuiltin(SB_FRAG_COORD, gl_FragCoord);
        }
    };
    
    /// <summary>
    /// shader 接口部分
    /// </summary>
    /// <typeparam name="TImplement">具体shader实现 c++ 版本 </typeparam>
    /// <typeparam name="TResource"> shader资源对象</typeparam>
    template <typename TImplement, typename TResource>
    struct  TFragmentShader : TShader<TFragmentShader<TImplement, TResource>>
    {
        inline void main()
        {
            _implement.main();
        }

        TFragmentShader()
        {
            _resource.init(*this);
            _implement.setResource(&_resource);
        }
    public:
        TImplement  _implement;
        TResource   _resource;
    };

    template <typename TImplement, typename TResource>
    struct  TVertexShader : TShader<TVertexShader<TImplement, TResource>>
    {
        inline void main()
        {
            _implement.main();
        }
        TVertexShader()
        {
            _resource.init(*this);
            _implement.setResource(&_resource);
        }
    public:
        TImplement  _implement;
        TResource   _resource;
    };

    struct  FEShaderMgr
    {
    public:
    };

    template<class CLASS>
    struct  ExportShader :public std::false_type
    {};

    #define DEFINE_SHADER(CLASS,shaderName)                             \
    template<>  struct FE::ExportShader<CLASS> :public std::true_type   \
    {                                                                   \
        static  const char* name()                                      \
        {                                                               \
            return  #shaderName;                                        \
        }                                                               \
        static  FE::FEShaderRT* createShader(void)                      \
        {                                                               \
            return new CLASS();                                         \
        }                                                               \
        static  void            destroyShader(FE::FEShaderRT*   shader) \
        {                                                               \
            delete static_cast<CLASS*>(shader);                         \
        }                                                               \
        static  void            invokeShader(FE::FEShaderRT*    shader) \
        {                                                               \
            static_cast<CLASS*>(shader)->invoke();                      \
        }                                                               \
        static const IShader*   shaderInterface()                       \
        {                                                               \
            static  IShader sShader =                                   \
            {                                                           \
                createShader,                                           \
                destroyShader,                                          \
                invokeShader                                            \
            };                                                          \
            return  &sShader;                                           \
        }                                                               \
    }

}