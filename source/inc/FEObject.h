#pragma     once

#include    <type_traits>
#include    <functional>
#include    <unordered_set>
#include    <variant>
#include    <array>
#include    "FEDefine.h"
#include    "FEReference.hpp"
#include    "FEUuid.h"
#include    "FEFlags.hpp"
#include    "FEMath.hpp"
#include    "FEKeyValues.hpp"
#include    "FEChunkInf.hpp"
#include    "FESerializeCtx.h"

namespace   FE
{
    class   FEWriter;
    class   FEReader;
    class   FEContext;
    class   FEAllocator;
    class   FEObject;

    using   Object          =   SharedPtr<FEObject>;
    using   CLSId           =   FEUuid;
    using   OBJId           =   FEUuid;
    using   FECreator       =   std::function<Object(FEContext&,const FEAllocator&)>;
    using   CLSVar          =   std::variant<std::monostate,bool,int16,uint16,int32,uint32,int64,uint64,float,float2,float3,float4,real,real2,real3,real4,String,Strings,Object,FEUuid>;
    using   CLSProp         =   FEKeyValues<String,CLSVar>;
    
    struct  CLSProperty                                        
    {                                                           
        CLSProperty(const CLSId& id)                           
        {                                                           
            _id =   id;                                             
        }  
        /// <summary>
        /// 类ID
        /// </summary>
        /// <returns></returns>
        const   CLSId&      classId() const
        {
            return  _id;
        }
        /// <summary>
        /// 类属性
        /// </summary>
        /// <returns></returns>
        inline  CLSProp&    property()
        {
            return  _property;
        }
    protected:
        CLSId       _id;                                        
        CLSProp     _property;                                  
    };                                                          

    extern  void    systemRegCreator(const FEUuid& id,const FECreator& creator);

    template<class CLASS>
    struct  UUIDTraits :public std::false_type  
    {
        static CLSProperty&    property()
        {   
            static  CLSProperty sData(FEUuid::zero());
            return  sData;
        }
    };


    #define UNUSED(x) (void)(x)

    #define UUIDOF(CLASS)       FE::UUIDTraits<CLASS>::property().classId()

    #define CLS_PROPERTY(CLASS) FE::UUIDTraits<CLASS>::property().property()


    #define DEFINE_CLASS_UUID(CLASS, uuid)                          \
    class   CLASS;                                                  \
    template <> struct UUIDTraits<CLASS> : public std::true_type    \
    {                                                               \
    public:                                                         \
        static  CLSProperty&    property()                          \
        {                                                           \
            static  CLSProperty sData(FEUuid::from(uuid));          \
            return  sData;                                          \
        }                                                           \
    }                                                               \

    ///
    /// 统一使用该宏定义,继承了FEObject 的类使用
    /// 不包含clone函数,如果有需要自定义实现clone函数的情况下
    /// 使用该宏定义
    ///
    #define IMPLEMENT_CLASS_REFLECT_MINI(class_name)                \
    public:                                                         \
        virtual const char*     className() const override          \
        {                                                           \
            return  #class_name;                                    \
        }                                                           \
        virtual const CLSId&    classId() const override            \
        {                                                           \
            return  UUIDOF(class_name);                             \
        }                                                           \
       virtual const CLSProp&  property() const  override           \
        {                                                           \
            return  CLS_PROPERTY(class_name);                       \
        }                                                           \
        virtual CLSProp&        property() override                 \
        {                                                           \
            return  CLS_PROPERTY(class_name);                       \
        }                                                           \


    ///
    /// 统一使用该宏定义,继承了FEObject 的类使用
    ///
    #define IMPLEMENT_CLASS_REFLECT(class_name)                     \
    public:                                                         \
        virtual const char*     className() const override          \
        {                                                           \
            return  #class_name;                                    \
        }                                                           \
        virtual const CLSId&    classId() const override            \
        {                                                           \
            return  UUIDOF(class_name);                             \
        }                                                           \
        virtual Object          clone() const  override             \
        {                                                           \
            return  new class_name(*this);                          \
        }                                                           \
        virtual const CLSProp&  property() const  override          \
        {                                                           \
            return  CLS_PROPERTY(class_name);                       \
        }                                                           \
        virtual CLSProp&        property() override                 \
        {                                                           \
            return  CLS_PROPERTY(class_name);                       \
        }                                                           \

    
    #define DEFINE_CLASS_CREATOR(class_name)                        \
        systemRegCreator( UUIDOF(class_name)                        \
                        ,[](FEContext&ctx,const FEAllocator&)   {   \
        return  new class_name(ctx);                                \
        })                                                          \

