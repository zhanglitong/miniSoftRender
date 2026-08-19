#pragma     once

#include    "FEViewer.h"
#include    "../FEFactoryTemplate.hpp"

namespace   FE
{

    /// lambda 表达式作为类型别名，确保ViewerLess是类型名
    constexpr auto    ViewerLessLambda = [](const Viewer& l, const Viewer& r)
    {
        return  l < r ;
    };
    /// 先定义类型别名，确保ViewerLess是类型名
    using   ViewerLess    =   decltype(ViewerLessLambda);

    class   FEViewerMgr : public FEFactoryTemplate<Viewer,ViewerLess>
    {
    public:
        FEViewerMgr(FEContext& ctx)
            :FEFactoryTemplate(ctx,ViewerLessLambda)
        {}
        FEViewerMgr(const FEViewerMgr& other)
            :FEFactoryTemplate(other)
        {}

        virtual ~FEViewerMgr() = default;
        /// <summary>
        /// 查找工厂对象,如果没有返回nullptr
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        Viewer  findObject(const String& name) const
        {
            for (auto& var: objects())
            {
                if (var->name() == name)
                    return  var;
            }
            return  nullptr;
        }
        /// <summary>
        /// 获取当前激活状态的viewer,如果没有激活则为nullptr
        /// </summary>
        /// <returns></returns>
        Viewer  activeViewer() const
        {
            return  _viewerActive;
        }
        /// <summary>
        /// 设置当前激活状态的viewer
        /// </summary>
        /// <param name="viewer"></param>
        void   setActiveViewer(Viewer viewer)
        {
            _viewerActive   =   viewer;
        }
    protected:
        /// <summary>
        /// 当前激活状态的viewer,如果没有激活则为nullptr
        /// 只有激活状态，才会接收输入事件
        /// </summary>
        Viewer  _viewerActive   =   nullptr;
    };
}
