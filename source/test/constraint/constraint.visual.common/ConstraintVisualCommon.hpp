#pragma     once
/// <summary>
/// 约束可视化测试公共框架
/// 提供: 窗口/场景/相机/共享立方体网格初始化、节点工厂、断言输出、固定步长帧推进
/// 各约束用例只需关注自己的节点、约束与断言
/// </summary>

#include    <iostream>
#include    <chrono>
#include    <thread>
#include    <functional>
#include    <vector>

#include    "FEContext.hpp"
#include    "FEAppHelper.hpp"
#include    "graphic/FEScene.h"
#include    "geometry/FEGeometryBox.hpp"
#include    "material/FEMaterialPBR.hpp"
#include    "animation/FEAnimationSys.hpp"
#include    "animation/FEPlayback.hpp"

namespace   fe_test
{
    using   namespace   FE;

    class   VisualCase
    {
    public:
        /// <summary>
        /// 创建窗口、场景、共享网格与固定取景相机;失败返回 false
        /// </summary>
        /// <param name="argc">命令行参数个数</param>
        /// <param name="argv">支持 --webgpu 切换渲染后端</param>
        /// <param name="title">用例名(用于打印结果)</param>
        bool    begin(int argc, char** argv, const char* title)
        {
            _title  =   title;

            _rendererId =   RS_VULKAN;
            for (int i = 1; i < argc; ++i)
            {
                if  (_stricmp(argv[i], "--webgpu") == 0 || _stricmp(argv[i], "-webgpu") == 0)
                    _rendererId =   RS_WEBGPU;
                else if (_stricmp(argv[i], "--vulkan") == 0 || _stricmp(argv[i], "-vulkan") == 0)
                    _rendererId =   RS_VULKAN;
            }

            _ctx    =   &FEContext::instance();

            FEApp::CreateInfo   info    =   {};
            info._width     =   1280;
            info._height    =   720;
            info._appInst   =   GetModuleHandle(nullptr);
            info._notify    =   [this](const FEMessage& msg)
            {
                if  (_scene != nullptr)
                    _scene->onMessage(msg);
            };
            _app    =   FEAppHelper::create(*_ctx, info);
            if  (_app == nullptr)
            {
                std::cerr << "create app failed" << std::endl;
                return  false;
            }
            _ctx->setWindow(_app.get());
            _ctx->setWorkPath(_app->path());
            _ctx->setResourcePath(_app->path() + "/../");

            /// <summary>
            /// 消息总线订阅:窗口关闭时抢在场景销毁 device 之前,释放用例与框架
            /// 持有的节点/网格引用,避免 device 销毁后仍有 GPU 资源释放
            /// </summary>
            _closeSub   =   _app->messageBus().subscribe(MSG_CLOSE,[this](const FEMessage&)
            {
                releaseResources();
            });

            _scenePtr   =   new FEScene(*_ctx);
            _scene      =   _scenePtr.get();
            if  (!_scene->setup(_app, _rendererId))
            {
                std::cerr << "scene setup failed" << std::endl;
                _scene  =   nullptr;
                return  false;
            }

            /// 共享立方体网格: 渲染按材质分组实例化,一个网格即可支持多节点
            FEGeometryBox   geo(*_ctx);
            geo.param()._size   =   float3(1.0f, 1.0f, 1.0f);
            geo.param()._segs   =   uint3(1, 1, 1);
            _mesh   =   geo.triangular({
                {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
                {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
                {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

            /// 固定取景: 俯视前方,保证全部用例节点都在视野内
            if  (auto  cam =   _scene->camera())
            {
                cam->setEye(real3(0.0, 5.0, 10.0));
                cam->setTarget(real3(0.0, 1.0, 0.0));
                cam->setViewSize(real2(1280.0, 720.0));
                cam->update();
            }
            return  true;
        }
        /// <summary>
        /// 注册窗口关闭回调:在场景销毁 device 之前执行,用于释放用例自己持有的
        /// 节点/组件/网格等引用(框架会额外释放 makeNode 创建的节点与共享网格)
        /// </summary>
        /// <param name="hook">清理代理</param>
        void    onClose(std::function<void()> hook)
        {
            _closeHooks.push_back(std::move(hook));
        }
        /// <summary>
        /// 创建立方体节点: 独立材质(渲染按材质分组),位置/缩放立即生效
        /// </summary>
        Node    makeNode(const real3& pos, const real3& scale, const float4& color)
        {
            Node    node    =   new FENode(*_ctx);
            _nodes.push_back(node);
            node->setMesh(_mesh);

            FEMaterialPBR*  mat =   new FEMaterialPBR(*_ctx);
            mat->data()._value._diffuse     =   color;
            mat->data()._value._emissive    =   float4(color.x * 0.35f, color.y * 0.35f, color.z * 0.35f, 1.0f);
            mat->data().update();
            node->setMaterial(mat);

            node->setLocalTranslation(pos);
            node->setLocalScaling(scale);
            node->makeDirty();
            node->update();
            return  node;
        }
        /// <summary>
        /// 等比缩放的立方体节点
        /// </summary>
        Node    makeNode(const real3& pos, real scale, const float4& color)
        {
            return  makeNode(pos, real3(scale), color);
        }
        /// <summary>
        /// 节点挂载组件后必须显式分发,组件系统才会收集动画/约束组件
        /// </summary>
        void    dispatch(const Nodes& nodes)
        {
            _scene->dispatchNodesToSystem(nodes);
        }
        /// <summary>
        /// 动画循环播放
        /// </summary>
        void    playLoop()
        {
            if  (auto  sys =   _scene->animationSystem())
            {
                auto    action  =   sys->getOrCreate();
                action->setMode(FEPlayback::PT_Loop);
                action->play();
            }
        }
        /// <summary>
        /// 推进若干帧(每帧间隔 sleepMs,让动画时间真实推进)
        /// </summary>
        void    step(int frames = 3, int sleepMs = 400)
        {
            for (int i = 0; i < frames; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
                _scene->onFrameStart();
                _scene->onFrameUpdate();
                _scene->onFrameRender();
                _scene->onFrameEnd();
            }
        }
        void    checkVec(const char* name, const real3& actual, const real3& expect, real eps = 2e-2f)
        {
            real    dist    =   real(glm::length(actual - expect));
            std::cout << (dist <= eps ? "[PASS] " : "[FAIL] ") << name
                      << "  actual(" << actual.x << ", " << actual.y << ", " << actual.z << ")"
                      << "  expect(" << expect.x << ", " << expect.y << ", " << expect.z << ")" << std::endl;
            if  (dist > eps)
                ++_failed;
        }
        void    checkDir(const char* name, const real3& dir, const real3& expect, real eps = 2e-2f)
        {
            real    dot =   real(glm::dot(glm::normalize(dir), glm::normalize(expect)));
            std::cout << (dot >= real(1) - eps ? "[PASS] " : "[FAIL] ") << name << "  dot=" << dot << std::endl;
            if  (dot < real(1) - eps)
                ++_failed;
        }
        void    checkBool(const char* name, bool value, bool expect)
        {
            std::cout << (value == expect ? "[PASS] " : "[FAIL] ") << name
                      << "  actual=" << value << " expect=" << expect << std::endl;
            if  (value != expect)
                ++_failed;
        }
        /// <summary>
        /// 进入渲染循环(持续观察约束效果)
        /// </summary>
        void    loop()
        {
            if  (_app != nullptr)
                _app->run();
        }
        /// <summary>
        /// 打印结论并释放场景/窗口
        /// </summary>
        int     finish()
        {
            if  (_failed == 0)
                std::cout << _title << " PASSED" << std::endl;
            else
                std::cout << _title << " FAILED, " << _failed << " case(s)" << std::endl;

            /// 未经窗口关闭消息直接结束(如用例提前返回)时同样先释放 GPU 资源引用
            if  (_app != nullptr)
                _app->messageBus().unsubscribe(_closeSub);
            releaseResources();

            _scene  =   nullptr;
            _scenePtr   =   nullptr;
            if  (_app != nullptr)
                _app->destroy();
            _app    =   nullptr;
            return  (_failed == 0) ? 0 : 1;
        }
    protected:
        /// <summary>
        /// 释放用例与框架持有的 GPU 资源引用
        /// 必须在 device 销毁之前调用(窗口关闭消息早于 FEScene::destroy)
        /// </summary>
        void    releaseResources()
        {
            /// 先执行用例清理,释放其持有的节点/组件引用
            for (auto& hook : _closeHooks)
            {
                if  (hook)
                    hook();
            }
            _closeHooks.clear();
            /// 再释放框架创建的节点持有的 GPU 资源(网格/材质),并断开组件与节点的双向引用,
            /// 否则节点会随约束/动画组件一直存活到 scene 析构,此时 device 已销毁
            for (auto& node : _nodes)
            {
                if  (node)
                    node->clear();
            }
            _nodes.clear();
            _mesh   =   nullptr;
        }
    public:
        FEContext*          _ctx        =   nullptr;
        FEScene*            _scene      =   nullptr;
        Scene               _scenePtr;
        App                 _app;
        Mesh                _mesh;
        Nodes               _nodes;
        std::vector<std::function<void()>>  _closeHooks;
        FEMessageBus::HandlerId             _closeSub   =   0;
        FEUuid              _rendererId =   RS_VULKAN;
        const char*         _title      =   "";
        int                 _failed     =   0;
    };
}