    #define USING_PLUGIN(name)                                      \
    extern          uint    loadPlugin_##name();                    \
    static const    uint    PLUGIN_##name   =  loadPlugin_##name()  \


    DEFINE_CLASS_UUID(FEObject,"{00000000-1111-2222-3333-0123456789AB}");

    enum    FlagBit :uint32_t
    {
        /// <summary>
        /// 是否可见
        /// </summary>
        FLAG_VISIBLE        =   (1<<0),
        /// <summary>
        /// 是否需要更新
        /// </summary>
        FLAG_UPDATE         =   ((FLAG_VISIBLE) <<1 ),
        /// <summary>
        /// 是否支持是否可以被拾取
        /// </summary>
        FLAG_PICKUP         =   ((FLAG_UPDATE)  <<1 ),
        /// <summary>
        /// 是否被选中
        /// </summary>
        FLAG_SELECTED       =   ((FLAG_PICKUP)  <<1 ),
        /// <summary>
        /// 是否支持序列化
        /// </summary>
        FLAG_SERIAL         =   ((FLAG_SELECTED)<<1 ),
        /// <summary>
        /// 状态
        /// </summary>
        FLAG_EXPAND         =   ((FLAG_SERIAL)<<1 ),
        /// <summary>
        /// 添加对象
        /// </summary>
        FLAG_ADD_CHILD      =   ((FLAG_EXPAND)<<1),
        /// <summary>
        /// 移除对象
        /// </summary>
        FLAG_REMOVE_CHILD   =   ((FLAG_ADD_CHILD)<<1),
        /// <summary>
        /// 对象修改
        /// </summary>
        FLAG_MODIFY_CHILD   =   ((FLAG_REMOVE_CHILD)<<1),
        /// <summary>
        /// 必须在最后，其他的子定义会从该值开始
        /// </summary>
        FLAG_LAST           =   (FLAG_MODIFY_CHILD<<1)
    };

