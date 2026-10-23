#pragma     once

namespace   FE
{
    /// <summary>
    /// 标记未使用的变量/形参,避免编译警告
    /// C++17 折叠表达式,逐个将参数转为 void,从左到右求值
    /// </summary>
    template <typename... Args>
    inline void UNUSED(Args&&... args)
    {
        ((void)(args), ...);
    }
}
