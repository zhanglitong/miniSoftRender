// anim.js
// 纯脚本创建动画:C++ 侧没有任何动画代码
// 效果:Box 从 (0,0,0) 平移到 (10,0,0),历时 2 秒,循环播放
// 与 anim.lua 逐行对齐;唯一差异:JS 索引 0-based,Lua getTrack(1) 对应 JS getTrack(0)

var node = fe.findNode("Box");
if (node === null)
    throw new Error("fe.findNode('Box') failed: host must create a node named 'Box'");

// 1. 创建含 Position(XYZ)轨道的动画(见 FEAnimationHelper::createNodeAnimtion)
var anim = fe.anim.createNodeAnimation(fe.anim.NP_TRANSFORM_XYZ);
anim.setName("BoxMove");
anim.setOffset(0.0);

// 2. 向 clip 填充关键帧:对象兼容 {x,y,z} 与数组 [x,y,z]
var clip = anim.clip();
clip.addKeyFrame(0.0, { x: 0.0, y: 0.0, z: 0.0 });
clip.addKeyFrame(2.0, { x: 10.0, y: 0.0, z: 0.0 });

// 3. 只读接口演示:轨道数量/名字/关键帧数/时间范围
var track = clip.getTrack(0);
fe.log("clip: " + clip.trackCount() + " track(s), track0 '" + track.name() +
       "' prop=" + track.propertyIndex() + " keys=" + track.keyCount() +
       " range=[" + clip.range().min + "," + clip.range().max + "]");

// 4. 动画作为组件挂到节点,并分发到动画系统
node.addComponent(anim);
fe.dispatchNode(node);

// 5. 播放控制:默认 Action,循环模式
var act = fe.anim.action("default");
act.setMode(fe.anim.PT_LOOP);
act.play();

fe.log("script animation started: Box (0,0,0) -> (10,0,0) in 2s, LOOP");
