#include    "../../inc/FEDelegate.hpp"
#include    <cstdio>

using   namespace   FE;

struct  Foo
{
    int value   =   0;
    void    onEvent(int x)
    {
        value   +=  x;
        printf("Foo::onEvent(%d), value=%d\n", x, value);
    }
    void    onEventConst(int x) const
    {
        printf("Foo::onEventConst(%d)\n", x);
    }
};

void    freeFunc(int x)
{
    printf("freeFunc(%d)\n", x);
}

int main()
{
    FETMultiDelegate<void(int)>    dlg;

    Foo foo;
    Foo bar;

    /// 测试成员函数绑定
    dlg +=  {&foo, &Foo::onEvent};
    dlg +=  {&bar, &Foo::onEvent};
    /// 测试自由函数
    dlg +=  [](int x) { printf("lambda(%d)\n", x); };
    dlg +=  freeFunc;

    printf("size=%zu\n", dlg.size());
    dlg(10);

    /// 测试移除 foo 的回调
    dlg -=  &foo;
    printf("after -= &foo, size=%zu\n", dlg.size());
    dlg(20);

    return  0;
}
