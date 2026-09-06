#pragma     once

#include    "FEAxis.h"
namespace   FE
{
    class FEBrowseAxisMovePrivate;
    class FE_API FEBrowseAxisMove:public FEBrowseAxis
    {
    public:
        FEBrowseAxisMove(FEContext& context);
        ~FEBrowseAxisMove();
    private:
        FEBrowseAxisMovePrivate* _p;
        friend class FEBrowseAxisMovePrivate;
    public:
        virtual void update(FEContext& context) override;
        virtual void render(FEContext& context) override;
    };
}
