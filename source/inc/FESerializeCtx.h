#pragma     once

#include    <type_traits>
#include    <functional>
#include    <unordered_set>
#include    <variant>
#include    <array>
#include    "FEDefine.h"
#include    "FEUuid.h"


namespace   FE
{
    class   FEObject;
    using   Object          =   SharedPtr<FEObject>;
    class   FEAllocator;
    class   FEContext;
    using   FECreator       =   std::function<Object(FEContext&,const FEAllocator&)>;

    struct  FESerializeCtx
    {
        enum    Option: uint32_t
        {
            /// <summary>
            /// 查询对象
            /// </summary>
            O_Query,
            /// <summary>
            /// 查询类型index
            /// </summary>
            O_QueryClassIndex,
            /// <summary>
            /// 缓存对象
            /// </summary>
            O_Cache,
            /// <summary>
            /// 获取进度对象
            /// </summary>
            O_Progress ,
            /// <summary>
            /// 更新进度
            /// </summary>
            O_UpdateProgress,
        };
    public:
        using   QResult             =   std::pair<FEObject*,uint32_t>;
        /// <summary>
        /// 1. option == O_Query,           则使用id,查询,如果查询到了，返回对象以及对象索引
        /// 2. option == O_QueryClassIndex, 则使用id,查询,如果查询到了，返回索引,<0,没有找到,对象 == nullptr;
        /// 3. option == O_Cache,           则使用id,object 会被cache到上下文，返回值是对象自身以及索引
        /// 4. option == O_Progress,        返回object,是FEProgress对象
        /// 5. option == O_UpdateProgress,  更新进度
        /// </summary>
        using   QueryObject         =   std::function<QResult(const FEUuid& id,FEObject* object,Option option)>;
        /// <summary>
        /// 加载数据总大小
        /// </summary>
        uint64      allByte         =   0;
        /// <summary>
        /// 已经处理的大小
        /// </summary>
        uint64      doByte          =   0;
        bool        change          =   false;
        /// <summary>
        /// 缓存上一次读取classId
        /// </summary>
        FEUuid      preCLSId;
        /// <summary>
        /// 上一次的创建器,如果读取到clsId == preCLSId preCreator 可以直接使用，避免重复查询
        /// </summary>
        FECreator   preCreator;
        /// <summary>
        /// 上下文查询函数
        /// </summary>
        QueryObject query   =   [](const FEUuid& id,FEObject* object,Option option)->QResult
        {
            (void)id;
            (void)object;
            (void)option;
            return  {};
        };
    };
}



