#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/graphic/FEScene.h"
namespace   FE
{
    constexpr   uint    MAX_FRAME_BUFFER    =   2;
    
    class   Demo
    {
    public:
        FEContext   _ctx;
        App         _app;
        Scene       _scene;
        bool        _prepared   =   false;
    public:
        Demo();
        ~Demo();
    public:
        void    messageNotify(const FEMessage& msgIn);
        void    main();
    };
}
