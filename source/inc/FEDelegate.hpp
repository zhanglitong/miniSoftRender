#pragma     once

#include    <functional>
#include    <vector>
#include    <algorithm>
#include    <type_traits>
#include    <utility>

namespace   FE
{
    /// <summary>
    /// 委托参数辅助类型，用于将 (对象指针, 可调用对象) 转换为 std::function
    /// </summary>
    template<class Signature>
    struct  FEDelegateArg;

    template<class Ret, class... Args>
    struct  FEDelegateArg<Ret(Args...)>
    {
        void*                               obj     =   nullptr;
        std::function<Ret(Args...)>         fn;
        /// <summary>
        /// 从任意可调用对象（函数指针、lambda、std::function 等）构造
        /// </summary>
        template<class F, class = std::enable_if_t<
            !std::is_same_v<std::decay_t<F>, FEDelegateArg> &&
            !std::is_member_function_pointer_v<std::decay_t<F>>>>
        FEDelegateArg(F&& f)
            : fn(std::forward<F>(f))
        {}

        /// <summary>
        /// 从 (对象指针, 任意可调用对象) 构造，支持 {obj, &Class::Method} 和 {obj, lambda/自由函数}
        /// 第二参数为成员函数指针时，通过 (obj->*method)(args...) 调用，Args 可隐式转换到方法参数类型
        /// 第二参数为非成员可调用对象时，直接包装为 std::function
        /// </summary>
        template<class T, class F, class = std::enable_if_t<
            !std::is_same_v<std::decay_t<F>, FEDelegateArg>>>
        FEDelegateArg(T* o, F&& f)
            : obj(static_cast<void*>(o))
        {
            if constexpr (std::is_member_function_pointer_v<std::decay_t<F>>)
            {
                fn  =   [o, f](Args... args) -> Ret { return (o->*f)(args...); };
            }
            else
            {
                fn  =   std::forward<F>(f);
            }
        }
    };

    /// <summary>
    /// 多播委托，支持注册多个回调函数并依次调用
    /// </summary>
    /// <typeparam name="Signature">函数签名，例如 void(int, float)</typeparam>
    template<class Signature>
    class   FETMultiDelegate;

    /// <summary>
    /// delegate += {this, &Class::Method};          // 成员函数指针（允许参数隐式转换）
    /// delegate += {this, [](Args...){...}};        // lambda + 对象跟踪
    /// delegate += {nullptr, free_function};        // 自由函数
    /// delegate += [this](Args...){...};            // lambda（无对象跟踪）
    /// delegate += std::function<Ret(Args...)>{...};// std::function
    /// </summary>
    /// <typeparam name="Ret"></typeparam>
    /// <typeparam name="...Args"></typeparam>
    template<class Ret, class... Args>
    class   FETMultiDelegate<Ret(Args...)>
    {
    public:
        using   FuncType    =   std::function<Ret(Args...)>;
        using   ArgType     =   FEDelegateArg<Ret(Args...)>;
    private:
        struct  Entry
        {
            void*       obj;    ///< 绑定的对象指针（用于 -= 移除），自由函数为 nullptr
            FuncType    func;
        };
        std::vector<Entry>  _entries;
    public:
        /// <summary>
        /// 注册一个回调函数
        /// </summary>
        /// <param name="func">回调函数</param>
        /// <param name="obj">绑定的对象指针，用于 -= 移除</param>
        void    add(const FuncType& func, void* obj = nullptr)
        {
            _entries.push_back({obj, func});
        }
        /// <summary>
        /// 移除所有回调函数
        /// </summary>
        void    clear()
        {
            _entries.clear();
        }
        /// <summary>
        /// 获取回调函数数量
        /// </summary>
        /// <returns></returns>
        size_t  size() const
        {
            return  _entries.size();
        }
        /// <summary>
        /// 注册回调：支持 {this, &Class::Method}、lambda、函数指针等
        /// </summary>
        /// <param name="arg">委托参数</param>
        /// <returns></returns>
        FETMultiDelegate& operator+=(ArgType arg)
        {
            _entries.push_back({arg.obj, std::move(arg.fn)});
            return  *this;
        }
        /// <summary>
        /// 移除绑定到指定对象的所有回调
        /// </summary>
        /// <param name="obj">对象指针</param>
        /// <returns></returns>
        FETMultiDelegate& operator-=(void* obj)
        {
            auto    itr =   std::remove_if(_entries.begin(),_entries.end(),[obj](const Entry& e) { return e.obj == obj; });
            _entries.erase(itr,_entries.end());
            return  *this;
        }
        void    operator()(const Args&... args) const
        {
            for (auto& e : _entries)
            {
                if (e.func) e.func(args...);
            }
        }
        void    fireNotify(const Args&... args) const
        {
            for (auto& e : _entries)
            {
                if (e.func) e.func(args...);
            }
        }
    };
}
