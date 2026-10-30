/// test.animation
/// 动画系统覆盖式应用测试:
///   1. 节点自身本地变换: 位置(3,1,0)、缩放(2,2,2)、绕 Z 轴 30 度
///   2. 动画仅含位置轨道: 0s -> (-0.4,0,0),2s -> (0.4,0,0)
///   3. 断言采样位置覆盖节点本地位置;未打帧的缩放/旋转保持节点本地值
///   4. 断言动画停用后节点回落到自身本地变换
/// 用法: test.animation.exe [--vulkan|--webgpu] [--pingpong]

#include    <iostream>
#include    <cmath>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEAppHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/material/FEMaterialPBR.hpp"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"

using   namespace   FE;

/// 注册渲染后端插件
USING_PLUGIN(rs_vulkan);
USING_PLUGIN(rs_webgpu);

int     main(int argc, char** argv)
{
    /// 选择渲染后端,默认 Vulkan; --pingpong 切换播乒乓模式
    FEUuid  rendererId  =   RS_VULKAN;
    FEPlayback::PlayMode    playMode    =   FEPlayback::PT_PingPong;
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
        else if (_stricmp(arg, "--pingpong") == 0)
        {
            playMode    =   FEPlayback::PT_PingPong;
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

    /// 2. 创建引擎并初始化渲染后端
    Scene   scene   =   new FEScene(ctx);
    pScene          =   scene.get();
    if (!scene->setup(app, rendererId))
    {
        std::cerr << "scene setup failed" << std::endl;
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 3. 生成立方体节点,并设置非默认的本地变换(位置/缩放/旋转)
    FEGeometryBox   geo(ctx);
    geo.param()._size  =   float3(1.0f, 1.0f, 1.0f);
    geo.param()._segs  =   uint3(1, 1, 1);

    Mesh    mesh    =   geo.triangular({
        {IS_VERTEX_POS,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_NOR,       FMT_R32G32B32_FLOAT},
        {IS_VERTEX_TEXCOORD0, FMT_R32G32_FLOAT} });

    Node    node    =   new FENode(ctx);
    node->setMesh(mesh);

    FEMaterialPBR* pMat  =   new FEMaterialPBR(ctx);
    pMat->data()._value._diffuse  =   float4(0.15f, 0.55f, 0.95f, 1.0f);
    pMat->data()._value._emissive =   float4(0.10f, 0.35f, 0.65f, 1.0f);
    pMat->data().update();
    node->setMaterial(pMat);

    node->setLocalTranslation(real3(3.0,1.0,0.0));
    node->setLocalScaling(real3(0.5,0.5,0.5));
    node->setLocalRotation(quatr(glm::radians(real3(0.0,0.0,30.0))));
    node->makeDirty();
    node->update();

    /// 4. 创建只含位置轨道的动画:0s -> (-0.4,0,0),2s -> (0.4,0,0)
    Animation   anim    =   FEAnimationHelper::createNodeAnimtion(ctx, NP_TRANSFORM_XYZ);
    AnimClip    clip    =   anim->clip();
    clip->addKeyFrame(0.0, real3(-0.4,0.0,0.0));
    clip->addKeyFrame(2.0, real3(0.4,0.0,0.0));
    node->addComponent(Component(anim.get()));
    /// 必须在挂载组件之后分发,动画系统才会收集到该节点的动画
    scene->dispatchNodesToSystem({node});

    /// 5. 断言:位置被动画覆盖,缩放/旋转保持节点本地值
    bool    passed      =   true;
    auto    checkReal   =   [&](const char* name,real actual,real expect)
    {
        bool    ok  =   std::abs(actual - expect) <= 1e-3;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name
                  << " actual=" << actual << " expect=" << expect << std::endl;
        passed  =   passed && ok;
    };

    anim->update(1.0);
    node->update();
    real3   gpos;
    real3   gscl;
    quatr   grot;
    FE::decompose<real>(node->globalTransform(),gpos,gscl,grot);
    std::cout << "t=1.0 pos=[" << gpos.x << "," << gpos.y << "," << gpos.z
              << "] scale=[" << gscl.x << "," << gscl.y << "," << gscl.z << "]" << std::endl;
    checkReal("覆盖:位置 X",gpos.x,0.0);
    checkReal("覆盖:位置 Y",gpos.y,0.0);
    checkReal("保留:缩放 X",gscl.x,0.5);
    checkReal("保留:缩放 Y",gscl.y,0.5);
    quatr   qz30    =   quatr(glm::radians(real3(0.0,0.0,30.0)));
    checkReal("保留:绕 Z 旋转",std::abs((real)glm::dot(grot,qz30)),1.0);

    /// 6. 断言:动画停用后节点回落到自身本地变换
    anim->setEnable(false);
    anim->update(1.0);
    node->update();
    FE::decompose<real>(node->globalTransform(),gpos,gscl,grot);
    std::cout << "disabled pos=[" << gpos.x << "," << gpos.y << "," << gpos.z << "]" << std::endl;
    checkReal("回落:位置 X",gpos.x,3.0);
    checkReal("回落:位置 Y",gpos.y,1.0);

    /// 6.5 编辑轴回归:编辑只改节点、不写关键帧(打帧是独立动作);
    ///     暂停期间不再每帧采样,编辑结果被保留;打帧后采样回放编辑结果
    {
        /// 补齐默认动画的三条轨道(位置轨道前面已建,这里补缩放/旋转)
        FEAnimationHelper::addDefaultTracks(clip,ctx,NodeProperyBits(NP_SCALE_XYZ|NP_ROTATE_XYZ));
        anim->setEnable(true);
        anim->update(0.0);
        node->update();
        auto    sys =   scene->animationSystem();
        if (sys == nullptr)
        {
            std::cout << "[FAIL] animationSystem 为空" << std::endl;
            passed  =   false;
        }
        else
        {
            /// 移动抓手:直接写节点属性,不经动画系统,也不产生关键帧
            real3   editPos =   real3(1.5,0.25,-0.75);
            Object  objNode =   Object(node.get());
            objNode->beginSetProp();
            objNode->setProperty(PROP_TRANSFORM_XYZ,editPos);
            objNode->endSetProp(0.0,true);
            real3   lpos    =   node->localTranslation();
            checkReal("编辑:位置 X",lpos.x,editPos.x);
            checkReal("编辑:位置 Y",lpos.y,editPos.y);
            checkReal("编辑:位置 Z",lpos.z,editPos.z);

            /// 旋转抓手:直接写节点 PROP_QUAT
            const   quatr   rotQ    =   normalize(quatr(glm::radians(real3(0.0,0.0,45.0))));
            const   quatf   editRot =   quatf((float)rotQ.w,(float)rotQ.x,(float)rotQ.y,(float)rotQ.z);
            objNode->beginSetProp();
            objNode->setProperty(PROP_QUAT,editRot);
            objNode->endSetProp(0.0,true);
            checkReal("编辑:旋转 dot",(real)std::abs(glm::dot(node->localRotation(),editRot)),1.0);

            /// 缩放抓手:直接写节点 PROP_SCALE_XYZ
            const   real3   editScl =   real3(1.25,1.25,1.25);
            objNode->beginSetProp();
            objNode->setProperty(PROP_SCALE_XYZ,editScl);
            objNode->endSetProp(0.0,true);
            real3   lscl    =   node->localScaling();
            checkReal("编辑:缩放 X",lscl.x,editScl.x);

            /// 打帧:把当前节点姿态写成 0s 关键帧,采样后应保持编辑结果
            /// addNodeKeyFrame 只新增时间点,该时间已存在键时由 updateNodeKeyFrame 覆盖
            FEAnimationHelper::addNodeKeyFrame(anim,0.0,node);
            FEAnimationHelper::updateNodeKeyFrame(anim,0.0,node);
            anim->update(0.0);
            node->update();
            lpos    =   node->localTranslation();
            checkReal("打帧后:位置 X",lpos.x,editPos.x);
            checkReal("打帧后:位置 Y",lpos.y,editPos.y);
            checkReal("打帧后:旋转 dot",(real)std::abs(glm::dot(node->localRotation(),editRot)),1.0);
            lscl    =   node->localScaling();
            checkReal("打帧后:缩放 X",lscl.x,editScl.x);
        }
    }

    /// 6.6 打帧自动补齐缺失轨道:clip 只有位置轨道时,旋转也能落帧
    {
        Animation   anim2   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_TRANSFORM_XYZ);
        AnimClip    clip2   =   anim2->clip();
        anim2->attach(Object(node.get()));
        const   NodeProperyBits rotBits(NP_ROTATE_X|NP_ROTATE_XYZ);
        /// 未补齐前 clip 里没有旋转轨道,旋转关键帧无处可写
        bool    hasRot  =   false;
        for (auto& track : clip2->tracks())
            hasRot  =   hasRot || (track && track->propertyIndex() == PROP_ROTATE_XYZ);
        checkReal("补齐前:无旋转轨道",hasRot ? 1.0 : 0.0,0.0);
        /// 自动补齐缺失轨道后才应能落帧
        FEAnimationHelper::ensureTracks(clip2,ctx,rotBits);
        bool    keyed   =   FEAnimationHelper::addNodeKeyFrame(anim2,0.0,node,rotBits);
        checkReal("补齐后:旋转打帧成功",keyed ? 1.0 : 0.0,1.0);
        size_t  rotateTracks    =   0;
        size_t  rotateKeys      =   0;
        for (auto& track : clip2->tracks())
        {
            if (track && track->propertyIndex() == PROP_ROTATE_XYZ)
            {
                ++rotateTracks;
                if (track->_times)
                    rotateKeys  =   track->_times->values().size();
            }
        }
        checkReal("补齐后:旋转轨道数",(real)rotateTracks,1.0);
        checkReal("补齐后:旋转关键帧数",(real)rotateKeys,1.0);
    }

    /// 6.8 多圈旋转:0° → 720° 关键帧。欧拉轨道逐分量线性插值应走长弧,
    ///     中点采样为 360° 而不是最短弧的 0°,且累计角账本保持有效
    {
        Node        node8   =   new FENode(ctx);
        Animation   anim8   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_ROTATE_XYZ);
        AnimClip    clip8   =   anim8->clip();
        node8->addComponent(Component(anim8.get()));
        scene->dispatchNodesToSystem({node8});
        /// 走真实打帧入口:读节点累计欧拉角账本写关键帧。0° 与 720° 都应原样记录,
        /// 不能被 quatToEuler 归一化到 ±180°(否则回放只剩最短弧,表现为"转得慢")
        node8->setLocalEuler(real3(0.0,0.0,0.0));
        FEAnimationHelper::addNodeKeyFrame(anim8,0.0,node8);
        node8->setLocalEuler(real3(0.0,0.0,720.0));
        FEAnimationHelper::addNodeKeyFrame(anim8,2.0,node8);
        real    lastKeyZ    =   -1.0;
        for (auto& track : clip8->tracks())
        {
            if (track && track->propertyIndex() == PROP_ROTATE_XYZ && track->_values.index() == 3)
            {
                auto&   vals    =   std::get<3>(track->_values)->values();
                if (!vals.empty())
                    lastKeyZ    =   (real)vals.back().z;
            }
        }
        checkReal("打帧读账本:末帧 Z",lastKeyZ,720.0);
        anim8->update(1.0);
        FETransform*    tf8 =   anim8->getTransform();
        if (tf8 == nullptr)
        {
            std::cout << "[FAIL] 多圈:采样变换为空" << std::endl;
            passed  =   false;
        }
        else
        {
            checkReal("多圈插值:中点 Z",(real)tf8->euler().z,360.0);
            checkReal("多圈插值:账本有效",tf8->eulerValid() ? 1.0 : 0.0,1.0);
            anim8->applyTransform();
            checkReal("多圈回写:节点账本 Z",node8->localEuler().z,360.0);
        }
    }

    /// 6.9 累计欧拉角账本语义:展开算法、拖拽累积(跨 180°)、松手保持、属性读写不归一化
    {
        Node    nodeE   =   new FENode(ctx);
        Object  objE    =   Object(nodeE.get());
        /// 展开算法:把等价角拉到 reference 附近
        checkReal("账本:unwrap 前向",(real)FETransform::unwrapAngle(10.0,720.0),730.0);
        checkReal("账本:unwrap 反向",(real)FETransform::unwrapAngle(350.0,0.0),-10.0);
        /// 连续 25°/步,共 29 步 = 725°;跨过 180° 时不能被折回 ±180
        quatr   rotE    =   quatr(1.0,0.0,0.0,0.0);
        for (size_t i = 0; i < 29; ++i)
        {
            quatr   delta   =   quatr(glm::radians(real3(0.0,0.0,25.0)));
            rotE            =   glm::normalize(delta * rotE);
            nodeE->setLocalRotationAccumulated(rotE);
            nodeE->update();
            if (i == 7)
                checkReal("账本:跨 180° 后 Z",nodeE->localEuler().z,200.0);
        }
        checkReal("账本:拖拽累计 Z",nodeE->localEuler().z,725.0);
        checkReal("账本:拖拽结束有效",nodeE->localEulerValid() ? 1.0 : 0.0,1.0);
        /// 松手后同值四元数写入应保留账本(不做 ±180 归一化)
        objE->setProperty(PROP_QUAT,quatf(nodeE->localRotation()));
        checkReal("账本:同值四元数保持",nodeE->localEuler().z,725.0);
        /// 朝向真正变化则账本失效,退回四元数换算
        objE->setProperty(PROP_QUAT,quatf(glm::radians(real3(0.0,0.0,90.0))));
        checkReal("账本:异值失效",nodeE->localEulerValid() ? 1.0 : 0.0,0.0);
        checkReal("账本:异值回落 Z",nodeE->localEuler().z,90.0);
        /// PROP_ROTATE_* 读写:单位是度,不做二次 RAD2DEG(旧 bug 会放大 57 倍)
        nodeE->setLocalEuler(real3(0.0,0.0,0.0));
        objE->setProperty(PROP_ROTATE_Z,real(900.0));
        checkReal("属性:Z 写入 900",nodeE->localEuler().z,900.0);
        checkReal("属性:Z 读出 900",std::get<real>(objE->getProperty(PROP_ROTATE_Z)),900.0);
        objE->setProperty(PROP_ROTATE_X,real(100.0));
        checkReal("属性:X 写入 100",nodeE->localEuler().x,100.0);
        checkReal("属性:X 不影响 Z",nodeE->localEuler().z,900.0);
        /// XYZ 整组往返
        objE->setProperty(PROP_ROTATE_XYZ,real3(10.0,20.0,30.0));
        real3   eXYZ    =   std::get<real3>(objE->getProperty(PROP_ROTATE_XYZ));
        checkReal("属性:XYZ 往返 X",eXYZ.x,10.0);
        checkReal("属性:XYZ 往返 Y",eXYZ.y,20.0);
        checkReal("属性:XYZ 往返 Z",eXYZ.z,30.0);
        /// FETransformPivot(动画内部副本)的账本语义必须与 FETransform 一致
        FETransformPivot    pv;
        pv.setEuler(float3(0.0f,0.0f,720.0f));
        checkReal("枢轴:setEuler 账本 Z",(real)pv.euler().z,720.0);
        pv.setRotation(pv.rotation());
        checkReal("枢轴:同值四元数保持",(real)pv.euler().z,720.0);
        pv.setRotation(quatf(glm::radians(float3(0.0f,0.0f,90.0f))));
        checkReal("枢轴:异值失效",pv.eulerValid() ? 1.0 : 0.0,0.0);
        /// setRotationUnwrapped 以参考角展开
        FETransform tfU;
        tfU.setEuler(float3(0.0f,0.0f,725.0f));
        tfU.setRotationUnwrapped(quatf(glm::radians(float3(0.0f,0.0f,5.0f))),float3(0.0f,0.0f,725.0f));
        checkReal("账本:展开接续 725",(real)tfU.euler().z,725.0);
    }

    /// 6.10 插值类型覆盖:贝塞尔/常量/反向多圈/标量轨道/四元数轨道回退
    {
        /// 贝塞尔 0→720:对称控制点,中点仍应是 360(走长弧)
        Node    nodeB   =   new FENode(ctx);
        Animation   animB   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_ROTATE_XYZ);
        AnimClip    clipB   =   animB->clip();
        clipB->addKeyFrame(0.0,real3(0.0,0.0,0.0));
        clipB->addKeyFrame(2.0,real3(0.0,0.0,720.0));
        nodeB->addComponent(Component(animB.get()));
        scene->dispatchNodesToSystem({nodeB});
        for (auto track : clipB->tracks())
        {
            if (track != nullptr)
                track->_type    =   FEKeyFrameTrack::IT_Bezier;
        }
        animB->update(1.0);
        if (animB->getTransform() != nullptr)
            checkReal("插值:贝塞尔中点 Z",(real)animB->getTransform()->euler().z,360.0);
        /// 常量插值:保持起始帧
        for (auto track : clipB->tracks())
        {
            if (track != nullptr)
                track->_type    =   FEKeyFrameTrack::IT_Constant;
        }
        animB->update(1.0);
        if (animB->getTransform() != nullptr)
            checkReal("插值:常量中点 Z",(real)animB->getTransform()->euler().z,0.0);

        /// 反向多圈 0→-720:中点 -360
        Node    nodeR   =   new FENode(ctx);
        Animation   animR   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_ROTATE_XYZ);
        animR->clip()->addKeyFrame(0.0,real3(0.0,0.0,0.0));
        animR->clip()->addKeyFrame(2.0,real3(0.0,0.0,-720.0));
        nodeR->addComponent(Component(animR.get()));
        scene->dispatchNodesToSystem({nodeR});
        animR->update(1.0);
        if (animR->getTransform() != nullptr)
        {
            checkReal("插值:反向中点 Z",(real)animR->getTransform()->euler().z,-360.0);
            animR->applyTransform();
            checkReal("插值:反向回写节点 Z",nodeR->localEuler().z,-360.0);
        }

        /// 标量轨道 PROP_ROTATE_Z 多圈:中点 360,末点 720
        Node    nodeS   =   new FENode(ctx);
        Animation   animS   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_ROTATE_Z);
        animS->clip()->addKeyFrame(0.0,real(0.0));
        animS->clip()->addKeyFrame(2.0,real(720.0));
        nodeS->addComponent(Component(animS.get()));
        scene->dispatchNodesToSystem({nodeS});
        animS->update(1.0);
        if (animS->getTransform() != nullptr)
            checkReal("插值:标量轨道中点 Z",(real)animS->getTransform()->euler().z,360.0);
        animS->update(2.0);
        if (animS->getTransform() != nullptr)
            checkReal("插值:标量轨道末点 Z",(real)animS->getTransform()->euler().z,720.0);

        /// 四元数轨道(PROP_QUAT)只能表示朝向:slerp 走最短弧,账本必然失效,
        /// 这是固有上限,用断言固化该行为,避免以后误判为 bug
        Node    nodeQ   =   new FENode(ctx);
        Animation   animQ   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_QUAT);
        quatr   q0      =   quatr(1.0,0.0,0.0,0.0);
        quatr   q180    =   quatr(glm::radians(real3(0.0,0.0,180.0)));
        quatr   q90     =   quatr(glm::radians(real3(0.0,0.0,90.0)));
        animQ->clip()->addKeyFrame(0.0,q0);
        animQ->clip()->addKeyFrame(2.0,q180);
        nodeQ->addComponent(Component(animQ.get()));
        scene->dispatchNodesToSystem({nodeQ});
        animQ->update(1.0);
        if (animQ->getTransform() != nullptr)
        {
            checkReal("插值:四元数中点朝向",std::abs((real)glm::dot(quatr(animQ->getTransform()->rotation()),q90)),1.0);
            checkReal("插值:四元数账本失效",animQ->getTransform()->eulerValid() ? 1.0 : 0.0,0.0);
        }
    }

    /// 6.11 动画停用后恢复节点原始累计账本(而不是回落到 ±180)
    {
        Node    nodeT   =   new FENode(ctx);
        nodeT->setLocalEuler(real3(0.0,0.0,720.0));
        Animation   animT   =   FEAnimationHelper::createNodeAnimtion(ctx,NP_ROTATE_XYZ);
        animT->clip()->addKeyFrame(0.0,real3(0.0,0.0,0.0));
        animT->clip()->addKeyFrame(2.0,real3(0.0,0.0,180.0));
        nodeT->addComponent(Component(animT.get()));
        scene->dispatchNodesToSystem({nodeT});
        animT->update(1.0);
        checkReal("恢复前:采样 Z",nodeT->localEuler().z,90.0);
        animT->setEnable(false);
        animT->update(1.0);
        checkReal("停用后:恢复原始账本 Z",nodeT->localEuler().z,720.0);
        checkReal("停用后:账本有效",nodeT->localEulerValid() ? 1.0 : 0.0,1.0);
    }

    /// 7. 恢复动画并交给播放器循环播放,进入渲染循环
    anim->setEnable(true);
    anim->update(0.0);
    node->update();
    if (auto  sys =   scene->animationSystem())
    {
        auto    action  =   sys->getOrCreate();
        action->setMode(playMode);
        action->play();
        /// 播放中禁止编辑:写值被拒绝
        bool    edited  =   sys->setNodeProperty(Object(node.get()),PROP_TRANSFORM_XYZ,real3(9.0,9.0,9.0));
        checkReal("播放中:编辑被拒绝",edited ? 1.0 : 0.0,0.0);
    }
    std::cout << "playMode=" << (playMode == FEPlayback::PT_PingPong ? "pingpong" : "loop") << std::endl;

    std::cout << "test.animation " << (passed ? "PASSED" : "FAILED") << std::endl;
    if (!passed)
    {
        scene   =   nullptr;
        app->destroy();
        return  1;
    }

    /// 8. 进入渲染循环:立方体在 0~1.2 之间循环平移
    app->run();

    scene   =   nullptr;
    app->destroy();

    return  0;
}
