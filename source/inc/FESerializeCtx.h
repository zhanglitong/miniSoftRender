#pragma     once

#include    <type_traits>
#include    <functional>
#include    <unordered_set>
#include    <variant>
#include    <array>
#include    <vector>
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
        /// 延迟引用回填函数:返回 true 表示已成功绑定,该登记从队列移除;
        /// 返回 false(目标类型不符等)则保留,最终计入 pendingCount()
        /// </summary>
        using   RefBinder           =   std::function<bool(Object object)>;
        using   ReserveHint         =   std::function<void(size_t)>;
        using   ResolveNotify       =   std::function<void()>;
        struct  PendingRef
        {
            FEUuid      _id;
            RefBinder   _binder;
        };
        using   PendingRefs         =   std::vector<PendingRef>;
        using   ResolveNotifies     =   std::vector<ResolveNotify>;
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
        /// 预分配对象表容量(容量提示,可选)
        /// </summary>
        ReserveHint reserve;
        /// <summary>
        /// 上下文查询函数
        /// </summary>
        QueryObject query   =   [](const FEUuid& id,FEObject* object,Option option)->QResult
        {
            UNUSED(id);
            UNUSED(object);
            UNUSED(option);
            return  {};
        };
        /// <summary>
        /// 登记一个当前查不到目标的引用。反序列化是单遍顺序流,目标对象可能写在后面
        /// (如蒙皮关节节点在依赖数据之后),由读取器在所有对象读完、对象表完整后
        /// 调用 resolvePending() 统一回填
        /// </summary>
        inline  void    bindLater(const FEUuid& id,RefBinder binder)
        {
            if (id == FEUuid::zero() || !binder)
                return;
            _pendingRefs.push_back({id,std::move(binder)});
        }
       
        /// <summary>
        /// 还有多少引用未回填
        /// </summary>
        inline  size_t  pendingCount() const
        {
            return  _pendingRefs.size();
        }
        /// <summary>
        /// 登记一个在所有延迟引用回填完成后执行的回调。
        /// 用于列表类引用:需要等每个元素都尝试绑定后,再一次性提交容器
        /// </summary>
        inline  void    afterResolve(ResolveNotify notify)
        {
            if (notify)
                _resolveNotifies.push_back(std::move(notify));
        }
        /// <summary>
        /// 用当前对象表回填所有登记的引用,返回仍未解析的数量(0 表示全部成功)。
        /// final == true 时同时执行 afterResolve 登记的回调;
        /// 多文件分批读取时,只有最后一批应传 true
        /// </summary>
        inline  size_t  resolvePending(bool final = true)
        {
            /// 先换出队列,避免回填过程中新增登记导致迭代器失效
            PendingRefs refs;
            refs.swap(_pendingRefs);
            for (auto& ref : refs)
            {
                Object  object  =   query(ref._id,nullptr,O_Query).first;
                if (object == nullptr || !ref._binder(object))
                    _pendingRefs.push_back(std::move(ref));
            }
            if (final)
            {
                ResolveNotifies notifies;
                notifies.swap(_resolveNotifies);
                for (auto& notify : notifies)
                    notify();
            }
            return  _pendingRefs.size();
        }
    protected:
       
        PendingRefs     _pendingRefs;
        ResolveNotifies _resolveNotifies;
    };
}



