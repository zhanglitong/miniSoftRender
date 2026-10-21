-- anim.lua
-- 纯脚本创建动画:C++ 侧没有任何动画代码
-- 效果:Box 从 (0,0,0) 平移到 (10,0,0),历时 2 秒,循环播放

local node = fe.findNode("Box")
if node == nil then
    error("fe.findNode('Box') failed: host must create a node named 'Box'")
end

-- 1. 创建含 Position(XYZ)轨道的动画(见 FEAnimationHelper::createNodeAnimtion)
local anim = fe.anim.createNodeAnimation(fe.anim.NP_TRANSFORM_XYZ)
anim:setName("BoxMove")
anim:setOffset(0.0)

-- 2. 向 clip 填充关键帧:table 兼容 {x=,y=,z=} 与 {1,2,3}
local clip = anim:clip()
clip:addKeyFrame(0.0, {x = 0.0,  y = 0.0, z = 0.0})
clip:addKeyFrame(2.0, {x = 10.0, y = 0.0, z = 0.0})

-- 3. 只读接口演示:轨道数量/名字/关键帧数/时间范围
local track = clip:getTrack(1)
fe.log(string.format("clip: %d track(s), track1 '%s' prop=%d keys=%d range=[%g,%g]",
        clip:trackCount(), track:name(), track:propertyIndex(),
        track:keyCount(), clip:range().min, clip:range().max))

-- 4. 动画作为组件挂到节点,并分发到动画系统
node:addComponent(anim)
fe.dispatchNode(node)

-- 5. 播放控制:默认 Action,循环模式
local act = fe.anim.action("default")
act:setMode(fe.anim.PT_LOOP)
act:play()

fe.log("script animation started: Box (0,0,0) -> (10,0,0) in 2s, LOOP")