    using   Flags       =   FEFlags<FlagBit,int32_t>;


   
    class   FE_API  FEObject :public FEReference<Flags>
    {
    public:
        using   Object      =   SharedPtr<FEObject>;
        using   Objects     =   std::vector<Object>;
        using   ObjectUSet  =   std::unordered_set<Object>;
    public:
        FEObject(FEContext& ctx,bool genId = true);
        FEObject(const FEObject& other);
        virtual ~FEObject()   =   default;
    public:
        /// <summary>
        /// 类Id
        /// </summary>
        /// <returns></returns>
        virtual const CLSId&    classId() const;
        /// <summary>
        /// 类名
        /// </summary>
        /// <returns></returns>
        virtual const char*     className() const;
        /// <summary>
        /// 函数返回不是对象的属性，是该类的属性，全局静态变量，所有实例共享一份数据
        /// 典型业务应用场景:类别名，类的图标...
        /// 这里之所以不是静态函数，目的是方便使用，使用者不关注对象类型
        /// </summary>
        /// <returns></returns>
        virtual const CLSProp&  property() const;  
        virtual CLSProp&        property();               
        /// <summary>
        /// 克隆对象
        /// </summary>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual Object          clone() const;    
        inline  FEContext&      ctx()       {   return  _ctx;   }
        const   FEContext&      ctx() const {   return  _ctx;   }
        /// <summary>
        /// 是否支持序列化
        /// </summary>
        /// <returns></returns>
        inline  bool        supportSerial() const
        {
            return  flags().hasFlag(FLAG_SERIAL);
        }
        /// <summary>
        /// 对象Id
        /// </summary>
        /// <returns></returns>
        const   OBJId&      objectId()  const
        {
            return  _id;
        }
        /// <summary>
        /// 设置对象Id
        /// </summary>
        /// <param name="id"></param>
        inline  void        setObjectId(const OBJId& id)
        {
            _id =   id;
        }
        /// <summary>
        /// 标记,对象的状态标志(只读),
        /// </summary>
        /// <returns></returns>
        const   Flags&      flags()  const
        {
            return  _flags;
        }
        /// <summary>
        /// 标记,对象的状态标志(读写)
        /// </summary>
        /// <returns></returns>
        inline  Flags&      flags()
        {
            return  _flags;
        }
        template<typename TFlag>
        const  TFlag&       flagsAs() const
        {
            return  (TFlag&)_flags;
        }
        template<typename TFlag>
        inline  TFlag&      flagsAs() 
        {
            return  (TFlag&)_flags;
        }
        /// <summary>
        /// 写入,返回写入的字节数
        /// 基类中实现数据头的写入，以及自身的数据写入
        /// 典型用例: 
        ///         1.子类重写，实现对数据的读取控制
        ///         2.子类重写serializeTraits,只关注自己需要写入的数据，无需关注其他
        /// </summary>
        /// <returns>写入的字节数</returns>
        virtual uint64      serialize(FEWriter& writer,uint& version,FESerializeCtx& ctx) const;
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="chunk">数据头，子类可根据情况修改(flags字段)，实现一些优化处理</param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const;
        /// <summary>
        /// 读取,返回读取的字节数
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        virtual uint64      deserialize(FEReader& reader,uint& version,FESerializeCtx& ctx);
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="chunk">数据头，子类可根据chunk._flags字段控制读取</param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx);
        /// <summary>
        /// 根据类型id获取接口信息
        /// </summary>
        /// <param name="classId"></param>
        /// <returns></returns>
        virtual Object      queryInterface(const CLSId& classId);
        /// <summary>
        /// 对象的初始化版本,子类可以升级重写,但不能低于初始化版本
        /// </summary>
        /// <returns></returns>
        virtual uint        version() const;
        /// <summary>
        /// 获取依赖的对象,子类实现
        /// </summary>
        /// <param name="uset"></param>
        /// <returns>返回以来的对象个数</returns>
        virtual size_t      queryDepends(ObjectUSet& uSet) const;
        /// <summary>
        /// 通用设置对象属性接口，子类实现
        /// </summary>
        virtual void        beginSetProp()
        {}
        /// <summary>
        /// 设置属性
        /// </summary>
        /// <param name="prop">属性索引(别名)</param>
        /// <param name="value">属性值</param>
        /// <returns>true,表示修改成功,否则没有修改</returns>
        virtual bool        setProperty(int prop,const CLSVar& value)
        {
            UNUSED(prop);
            UNUSED(value);
            return  false;
        }
        /// <summary>
        /// @ref setProperty 返回结果作为输入参数，用来决定是否需要更新操作
        /// </summary>
        /// <param name="bModify"></param>
        virtual void        endSetProp(bool bModify)
        {}
        /// <summary>
        /// 直接转换，有风险，在确定的情况下使用，性能更好
        /// </summary>
        /// <typeparam name="TAsTo"></typeparam>
        /// <returns></returns>
        template<typename TAsTo>
        inline  TAsTo*      as()
        {
            return  (TAsTo*)(this);
        }
        template<typename TAsTo>
        const   TAsTo*      as() const
        {
            return  (const TAsTo*)(this);
        }
        /// <summary>
        /// 动态类型转换
        /// </summary>
        /// <typeparam name="TCastTo"></typeparam>
        /// <returns></returns>
        template<typename TCastTo>
        inline  TCastTo*    cast()
        {
            return  dynamic_cast<TCastTo*>(this);
        }
        template<typename TCastTo>
        const   TCastTo*    cast() const
        {
            return  dynamic_cast<const TCastTo*>(this);
        }
        
    protected:
        FEContext&  _ctx;
        /// <summary>
        /// 对象的Id 惟一
        /// </summary>
        FEUuid      _id;
    };

    using   Object      =   SharedPtr<FEObject>;
    using   Objects     =   std::vector<Object>;
    using   ObjectMap   =   std::map<OBJId,Object>;
    using   ObjectUMap  =   std::unordered_map<OBJId,Object>;
    using   ObjectSet   =   std::set<Object>;
    using   ObjectUSet  =   std::unordered_set<Object>;
}

namespace   std
{
    template<>
    class   hash<FE::Object>
    {
    public:
        inline  size_t operator()(const FE::Object& key) const noexcept
        {
            std::hash<void*>    data;
            return  data((void*)key.get());
        }
    };

    template<>
    class   hash<FE::FEObject*>
    {
    public:
        inline  size_t operator()(const FE::FEObject* key) const noexcept
        {
            std::hash<void*>    data;
            return  data((void*)key);
        }
    };
}


