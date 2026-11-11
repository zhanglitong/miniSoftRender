#pragma     once
#include    "FEObject.h"
#include    "graphic/FEDSet.h"
#include    "graphic/FEPipeline.h"
#include    "graphic/FEDevice.h"
#include    "graphic/FEConstUuid.h"
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
        bool    isDirty() const
        {
            return  _dirty;
        }
        void    makeDirty()
        {
            _dirty  =   true;
        }
        /// <summary>
        /// 使用前检查
        /// </summary>
        void    checkDirty()
        {
            if (_dirty)
            {
                _dirty  =   false;
                update();
            }
        }
    public:
        UType       _value;
        GPUBuffer   _gpu;
        bool        _dirty  =   true;
    };
    template<typename UType,MemoryUsage usage = DEVICE_DEFAULT_BIT>
    class   TStorge
    {
    public:
        TStorge(FEContext& ctx)
        {
            _value  =   {};
            _gpu    =   ctx.device().createSBO();
            /// 追加 INDIRECT 用途:存储缓冲同时可能作为间接绘制的计数缓冲
            /// (drawArray/IndexedIndirectCount 的 countBuffer),缺少该用途时
            /// Vulkan 校验层会报 buffer usage 不匹配
            _gpu->create({sizeof(UType),usage,TRANSFER_DST_BIT | TRANSFER_SRC_BIT | INDIRECT_BUFFER_BIT});
        }
        void    update()
        {
            _dirty  =   false;
            _gpu->update(&_value,sizeof(_value),0);
        }
        bool    isDirty() const
        {
            return  _dirty;
        }
        void    makeDirty()
        {
            _dirty  =   true;
        }
        /// <summary>
        /// 使用前检查
        /// </summary>
        void    checkDirty()
        {
            if (_dirty)
            {
                update();
            }
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
        bool        _dirty  =   true;
    };

    class   FEDevice;
    DEFINE_CLASS_UUID(FEMaterial,"{E3982354-7576-4F9A-BC38-5697DB03EEB3}");
    class   FE_API  FEMaterial:public FEObject
    {
    public:
        using   DSets   =   std::vector<DSet>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterial)
    protected:
        DSets       _dsets      =   {};
        /// <summary>
        /// OIT 变体管线的描述符集(setupOITVariants 惰性构建):
        /// 与 _dsets 绑定同样的资源,额外含 OIT SSBO;为空表示无 OIT 变体
        /// </summary>
        DSets       _oitDsets   =   {};
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
        const   Pipeline    pipeline(EPrimitive pri) const
        {
            return  _pipelines[pri];
        }
        inline  Pipeline    pipeline(EPrimitive pri)
        {
            return  _pipelines[pri];
        }
        /// <summary>
        /// OIT 变体管线(LIST 几何趟绘制透明材质用):
        /// 默认无变体(返回 null);PBR 材质按需克隆主管线生成,
        /// OIT 材质本身就是 oitGeometry 管线,直接返回 pipeline()
        /// </summary>
        virtual Pipeline    oitPipeline(EPrimitive )
        {
            return  nullptr;
        }
        /// <summary>
        /// LIST(几何)趟使用的描述符集:必须与 oitPipeline 的布局匹配。
        /// 默认返回 OIT 变体 dsets(无变体时为空,渲染层跳过);
        /// OIT 材质的主管线即几何趟管线,重写为返回 _dsets
        /// </summary>
        virtual DSets&      listDsets()
        {
            return  _oitDsets;
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
        virtual bool        setup(const String& prefix);
        /// <summary>
        /// OIT 双趟渲染过滤标记:OIT 材质(LIST 趟收集进链表)返回 true,
        /// 普通材质(MAIN 趟正常画)返回 false。默认 false。
        /// </summary>
        virtual bool        isOIT() const { return false; }
        /// <summary>
        /// 透明物体标记(alpha 混合/transmission):渲染工厂据此保证透明组
        /// 在同 factory 内所有不透明组之后绘制,渲染层据此把透明 factory
        /// 的渲染优先级排到最后,避免透明面先画被后续不透明几何覆盖。
        /// </summary>
        virtual bool        isTransparent() const { return false; }
        /// <summary>
        /// 透射材质标记(KHR_materials_transmission 因子>0):FEScene 据此决定
        /// 是否需要先渲染不透明场景到 sceneTex 再做折射双趟。默认 false。
        /// </summary>
        virtual bool        isTransmission() const { return false; }
        /// <summary>
        /// 应用动态设置
        /// </summary>
        virtual void        appDynamicState(CMDPtr cmd,EPrimitive pri);
        /// <summary>
        /// 查询描述符
        /// </summary>
        /// <param name="set"></param>
        /// <returns></returns>
        DSet    query(uint16 set);
        /// <summary>
        /// 绑定数据到描述符，调用完成后，需要调用更新函数，把数据传给显卡。
        /// 若 OIT 变体 dsets 已构建(_oitDsets 非空),同样的绑定会同步写入,
        /// 保证 LIST 趟 OIT 管线采样到与主管线一致的资源
        /// </summary>
        /// <param name="set"></param>
        /// <param name="bind"></param>
        /// <param name="objects"></param>
        /// <returns></returns>
        bool    bind(uint16 set,uint16 bind,const Objects& objects,const uint64s& ranges = {},const uint64s& offsets = {});
        /// <summary>
        /// 绑定到指定的描述符集数组(bind 的底层实现,OIT 变体初始化用)
        /// </summary>
        bool    bindTo(DSets& dsets,uint16 set,uint16 bind,const Objects& objects,const uint64s& ranges = {},const uint64s& offsets = {});
        /// <summary>
        /// 绑定引擎全局 UBO(GlobalUBOId)到 SB_Global。
        /// 昼夜衰减、解析天空 IBL、选中高亮等所有材质共用的引擎级状态都从这里读取;
        /// 着色器未声明该绑定时返回 false,不影响其余资源
        /// </summary>
        bool    bindSkyEnvironment()
        {
            auto    globalUBO   =   _ctx.device().queryCache(FEConstUuid::GlobalUBOId);
            if  (globalUBO == nullptr)
                return  false;
            return  bind(0,SB_Global,   {globalUBO});
        }
        /// <summary>
        /// 数据上传到显卡，需要手动调用
        /// </summary>
        void    update();
    protected:
        /// <summary>
        /// 实现 ubo,texture 自动关联(到指定描述符集;autoAttach 同时处理两套)
        /// </summary>
        void    autoAttach();
        void    autoAttachTo(DSets& dsets);
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
