#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/graphic/FEScene.h"
namespace   FE
{
    constexpr   uint    MAX_FRAME_BUFFER    =   2;

    class   Demo
    {
    public:
        FEContext&  _ctx;
        App         _app;
        Scene       _scene;
        FEUuid      _rendererId    =   RS_VULKAN;
        uint        _sampleCount    =   1;   ///< 多采样数(MSAA)
        bool        _prepared       =   false;
    public:
        Demo(const FEUuid& rendererId  =   RS_VULKAN, uint sampleCount = 1);
        ~Demo();
    public:
        void    messageNotify(const FEMessage& msgIn);
        void    main();
    };
}
