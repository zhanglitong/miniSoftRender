#pragma     once
#include    "FEApp.hpp"

#include    "platform/FEWinApp.hpp"

namespace   FE
{
    class   FEAppHelper
    {
    public:
      static    App     create(FEContext& ctx,const FEApp::CreateInfo& setting)
      {
#if     FE_PLATFORM == FE_PLATFORM_WIN32
            App     app     =   new FEWinApp(ctx);
            if (!app->setup(setting))
                return  nullptr;
            else
                return  app;
#elif (FE_PLATFORM == FE_PLATFORM_LINUX)
        static_assert(false);
#else
          static_assert(false);
#endif
      }
    };
}
