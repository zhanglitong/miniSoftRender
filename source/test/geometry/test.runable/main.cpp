/// test.runable
/// 跨线程可执行项(FEScene::postRunable)测试:
///   1. 创建引擎 + 渲染窗口,主线程准备好共享的立方体 mesh 与材质
///   2. 两个工作线程每 10ms 创建一个位置随机(0-100)的立方体节点,
///      节点创建在各自线程完成,只有加入渲染系统(deliver)通过
///      postRunable 投递给场景
///   3. 场景在每帧帧首(onFrameStart)统一执行投递项,保证
///      dispatchNodesToSystem 永远运行在场景主线程
///   4. 场景内节点数达到上限后,主线程按 FIFO 批量移除最旧节点并释放资源;
///      已投递未执行的项也有限制,渲染跟不上时给工作线程背压
/// 用法: test.runable.exe [--vulkan|--webgpu]

#include    <algorithm>
#include    <atomic>
#include    <chrono>
#include    <iostream>
#include    <random>
#include    <thread>

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/FEAppHelper.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/geometry/FEGeometryBox.hpp"
#include    "../../../inc/material/FEMaterialPBR.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

namespace
{
    /// 场景内保留的立方体数量上限,超出后按 FIFO 移除最旧的一批
    constexpr   size_t  kMaxLiveNodes  =   256;
    /// 一次清理的数量:批量移除,避免每帧只删一个造成频繁增删
    constexpr   size_t  kEvictBatch    =   32;
    /// 已投递未执行项的上限:渲染跟不上时阻塞工作线程,避免队列无限膨胀
    constexpr   size_t  kMaxPending    =   64;

    /// <summary>
    /// 主线程与工作线程共享的计数
    /// </summary>
    struct  TestState
    {
        std::atomic<bool>       _running    {   true    };
        std::atomic<size_t>     _posted     {   0       };
        std::atomic<size_t>     _removed    {   0       };
        std::atomic<size_t>     _pending    {   0       };
    };

    /// <summary>
    /// 工作线程:每 10ms 创建一个随机位置的立方体,并投递给场景主线程
    /// </summary>
    /// <param name="scene">场景,postRunable 可在任意线程调用</param>
    /// <param name="mesh">共享立方体网格(引用计数线程安全)</param>
    /// <param name="pMat">共享材质,生命周期由主线程持有</param>
    /// <param name="live">存活节点表,只在主线程(投递项执行时)访问</param>
    /// <param name="state">共享计数与退出标志</param>
    /// <param name="seed">随机数种子,两个线程取不同值避免产生相同序列</param>
    void    cubeProducer(FEContext& ctx,FEScene* scene,Mesh mesh,FEMaterial* pMat,
                         Nodes& live,TestState* state,uint seed)
    {
        std::mt19937                            rng(seed);
        std::uniform_real_distribution<float>   pos(0.0f,100.0f);

        while (state->_running.load(std::memory_order_relaxed))
        {
            /// 背压:待执行项过多时先等主线程消费,防止队列无限增长
            if (state->_pending.load(std::memory_order_relaxed) >= kMaxPending)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            /// 节点只在本线程内构造,还没进入渲染系统,不存在并发访问
            Node    node    =   new FENode(ctx);
            node->setLocalTranslation(real3(pos(rng),pos(rng),pos(rng)));
            node->setMesh(mesh);
            node->setMaterial(pMat);
            node->makeDirty();
            node->update();

            /// 唯一触碰渲染系统的操作必须回主线程执行
            QueueItems  items;
            QueueItem   item    =   new FEQueueItem(ctx);
            item->_runable  +=  [scene,node,&live,state]
            {
                scene->dispatchNodesToSystem({ node });
                live.emplace_back(node);
                /// 超过上限时批量移除最旧的节点;
                /// 移除只在主线程做,不会与 dispatch 竞争渲染系统
                if (live.size() > kMaxLiveNodes)
                {
                    const   size_t  count   =   std::min(kEvictBatch,live.size() - kMaxLiveNodes);
                    Nodes           evicted(live.begin(),live.begin() + count);
                    scene->removeNodesFromSystem(evicted);
                    live.erase(live.begin(),live.begin() + count);
                    state->_removed.fetch_add(count,std::memory_order_relaxed);
                }
                state->_pending.fetch_sub(1,std::memory_order_relaxed);
            };
            items.emplace_back(item);
            /// 先加 pending 再投递:投递项可能立刻被主线程执行并减计数
            state->_pending.fetch_add(1,std::memory_order_relaxed);
            if (scene->postRunable(std::move(items)) != 0)
                state->_posted.fetch_add(1,std::memory_order_relaxed);
            else
                state->_pending.fetch_sub(1,std::memory_order_relaxed);

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan
    FEUuid  rendererId  =   RS_VULKAN;
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--webgpu") == 0 || _stricmp(arg, "-webgpu") == 0 ||
            _stricmp(arg, "/webgpu") == 0)
        {
            rendererId = RS_WEBGPU;
        }
        else if (_stricmp(arg, "--vulkan") == 0 || _stricmp(arg, "-vulkan") == 0 ||
                 _stricmp(arg, "/vulkan") == 0)
        {
            rendererId = RS_VULKAN;
        }
    }

