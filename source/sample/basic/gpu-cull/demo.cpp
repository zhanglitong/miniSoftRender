#include    "demo.h"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/geometry/FEGeometryGrid.hpp"
#include    "../../../inc/FEFileFormatHelper.hpp"
#include    "../../../inc/FENodeHelper.hpp"
#include    "../../../inc/FEProperty.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"
#include    "../../../inc/FEGeometryLibrary.hpp"

namespace   FE
{
    Demo::Demo()
    {
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
        FEGeometryCylinder  geoBox(_ctx);
        Attrs               attrs   =   
        {
            {IS_VERTEX_POS, FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR, FMT_R32G32B32_FLOAT},
        };
        Node            root    =   new FENode(_ctx);
        Mesh            mesh    =   geoBox.triangular(attrs);
        root->children().reserve(100 * 100 *100);
        auto            pbrMat  =   new FEMaterialPBR(_ctx);
        pbrMat->data().update();
        for (size_t z = 0; z < 100; z++)
        {
            for (size_t r = 0; r < 100; ++r)
            {
                for (size_t c = 0; c < 100; ++c)
                {
                    Node    node    =   new FENode(_ctx);
                    node->setLocalTranslation(real3(r * 2,c * 2,z * 2));
                    node->setMaterial(pbrMat);
                    node->setMesh(mesh);
                    root->addChild(node);
                }
            }
            
        }
        root->update();

        _scene->dispatchNodesToSystem({root});
        _scene->addNodesToTree({root});
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
