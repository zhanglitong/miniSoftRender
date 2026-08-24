#include    "demo.h"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/geometry/FEGeometryGrid.hpp"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/FENodeHelper.hpp"
#include    "../../../inc/FEProperty.hpp"

namespace   FE
{
    Demo::Demo()
    {

        /// KFValue     val     =   float3(1,1,1);
        /// KFValue     val1    =   0.4;
        /// KFValue     v1      =   0.2 + val + 0.4f;
        /// KFValue     v2      =   0.2 + val1 ; 
        
        FEApp::CreateInfo   info    =   {};
        info._notify    =   std::bind(&Demo::messageNotify,this,std::placeholders::_1);
        info._appInst   =   GetModuleHandle(nullptr);
        _app    =   FE::FEAppHelper::create(_ctx,info);
        if (_app == nullptr)
            return;
        _ctx.setWindow(_app.get());
        _ctx.setWorkPath(_app->path());
        _ctx.setResourcePath(_app->path() + "/../");
        _scene      =   new FEScene(_ctx);
        _scene->setup(_app);
        _scene->test();

        //String          gltfFile    =   _ctx.resourcePath() + "/assets/model/glTF/FlightHelmet.gltf";
        String          gltfFile    =   R"(E:\study\gltf\glTF-Sample-Assets\Models\AnimatedColorsCube\glTF\AnimatedColorsCube.gltf)";
        FEFileFormat    fmtText(".gltf","1.0.0.0","GLTF text Format!");
        
        auto            reader  =   FEFileFormatHelper::queryReader(_ctx,fmtText);
        if (reader)
        {
            auto    objects =   reader->readFiles({gltfFile});
            Nodes   nodes;
            for (auto var : objects)
            {   
                Node    node    =   var->cast<FENode>();
                if (node == nullptr)
                    continue;
                else
                    nodes.push_back(node);
            }
            _scene->dispatchNodesToSystem(nodes);
            _scene->addNodesToTree(nodes);
        }
        _prepared   =   true;
    }


    Demo::~Demo()
    {
        _scene  =   nullptr;
        _app    =   nullptr;
    }
    void    Demo::main()
    {
        LOG_INF("Demo::main()");
        _app->run();

        _scene  =   nullptr;
        _app->destroy();
        _app    =   nullptr;
    }
 
    void    Demo::messageNotify(const FEMessage& msgIn)
    {
        if (_scene == nullptr)
            return;
        switch(msgIn.msgId())
        {
        case MSG_RESIZE         :
            _prepared   =   false;
            _scene->onMessage(msgIn);
            _prepared   =   true;
            return;
        case MSG_RESIZE_START   :
            break;
        case MSG_RESIZE_END     :
            break;
        case MSG_UPDATE         :   
            if (!_prepared)
                return;
            break;
        case MSG_RENDER         :
            if (!_prepared)
                return;
            break;
        }
        _scene->onMessage(msgIn);
    }
}
