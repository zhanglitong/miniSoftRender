#pragma     once
#include    "FEObject.h"
#include    "graphic/FEDSet.h"
#include    "graphic/FEPipeline.h"
#include    "graphic/FEDevice.h"
#include    "graphic/FEShaderDefine.h"


namespace   FE
{
    template<typename UType,MemoryUsage usage = HOST_COHERENT_BIT>
    class   TBlock
    {
    public:
        TBlock(FEContext& ctx)
        {
            _gpu    =   ctx.device().createUBO();
            _gpu->create({sizeof(UType),usage});
        }
        void    update()
        {
            _gpu->update(&_value,sizeof(_value),0);
        }
    public:
        UType       _value;
        GPUBuffer   _gpu;
    };
    template<typename UType,MemoryUsage usage = DEVICE_DEFAULT_BIT>
    class   TStorge
    {
    public:
        TStorge(FEContext& ctx)
        {
            _value  =   {};
            _gpu    =   ctx.device().createSBO();
            _gpu->create({sizeof(UType),usage});
        }
        void    update()
        {
            _gpu->update(&_value,sizeof(_value),0);
        }
        /// <summary>
        /// 同步显卡数据到内存端
        /// </summary>
        /// <returns></returns>
        bool    gpu2cpu()
        {
            static_assert(usage == HOST_COHERENT_BIT);
            if constexpr (usage != HOST_COHERENT_BIT)
                return  false;
            auto    ptr =   _gpu->lock(sizeof(_value),0);
            if (ptr == nullptr)
                return  false;
            memcpy(&_value,ptr,sizeof(_value));
            _gpu->unlock();
            return  true;
        }
    public:
        UType       _value;
        GPUBuffer   _gpu;
    };

    class   FEDevice;
    DEFINE_CLASS_UUID(FEMaterial,"{E3982354-7576-4F9A-BC38-5697DB03EEB3}");
    class   FEMaterial:public FEObject
    {
    public:
        using   DSets   =   std::vector<DSet>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterial)
    protected:
        DSets       _dsets      =   {};
        /// <summary>
        /// vulkan 每一种图元类型是一种pipeline
        /// 有些显卡支持动态设置图元类型，有些不支持
        /// 如果支持的显卡,则所有的内容是一样的
        /// 如果不支持,则每一个都是不同的管线
        /// </summary>
        Pipeline    _pipelines[PRI_MAX] =   {};
        
    public:
        FEMaterial(FEContext& ctx)
            :FEObject(ctx)
        {}

        FEMaterial(const FEMaterial& other)
            :FEObject(other)
        {}
        virtual ~FEMaterial()
        {}
        template<typename TValue>
        inline  TValue*     constantAs() 
        {
            return  (TValue*)_const;
        }
        const   Pipeline    pipeline(EPrimitive pri) const
        {
            return  _pipelines[pri];
        }
        inline  Pipeline    pipeline(EPrimitive pri) 
        {
            return  _pipelines[pri];
        }
        const   DSets&      dsets() const
        {
            return  _dsets;
        }
        inline  DSets&      dsets()
        {
            return  _dsets;
        }

        /// <summary>
        /// 子类重写，基类使用，返回0,则不写入
        /// </summary>
        /// <returns></returns>
        virtual uint        pushConstantSize() const
        {
            return  0;
        }
        virtual PCVOID      pushConstantData() const
        {
            return  nullptr;
        }
        /// <summary>
        /// 该函数会根据前缀字符串 + /类型 查询pipeline
        /// 例如 v3c4,系统会自动生成 v3c4/nameOfEnum(EPrimitive);
        /// for (uint8 i = 0; i < EPrimitive::PRI_MAX; ++i)
        /// {
        ///   String      plName  =   "v3c4/" + String(nameOfEnum(EPrimitive(i)));
        /// }
        /// </summary>
        /// <param name="prefix"></param>
        /// <returns></returns>
        bool    setup(const String& prefix);
        /// <summary>
        /// 应用动态设置
        /// </summary>
        void    appDynamicState(CMDPtr cmd,EPrimitive pri);
        /// <summary>
        /// 查询描述符
        /// </summary>
        /// <param name="set"></param>
        /// <returns></returns>
        DSet    query(uint16 set);
        /// <summary>
        /// 绑定数据到描述符，调用完成后，需要调用更新函数，把数据传给显卡
        /// </summary>
        /// <param name="set"></param>
        /// <param name="bind"></param>
        /// <param name="objects"></param>
        /// <returns></returns>
        bool    bind(uint16 set,uint16 bind,const Objects& objects,const uint64s& ranges = {},const uint64s& offsets = {});
        /// <summary>
        /// 数据上传到显卡，需要手动调用
        /// </summary>
        void    update();
    protected:
        /// <summary>
        /// 实现 ubo,texture 自动关联
        /// </summary>
        void    autoAttach();
    };
    using   Material        =   SharedPtr<FEMaterial>;
    using   Materials       =   std::vector<Material>;
    using   MaterialUSet    =   std::unordered_set<Material>;
}

namespace   std
{
    template<>
    class   hash<FE::Material>
    {
    public:
        inline  size_t operator()(const FE::Material& key) const noexcept
        {
            std::hash<void*>    data;
            return  data((void*)key.get());
        }
    };

    template<>
    class   hash<FE::FEMaterial*>
    {
    public:
        inline  size_t operator()(const FE::FEMaterial* key) const noexcept
        {
            std::hash<void*>    data;
            return  data((void*)key);
        }
    };
}