    FEContext&  ctx =   FEContext::instance();

    /// 场景指针在引擎创建后赋值;notify 把窗口消息转发给场景
    FEScene*    pScene  =   nullptr;

    /// 1. 创建渲染窗口
    FEApp::CreateInfo  info   =   {};
    info._width     =   1280;
    info._height    =   720;
    info._appInst   =   GetModuleHandle(nullptr);
    info._notify    =   [&](const FEMessage& msg)
    {
        if (pScene)
            pScene->onMessage(msg);
    };
    App app =   FEAppHelper::create(ctx, info);
    if (app == nullptr)
    {
        std::cerr << "create app failed" << std::endl;
        return  1;
    }
    ctx.setWindow(app.get());
    ctx.setWorkPath(app->path());
    ctx.setResourcePath(app->path() + "/../");

    /// 2. 创建场景并初始化渲染后端
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        pScene  =   nullptr;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 主线程准备共享的立方体 mesh 与材质,避免工作线程重复分配
    Mesh        mesh;
    FEMaterialPBR*  pMat    =   nullptr;
    {
        FEGeometryBox   geo(ctx);
        geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
        geo.param()._segs  =   uint3(1, 1, 1);
        mesh    =   geo.triangular({
            {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
            {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

        pMat    =   new FEMaterialPBR(ctx);
        pMat->data()._value._diffuse    =   float4(0.15f, 0.55f, 0.95f, 1.0f);
        pMat->data()._value._emissive   =   float4(0.10f, 0.35f, 0.65f, 1.0f);
        pMat->data().update();
    }
    if (mesh == nullptr || mesh->primitives().empty())
    {
        std::cerr << "box build validation failed" << std::endl;
        pScene  =   nullptr;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 4. 两个工作线程以 10ms 间隔持续创建立方体并投递;
    ///    存活节点表与清理逻辑只在主线程(投递项执行时)读写
    Nodes       live;
    TestState   state;
    std::thread producer0(cubeProducer, std::ref(ctx), scene.get(), mesh, pMat,
                          std::ref(live), &state, 1u);
    std::thread producer1(cubeProducer, std::ref(ctx), scene.get(), mesh, pMat,
                          std::ref(live), &state, 2u);
    std::cout << "2 producer threads posting a cube every 10ms (max live nodes="
              << kMaxLiveNodes << ", evict batch=" << kEvictBatch
              << "), entering render loop..." << std::endl;

    /// 5. 渲染循环:场景在每帧帧首执行投递项
    app->run();

    /// 6. 先停线程再销毁场景:否则工作线程可能访问已释放的场景
    state._running  =   false;
    producer0.join();
    producer1.join();
    std::cout << "posted=" << state._posted.load()
              << " removed=" << state._removed.load()
              << " live=" << live.size() << std::endl;

    pScene  =   nullptr;
    scene   =   nullptr;
    app->destroy();

    return  0;
}
