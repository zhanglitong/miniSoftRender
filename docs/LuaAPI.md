# miniSoftRender Lua 脚本 API 手册

引擎通过 `FELuaVM` 提供 Lua 脚本宿主，脚本中所有引擎功能都挂在全局表 **`fe`** 下，分为四个部分：

| 模块 | 内容 | 实现文件 |
|------|------|----------|
| `fe.*` | 核心函数：日志、节点查找/创建、渲染分发 | `FELuaVM.cpp` |
| `fe.vec2/vec3/...` | 数学类型构造函数与运算 | `FELuaMath.cpp` |
| `fe.geo.*` | 13 种参数化几何工厂，产出 `Mesh` 对象 | `FELuaGeo.cpp` |
| `fe.anim.*` | 动画 clip/track/关键帧/播放控制 | `FELuaAnim.cpp` |

宿主侧（C++）用法：

```cpp
FELuaVM lua(ctx);
lua.setScene(scene.get());   // fe.findNode / fe.anim.action 依赖场景
lua.doFile("anim.lua");      // 失败返回 false 并写错误日志
```

---

## 目录

- [快速上手](#快速上手)
- [1. fe 核心函数](#1-fe-核心函数)
- [2. 数学类型](#2-数学类型)
  - [2.1 构造函数](#21-构造函数)
  - [2.2 向量 vec2/vec3/vec4](#22-向量-vec2vec3vec4)
  - [2.3 四元数 quat](#23-四元数-quat)
  - [2.4 矩阵 mat3/mat4](#24-矩阵-mat3mat4)
- [3. Node 节点类型](#3-node-节点类型)
- [4. Mesh 网格类型](#4-mesh-网格类型)
- [5. fe.geo 几何工厂](#5-fegeo-几何工厂)
- [6. fe.anim 动画模块](#6-feanim-动画模块)
- [7. 调用示例](#7-调用示例)
  - [7.1 数学运算](#71-数学运算)
  - [7.2 参数化几何与网格查询](#72-参数化几何与网格查询)
  - [7.3 节点变换与层级](#73-节点变换与层级)
  - [7.4 动画：手动创建轨道](#74-动画手动创建轨道)
  - [7.5 动画：从节点记录关键帧](#75-动画从节点记录关键帧)
  - [7.6 播放控制](#76-播放控制)
  - [7.7 综合示例：从场景搭建到播放](#77-综合示例从场景搭建到播放)
- [附录 A：table 参数约定](#附录-atable-参数约定)
- [附录 B：值类型字符串](#附录-b值类型字符串)
- [附录 C：源码索引](#附录-c源码索引)

---

## 快速上手

以下示例（摘自 `source/test/test.lua.anim/anim.lua`）用纯脚本让 Box 在 2 秒内从原点平移到 (10,0,0) 并循环播放：

```lua
local node = fe.findNode("Box")            -- 1. 查找场景节点

local anim = fe.anim.createNodeAnimation(fe.anim.NP_TRANSFORM_XYZ)  -- 2. 创建动画
local clip = anim:clip()
clip:addKeyFrame(0.0, {x = 0.0,  y = 0.0, z = 0.0})   -- 3. 填关键帧
clip:addKeyFrame(2.0, {x = 10.0, y = 0.0, z = 0.0})

node:addComponent(anim)                    -- 4. 挂为节点组件
fe.dispatchNode(node)                      -- 5. 分发到动画/渲染系统

local act = fe.anim.action("default")      -- 6. 播放控制
act:setMode(fe.anim.PT_LOOP)
act:play()
```

**通用约定**

- 引擎对象（Node/Mesh/Animation 等）在 Lua 中是 userdata，用冒号 `:` 调用方法：`node:setName("xx")`。
- Node 的 setter 方法统一返回自身，支持链式调用：`node:setName("a"):setLocalTranslation({0,1,0})`。
- 数学参数同时兼容分量 table（`{x=1,y=2,z=3}`）与数组 table（`{1,2,3}`），也直接接受 math userdata（`fe.vec3(1,2,3)`）。
- 所有索引从 **1** 开始（Lua 惯例）；引擎内部索引由绑定层自动换算。
- 脚本执行出错时 `doFile/doString` 返回 false，错误写入引擎日志。

---

## 1. fe 核心函数

定义于 `FELuaVM.cpp`，是脚本与场景/引擎交互的入口。

| 函数 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `fe.log(msg)` | 写引擎日志（前缀 `[lua]`） | `msg`: string，日志内容 | 无 |
| `fe.findNode(name)` | 在场景节点树中**深度优先**查找节点（含顶层），按名字精确匹配 | `name`: string，节点名 | 找到返回 `Node` 对象，否则返回 `nil` |
| `fe.createNode([name])` | 创建节点并加入场景节点树顶层 | `name`: string，可选，节点名 | 新建的 `Node` 对象 |
| `fe.dispatchNode(node)` | 把节点分发到渲染工厂与动画等组件系统。**需在节点添加动画组件后、play 之前调用** | `node`: Node 对象 | 无 |

> `fe.findNode` / `fe.createNode` / `fe.dispatchNode` 依赖宿主已调用 `setScene`，否则抛出 Lua 错误 `"scene not set"`。

---

## 2. 数学类型

### 2.1 构造函数

挂在 `fe` 表上的全局构造函数（定义于 `FELuaMath.cpp`）：

| 函数 | 作用 | 参数形式 | 返回 |
|------|------|----------|------|
| `fe.vec2(...)` | 构造二维向量 | 见下方向量构造规则 | `vec2` userdata |
| `fe.vec3(...)` | 构造三维向量 | 同上 | `vec3` userdata |
| `fe.vec4(...)` | 构造四维向量 | 同上；传 `{x,y,z}` 时 w 补 0 | `vec4` userdata |
| `fe.quat(...)` | 构造四元数 | 无参→单位四元数；`(x,y,z,w)` 4 个数；quat；table `{x,y,z,w}` | `quat` userdata |
| `fe.quatAngleAxis(angle, axis)` | 绕任意轴构造旋转四元数（axis 自动归一化） | `angle`: 弧度；`axis`: vec3 | `quat` |
| `fe.mat3(...)` | 构造 3x3 矩阵 | 无参→单位阵；mat3 拷贝；quat（旋转矩阵） | `mat3` userdata |
| `fe.mat4(...)` | 构造 4x4 矩阵 | 无参→单位阵；mat4 拷贝；quat（旋转矩阵） | `mat4` userdata |
| `fe.mat4Perspective(fovy, aspect, near, far)` | 构造透视投影矩阵 | `fovy`: 垂直视场角（弧度）；`aspect`: 宽高比；`near`/`far`: 近/远裁剪面 | `mat4` |
| `fe.mat4LookAt(eye, center, up)` | 构造观察矩阵 | `eye`/`center`/`up`: vec3 | `mat4` |

**向量构造规则**（以 vec3 为例，其余同）：

```lua
fe.vec3()                  -- 全 0
fe.vec3(1, 2, 3)           -- 按分量
fe.vec3(5)                 -- 全部分量填 5
fe.vec3({x = 1, y = 2, z = 3})   -- 分量 table
fe.vec3({1, 2, 3})         -- 数组 table
fe.vec3(v)                 -- 拷贝另一个 vec3
```

> **四元数分量顺序约定**：Lua 侧统一为 `(x, y, z, w)`，与 table `{x=,y=,z=,w=}` 一致（注意 glm C++ 构造函数是 `(w,x,y,z)`，绑定层已处理，脚本无需关心）。

### 2.2 向量 vec2/vec3/vec4

**运算符**（通过元表实现，vec2/vec3/vec4/quat 通用）：

| 运算符 | 语义 |
|--------|------|
| `a + b` | 逐分量加 |
| `a - b` | 逐分量减 |
| `a * b` / `a * n` / `n * a` | 向量逐分量乘，或标量缩放 |
| `a / b` / `a / n` | 逐分量除，或除以标量 |
| `-a` | 逐分量取负 |
| `a == b` | 逐分量相等比较 |
| `#a` | 返回维度（2/3/4） |
| `tostring(a)` | 形如 `vec3(1, 2, 3)` |

**索引访问**（`__index`/`__newindex`）：

- 数字索引 **1 起始**：`v[1]`、`v[2]`…越界返回 `nil`。
- 字符串分量名：`v.x`、`v.y`、`v.z`、`v.w`（按维度）。
- 可写：`v.x = 5` 或 `v[1] = 5`。
- 其他字符串键回落到方法表（即 `v:length()` 也可写作 `v.length`）。

**方法**（vec2/vec3/vec4 通用，冒号调用）：

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `v:length()` | 向量模长 | 无 | number |
| `v:dot(b)` | 点积 | `b`: 同类型向量 | number |
| `v:distance(b)` | 两向量间距 | `b`: 同类型向量 | number |
| `v:normalize()` | 返回归一化后的新向量（不改自身） | 无 | 同类型向量 |
| `v:lerp(b, t)` | 线性插值 `mix(a,b,t)` | `b`: 同类型向量；`t`: number，插值系数 | 同类型向量 |
| `v:cross(b)` | **仅 vec3**：叉积 | `b`: vec3 | vec3 |

### 2.3 四元数 quat

除通用向量运算符（`+ - == # -a`，见上）外，quat 的 `*` 有三种重载：

| 表达式 | 语义 |
|--------|------|
| `q1 * q2` | 四元数复合（旋转叠加） |
| `q * v` | 用四元数旋转 vec3，返回旋转后的 vec3 |
| `q * n` / `n * q` | 标量缩放 |

**方法**：

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `q:conj()` | 共轭 | 无 | quat |
| `q:normalize()` | 归一化（返回新值） | 无 | quat |
| `q:dot(b)` | 点积 | `b`: quat | number |
| `q:slerp(b, t)` | 球面插值 | `b`: quat；`t`: number | quat |
| `q:angle()` | 旋转角（弧度） | 无 | number |
| `q:axis()` | 旋转轴 | 无 | vec3 |
| `q:toMat3()` | 转 3x3 旋转矩阵 | 无 | mat3 |
| `q:toMat4()` | 转 4x4 旋转矩阵 | 无 | mat4 |

### 2.4 矩阵 mat3/mat4

**运算符**：

| 表达式 | 语义 |
|--------|------|
| `m1 * m2` | 矩阵乘矩阵 |
| `m * v` | 矩阵乘列向量（mat3 × vec3，mat4 × vec4） |
| `m * n` / `n * m` | 标量缩放 |
| `m1 == m2` | 相等比较 |
| `tostring(m)` | 形如 `mat4(4x4)` |

**索引访问**：

- `m[i]`（1 起始）返回第 i **列**向量：mat3 → vec3，mat4 → vec4。
- 列可整体赋值：`m[4] = fe.vec4(5, 6, 7, 1)`。
- 元素访问：`m[3][3]`。

**方法**：

| 方法 | 适用 | 作用 | 参数 | 返回 |
|------|------|------|------|------|
| `m:transpose()` | mat3/mat4 | 转置（返回新值） | 无 | 同类型矩阵 |
| `m:inverse()` | mat3/mat4 | 求逆（返回新值） | 无 | 同类型矩阵 |
| `m:determinant()` | mat3/mat4 | 行列式 | 无 | number |
| `m:transformPoint(v)` | 仅 mat4 | 变换**点**（平移生效），等价 `m * vec4(v,1)` 取 xyz | `v`: vec3 | vec3 |
| `m:transformVector(v)` | 仅 mat4 | 变换**方向**（平移不生效），等价 `m * vec4(v,0)` 取 xyz | `v`: vec3 | vec3 |

---

## 3. Node 节点类型

对应引擎 `FENode`，可由 `fe.findNode` / `fe.createNode` 获得。除特别说明外，**setter 均返回自身支持链式调用**。

### 3.1 名称与可见性

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `n:name()` | 取节点名 | 无 | string |
| `n:setName(name)` | 设置节点名 | `name`: string | self |
| `n:visible()` | 查询可见性 | 无 | boolean |
| `n:visible(flag)` | 设置可见性（**此形式无返回值**） | `flag`: boolean | 无 |
| `n:color()` | 取节点颜色 | 无 | table `{r,g,b,a}`，分量 0–255 |
| `n:setColor(c)` | 设置节点颜色 | `c`: table `{r,g,b,a}` 或 `{r,g,b}`，0–255 | self |

### 3.2 局部变换

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `n:localTranslation()` | 取局部平移 | 无 | vec3 |
| `n:setLocalTranslation(pos)` | 设局部平移 | `pos`: vec3 / `{x,y,z}` / `{1,2,3}` | self |
| `n:localScaling()` | 取局部缩放 | 无 | vec3 |
| `n:setLocalScaling(scale)` | 设局部缩放 | `scale`: vec3 / table | self |
| `n:localRotation()` | 取局部旋转 | 无 | quat |
| `n:setLocalRotation(rot)` | 设局部旋转 | `rot`: quat / `{x,y,z,w}` | self |
| `n:localTransform()` | 取局部变换矩阵（TR 合成） | 无 | mat4 |

### 3.3 全局变换

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `n:globalTranslation()` | 取全局平移 | 无 | vec3 |
| `n:globalRotation()` | 取全局旋转 | 无 | quat |
| `n:globalScaling()` | 取全局缩放 | 无 | vec3 |
| `n:globalTransform()` | 取全局变换矩阵 | 无 | mat4 |
| `n:globalRSTransform()` | 取全局旋转+缩放矩阵（不含平移） | 无 | mat4 |
| `n:setGlobalTranslation(pos)` | 设全局平移（反解局部值） | `pos`: vec3 / table | self |
| `n:setGlobalRotation(rot)` | 设全局旋转 | `rot`: quat / table | self |
| `n:setGlobalScaling(scale)` | 设全局缩放 | `scale`: vec3 / table | self |

### 3.4 层级关系

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `n:parent()` | 取父节点 | 无 | Node；顶层节点返回 `nil` |
| `n:root()` | 取根节点 | 无 | Node |
| `n:children()` | 取全部子节点 | 无 | 数组 table（1 起始），元素为 Node |
| `n:addChild(child)` | 添加子节点 | `child`: Node | 无 |
| `n:removeChild(child)` | 移除子节点 | `child`: Node | boolean，是否成功 |
| `n:reparent([parent][, index])` | 重新挂接父子关系并**保持世界变换不变** | `parent`: Node，可省略或传 `nil` 表示提升为顶层；`index`: 新父孩子列表的 **0 起始**插入位置，缺省追加末尾 | 无 |

### 3.5 网格 / 组件 / 更新

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `n:mesh()` | 取节点网格 | 无 | Mesh；没有则返回 `nil` |
| `n:setMesh(mesh)` | 设置节点网格 | `mesh`: Mesh | self |
| `n:addComponent(comp)` | 添加组件（当前脚本绑定中仅 Animation 是组件） | `comp`: Animation 对象 | boolean，是否成功 |
| `n:removeComponent(comp)` | 移除组件 | `comp`: Animation 对象 | boolean，是否成功 |
| `n:update([tm])` | 刷新节点（变换合成、组件更新等） | `tm`: number，动画时间，缺省 0 | 无 |
| `n:fireChanged()` | 触发节点变更通知 | 无 | 无 |
| `n:makeDirty()` | 标记节点脏 | 无 | self |
| `n:updateAabb([recursion])` | 计算并返回局部 AABB | `recursion`: boolean，是否递归子节点，缺省 true | table `{min = vec3, max = vec3}` |
| `n:globalAabb()` | 取全局 AABB | 无 | table `{min = vec3, max = vec3}` |

---

## 4. Mesh 网格类型

对应引擎 `FEMesh`，由 `fe.geo.*` 工厂或 `node:mesh()` 获得。全部为只读查询方法。

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `m:vertexCount()` | 顶点数（POSITION 属性缓冲的元素个数） | 无 | integer |
| `m:aabb()` | 取网格局部包围盒 | 无 | table `{min = vec3, max = vec3}` |
| `m:updateAabb()` | 重算并返回包围盒 | 无 | table `{min = vec3, max = vec3}` |
| `m:attributes()` | 列出网格包含的属性流名 | 无 | 数组 table，元素为字符串：`"POSITION"` `"NORMAL"` `"TEXCOORD0"` `"COLOR0"`（未知槽位为 `"SLOT_0x…"`） |

---

## 5. fe.geo 几何工厂

`fe.geo` 子表提供 13 种参数化几何工厂，调用形式统一为：

```lua
local mesh = fe.geo.box{ size = {1,1,1}, segs = {2,2,2} }
```

- 参数为一个 **table，所有字段均可选**，缺省使用默认值；也可以完全不传参：`fe.geo.sphere()`。
- 返回 `Mesh` 对象。
- 默认输出属性流为 **POSITION + NORMAL + TEXCOORD0**；`grid` 例外，输出 **POSITION + COLOR0**。

### fe.geo.box

长方体。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `size` | `{x,y,z}` | `{1,1,1}` | 三轴尺寸 |
| `segs` | `{x,y,z}` | `{1,1,1}` | 三轴分段数 |

### fe.geo.sphere

球体（经纬分段）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radius` | number | `1.0` | 半径 |
| `widthSegments` | int | `16` | 经度分段 |
| `heightSegments` | int | `12` | 纬度分段 |
| `phiStart` / `phiLength` | number | `0` / `2π` | 经度起始角/跨度（弧度，可做部分球） |
| `thetaStart` / `thetaLength` | number | `0` / `π` | 纬度起始角/跨度（弧度） |

### fe.geo.cylinder

圆柱/圆台。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radiusTop` | number | `1.0` | 顶面半径 |
| `radiusBottom` | number | `1.0` | 底面半径 |
| `height` | number | `1.0` | 高度 |
| `radialSegments` | int | `16` | 圆周分段 |
| `heightSegments` | int | `1` | 高度分段 |
| `thetaStart` / `thetaLength` | number | `0` / `2π` | 圆周起始角/跨度 |
| `openEnded` | bool | `false` | true 时无顶/底盖 |

### fe.geo.cone

圆锥（radiusTop 固定为 0）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radius`（或 `radiusBottom`） | number | `1.0` | 底面半径，两个键等价，`radius` 优先 |
| `height` | number | `1.0` | 高度 |
| `radialSegments` | int | `32` | 圆周分段 |
| `heightSegments` | int | `1` | 高度分段 |
| `openEnded` | bool | `false` | true 时无底盖 |

### fe.geo.pyramid

棱锥（底面为 `radialSegments` 边形，radiusTop 固定为 0）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radius`（或 `radiusBottom`） | number | `1.0` | 底面外接半径 |
| `height` | number | `1.0` | 高度 |
| `radialSegments` | int | `4` | 底面边数（4 即四棱锥） |
| `openEnded` | bool | `false` | true 时无底面 |

### fe.geo.torus

圆环。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radius` | number | `1.0` | 环中心半径 |
| `tube` | number | `0.4` | 管半径 |
| `radialSegments` | int | `20` | 管截面分段 |
| `tubularSegments` | int | `64` | 环向分段 |
| `arc` | number | `2π` | 环向跨度（弧度） |

### fe.geo.torusknot

环面纽结。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radius` | number | `1.0` | 环中心半径 |
| `tube` | number | `0.4` | 管半径 |
| `tubularSegments` | int | `64` | 环向分段 |
| `radialSegments` | int | `16` | 管截面分段 |
| `p` / `q` | int | `2` / `3` | 纽结参数 (p,q) |

### fe.geo.circle

扇形/圆盘。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `radius` | number | `1.0` | 半径 |
| `thetaStart` / `thetaLength` | number | `0` / `2π` | 起始角/跨度（弧度） |
| `segments` | int | `16` | 分段数 |

### fe.geo.ring

圆环盘（平面垫圈形）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `innerRadius` | number | `0.5` | 内半径 |
| `outerRadius` | number | `1.0` | 外半径 |
| `thetaStart` / `thetaLength` | number | `0` / `2π` | 起始角/跨度（弧度） |
| `thetaSegments` | int | `16` | 圆周分段 |
| `phiSegments` | int | `1` | 径向分段 |

### fe.geo.plane

平面矩形。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `width` | number | `1.0` | 宽 |
| `height` | number | `1.0` | 高 |
| `wSegs` | int | `1` | 宽方向分段 |
| `hSegs` | int | `1` | 高方向分段 |

### fe.geo.grid

带颜色的参考网格线（输出 POSITION + COLOR0）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `size` | int | `10` | 网格边长（格数） |
| `divs` | int | `10` | 分割数 |
| `color1` | `{r,g,b,a}` | 红 `(255,0,0,255)` | X 轴线颜色 |
| `color2` | `{r,g,b,a}` | 绿 `(0,255,0,255)` | Y 轴线颜色 |
| `color3` | `{r,g,b,a}` | 蓝 `(0,0,255,255)` | Z 轴线颜色 |
| `color4` | `{r,g,b,a}` | 灰 `(180,180,180,255)` | 普通网格线颜色 |
| `dir` | int | `0` | 网格朝向 |

### fe.geo.ellipsoid

椭球（EGM 细分）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `size` | `{x,y,z}` | `{1,1,1}` | 三轴半径 |
| `segs` | `{x,y,z}` | `{10,10,10}` | 三轴分段 |

### fe.geo.tube

沿曲线扫掠的管体（支持直线/贝塞尔/Catmull-Rom）。

| 参数 | 类型 | 默认 | 说明 |
|------|------|------|------|
| `type` | string | `"catmullrom"` | 曲线类型，大小写不敏感：`"line"` / `"quadratic"` / `"cubic"` / `"catmullrom"` |
| `catmullType` | string | 引擎默认 | 仅 catmullrom 有效：`"centripetal"` / `"chordal"` / `"uniform"` |
| `points` | `{{x,y,z},...}` | 空 | 曲线控制点数组 |
| `radius` | number | `0.1` | 管半径 |
| `tubularSegments` | int | `64` | 沿曲线分段 |
| `radialSegments` | int | `8` | 管截面分段 |
| `closed` | bool | `false` | 曲线是否闭合 |
| `tension` | number | `0.5` | 曲线张力 |

---

## 6. fe.anim 动画模块

脚本可创建 clip/track/关键帧，绑定节点并控制播放。对象模型：

```
fe.anim.action(name) ──> Playback（播放器，可挂多个 Animation）
                          └── Animation（动画，持 1 个 clip，作为 Node 组件）
                                └── AnimClip（时间线，多条轨道）
                                      └── KeyFrameTrack（单属性轨道，关键帧序列）
```

### 6.1 模块函数

| 函数 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `fe.anim.createAnimation()` | 创建空动画（无 clip） | 无 | `Animation` |
| `fe.anim.createNodeAnimation([bits])` | 创建含默认节点属性轨道的动画（由 `FEAnimationHelper` 按属性位生成轨道） | `bits`: number，NP_* 位组合，缺省 `NP_Default`（位移 XYZ + 缩放 XYZ + 旋转 XYZ） | `Animation` |
| `fe.anim.createClip()` | 创建动画片段 | 无 | `AnimClip` |
| `fe.anim.createTrack(propIndex, valueType)` | 创建轨道并初始化时间线/值数组 | `propIndex`: number，PROP_* 属性索引；`valueType`: string，值类型，见附录 B | `KeyFrameTrack` |
| `fe.anim.action([name])` | 取场景动画系统中的播放器（不存在则创建），名字缺省 `"default"` | `name`: string | `Playback` |
| `fe.anim.addKeyFrameFromNode(anim, time, node[, bits])` | 把节点当前状态记为关键帧写入动画 | `anim`: Animation；`time`: number；`node`: Node；`bits`: NP_* 位组合，缺省全部属性（0xFFFFFFFF） | boolean |
| `fe.anim.updateKeyFrameFromNode(anim, time, node[, bits])` | 更新指定时间的关键帧为节点当前状态 | 同上 | boolean |
| `fe.anim.removeKeyFrameFromNode(anim, time)` | 移除指定时间的关键帧 | `anim`: Animation；`time`: number | boolean |

### 6.2 常量

定义于 `FELuaAnim.cpp`，挂在 `fe.anim` 表上，值为 integer。

**节点属性位（NP_\*，位标志，可用 `+` 组合传给 `createNodeAnimation` / `addKeyFrameFromNode` 的 bits 参数）**

| 常量 | 含义 |
|------|------|
| `NP_TRANSFORM_X` / `NP_TRANSFORM_Y` / `NP_TRANSFORM_Z` | 单轴位移轨道 |
| `NP_TRANSFORM_XYZ` | 三轴位移轨道 |
| `NP_SCALE_X` / `NP_SCALE_Y` / `NP_SCALE_Z` / `NP_SCALE_XYZ` | 缩放轨道 |
| `NP_ROTATE_X` / `NP_ROTATE_Y` / `NP_ROTATE_Z` / `NP_ROTATE_XYZ` | 欧拉角轨道 |
| `NP_QUAT` | 四元数旋转轨道 |
| `NP_COLOR_RGB` / `NP_COLOR_ALPHA` | 颜色 RGB / 透明度轨道 |
| `NP_VISIBLE` | 可见性轨道 |
| `NP_Default` | 默认组合 = TRANSFORM_XYZ \| SCALE_XYZ \| ROTATE_XYZ |

**属性索引（PROP_\*，传给 `createTrack` 的 propIndex）**

| 常量 | 含义 |
|------|------|
| `PROP_TRANSFORM_X/Y/Z` | 位移单轴（real） |
| `PROP_TRANSFORM_XYZ` | 位移三轴（real3） |
| `PROP_SCALE_X/Y/Z` | 缩放单轴（float） |
| `PROP_SCALE_XYZ` | 缩放三轴（float3） |
| `PROP_ROTATE_X/Y/Z` | 欧拉角单轴（float，单位：角度） |
| `PROP_ROTATE_XYZ` | 欧拉角三轴 |
| `PROP_QUAT` | 四元数旋转（quatf） |
| `PROP_COLOR_RGB` | 材质颜色（float3） |
| `PROP_COLOR_ALPHA` | 透明度（float） |
| `PROP_VISIBLE` | 可见性（uint8） |
| `PROP_MAT_DIFFUSE` | 材质环境光（float3） |
| `PROP_G_TRANSFORM_XYZ` | 全局位移 |

**插值方式（IT_\*，传给 `track:setInterpolation`）**

| 常量 | 含义 |
|------|------|
| `IT_BEZIER` | 贝塞尔插值（默认，可用 `tension` 调节） |
| `IT_LINEAR` | 线性插值 |
| `IT_CONSTANT` | 阶跃插值 |

**播放模式（PT_\*，传给 `playback:setMode`）**

| 常量 | 含义 |
|------|------|
| `PT_ONCE` | 播放一次 |
| `PT_LOOP` | 循环播放 |
| `PT_PINGPONG` | 乒乓循环 |

**时间模式（TM_\*）**

| 常量 | 含义 |
|------|------|
| `TM_DEFAULT` | 正向播放 |
| `TM_INVERT` | 倒放 |

### 6.3 Animation 对象

动画本体，可作为组件挂到 Node。

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `a:name()` / `a:setName(name)` | 名字查询/设置 | string | string / 无 |
| `a:offset()` / `a:setOffset(offset)` | 时间偏移查询/设置 | number | number / 无 |
| `a:clip()` | 取动画片段 | 无 | `AnimClip` |
| `a:setClip(clip)` | 替换动画片段 | `clip`: AnimClip | 无 |
| `a:range()` | 取时间范围 | 无 | table `{min = number, max = number}` |
| `a:isValid()` | 动画是否有效 | 无 | boolean |
| `a:isChanged()` | 是否有未应用的修改 | 无 | boolean |
| `a:clearChanged()` | 清除修改标记 | 无 | 无 |

### 6.4 AnimClip 对象

时间线，容纳多条轨道，也可直接添加“标量关键帧”。

**关键帧操作**（值参数支持标量 number、boolean、向量 table、颜色 table，转换规则见附录 A）：

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `c:addKeyFrame(time, val)` | 末尾追加关键帧 | `time`: number；`val`: 任意可转换值 | boolean |
| `c:insertKeyFrame(idx, time, val)` | 在指定位置插入关键帧 | `idx`: **1 起始**索引；`time`；`val` | boolean |
| `c:removeKeyFrame(key)` | 按索引或按时间移除关键帧 | `key`: 整数（1 起始索引）或 number（时间） | boolean |
| `c:updateKeyFrame(key, val)` | 按索引或按时间更新关键帧值 | `key`: 整数（1 起始）或时间；`val` | boolean |

**轨道与范围**：

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `c:addTrack(track)` | 添加轨道 | `track`: KeyFrameTrack | integer，轨道索引（0 起始） |
| `c:trackCount()` | 轨道数量 | 无 | integer |
| `c:getTrack(idx)` | 取轨道 | `idx`: **1 起始**索引，越界抛错 | `KeyFrameTrack` |
| `c:range()` | 取时间范围 | 无 | table `{min, max}` |
| `c:simplifyTimeline()` | 简化时间线（去除冗余关键帧） | 无 | 无 |

### 6.5 KeyFrameTrack 对象

单属性关键帧轨道（由 `fe.anim.createTrack` 创建或从 clip 取得）。

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `t:name()` / `t:setName(name)` | 轨道名（如 `"Position.X"`） | string | string / 无 |
| `t:propertyIndex()` / `t:setPropertyIndex(idx)` | 属性索引（PROP_*）查询/设置 | integer | integer / 无 |
| `t:addKeyFrame(time, val)` | 添加关键帧 | `time`; `val`: 与轨道值类型匹配的值 | boolean |
| `t:removeKeyFrame(key)` | 按索引（1 起始整数）或时间移除关键帧 | integer / number | boolean |
| `t:updateKeyFrame(key, val)` | 按索引或时间更新关键帧 | 同上 + `val` | boolean |
| `t:keyCount()` | 关键帧数量 | 无 | integer |
| `t:times()` | 全部关键帧时间 | 无 | 数组 table，元素 number |
| `t:values()` | 全部关键帧值 | 无 | 数组 table：标量为 number，向量为 `{x,y[,z[,w]]}`，颜色为 `{r,g,b,a}` |
| `t:range()` | 时间范围 | 无 | table `{min, max}` |
| `t:isValid()` | 轨道是否有效 | 无 | boolean |
| `t:interpolation()` / `t:setInterpolation(type)` | 插值方式查询/设置（IT_* 常量） | integer | integer / 无 |
| `t:tension()` / `t:setTension(tension)` | 张力系数查询/设置（影响 Bezier 插值形状） | number | number / 无 |

### 6.6 Playback 对象（播放器）

由 `fe.anim.action(name)` 获得，控制一组动画的播放。

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `p:play()` | 开始播放 | 无 | 无 |
| `p:stop()` | 停止播放 | 无 | 无 |
| `p:setMode(mode)` | 设置播放模式（PT_ONCE / PT_LOOP / PT_PINGPONG） | integer | 无 |
| `p:status()` | 播放状态：`0` = Running，`1` = Stoped | 无 | integer |
| `p:setClipTime(time)` | 设置当前动画时间（可做拖动进度条） | number | 无 |
| `p:buildCache()` | 构建播放缓存，提升播放性能（适合只读浏览场景；编辑态慎用） | 无 | 无 |
| `p:clearCache()` | 清除缓存，恢复普通逐帧求值模式 | 无 | 无 |
| `p:isCacheMode()` | 是否处于缓存模式 | 无 | boolean |
| `p:refreshRange()` | 重算时间范围并清缓存（编辑关键帧后调用） | 无 | 无 |
| `p:addAnimation(anim)` | 添加动画 | `anim`: Animation | integer，加入位置 |
| `p:removeAnimation(anim)` | 移除动画 | `anim`: Animation | integer，移除数量 |
| `p:count()` | 挂载的动画数量 | 无 | integer |

### 6.7 AnimationSys 对象

场景动画系统，脚本侧通常不直接持有，而是通过 `fe.anim.action()` 间接触达。

| 方法 | 作用 | 参数 | 返回 |
|------|------|------|------|
| `s:getOrCreate([name])` | 获取或创建播放器，名字缺省 `"default"` | string | `Playback` |
| `s:count()` | 播放器数量 | 无 | integer |
| `s:clear()` | 清空全部播放器 | 无 | 无 |

---

## 7. 调用示例

以下示例均可放入 `.lua` 文件由 `FELuaVM::doFile` 执行（涉及 `fe.findNode` 的示例要求宿主已用 `setScene` 关联场景并创建了同名节点）。

### 7.1 数学运算

```lua
-- ---------- 向量:构造/分量访问/运算/方法 ----------
local a = fe.vec3(1, 0, 0)
local b = fe.vec3(0, 1, 0)
local c = a + b                      -- (1,1,0),同样支持 - * / 与标量运算
c.y = 5                              -- 分量写入,也支持 c[2] = 5(1 起始)
local dir = c:normalize()            -- 归一化,返回新向量,不修改 c
fe.log(string.format("len=%g dist=%g", a:length(), a:distance(b)))
fe.log(tostring(a:cross(b)))         -- 叉积(仅 vec3):vec3(0, 0, 1)
fe.log(tostring(a:lerp(b, 0.5)))     -- 线性插值:vec3(0.5, 0.5, 0)
fe.log(tostring(-a))                 -- 取负:vec3(-1, 0, 0)

-- ---------- 四元数:轴角构造/旋转向量/插值 ----------
local q = fe.quatAngleAxis(math.rad(90), fe.vec3(0, 1, 0))   -- 绕 Y 轴 90 度
local r = q * fe.vec3(1, 0, 0)       -- 用四元数旋转一个方向:vec3(0, 0, -1)
local q2 = q * q                     -- 旋转复合(累计 180 度)
local qi = fe.quat():slerp(q, 0.5)   -- 单位四元数与 q 球面插值
fe.log(string.format("angle=%g axis=%s", q:angle(), tostring(q:axis())))

-- ---------- 矩阵:投影/观察/列赋值/点与方向变换 ----------
local proj = fe.mat4Perspective(math.rad(60), 1280 / 720, 0.1, 100)
local view = fe.mat4LookAt(fe.vec3(0, 0, 5), fe.vec3(0, 0, 0), fe.vec3(0, 1, 0))
local vp   = proj * view             -- 矩阵乘矩阵
local p    = view:transformPoint(fe.vec3(0, 0, 0))    -- 变换点(平移生效):vec3(0, 0, 5)
local v    = view:transformVector(fe.vec3(0, 0, -1))  -- 变换方向(平移不生效)
view[4] = fe.vec4(1, 2, 3, 1)        -- 按列整体赋值
fe.log(string.format("det(mat3)=%g, det(inv)=%g",
        fe.mat3():determinant(), view:inverse():determinant()))
```

### 7.2 参数化几何与网格查询

```lua
-- 全默认参数
local box = fe.geo.box()

-- 自定义参数(字段全部可选,缺省用默认值)
local sphere = fe.geo.sphere{ radius = 2, widthSegments = 32, heightSegments = 24 }
local torus  = fe.geo.torus{ radius = 1, tube = 0.3, radialSegments = 24 }
local tube   = fe.geo.tube{
    type   = "catmullrom",                  -- line / quadratic / cubic / catmullrom
    points = { {0,0,0}, {2,1,0}, {4,0,1}, {6,1,0} },
    radius = 0.1, closed = false,
}

-- 网格查询:顶点数/属性流/包围盒
fe.log(string.format("sphere: %d verts, attrs=[%s]",
        sphere:vertexCount(), table.concat(sphere:attributes(), ",")))
local ab = box:aabb()
fe.log(string.format("box aabb: min=%s max=%s", tostring(ab.min), tostring(ab.max)))

-- 创建节点并挂上网格
local node = fe.createNode("Sphere")
node:setMesh(sphere)
```

### 7.3 节点变换与层级

```lua
local parent = fe.createNode("Parent")
local child  = fe.createNode("Child")
parent:addChild(child)

-- setter 链式调用(setter 均返回自身)
parent:setName("Root")
     :setLocalTranslation({0, 1, 0})
     :setLocalScaling({1, 2, 1})
     :setLocalRotation(fe.quatAngleAxis(math.rad(45), fe.vec3(0, 1, 0)))
     :setColor({r = 200, g = 80, b = 60, a = 255})
     :visible(true)

-- 全局变换:直接指定世界空间状态,内部反解局部值
child:setGlobalTranslation({5, 0, 0})
fe.log("child global pos: " .. tostring(child:globalTranslation()))

-- 层级调整:保持世界变换不变
child:reparent(nil)             -- 提升为顶层
child:reparent(parent, 0)       -- 挂回 parent 的第 0 个孩子位置

-- 包围盒与刷新
local ab = parent:updateAabb(true)   -- true=递归子节点(缺省即 true)
fe.log(string.format("aabb: (%g,%g,%g)-(%g,%g,%g)",
        ab.min.x, ab.min.y, ab.min.z, ab.max.x, ab.max.y, ab.max.z))
parent:update(0)
parent:fireChanged()

-- 遍历子节点
for i, c in ipairs(parent:children()) do
    fe.log(string.format("child[%d] = %s", i, c:name()))
end
```

### 7.4 动画：手动创建轨道

```lua
-- 0. 宿主需已创建名为 "Box" 的节点
local node = fe.findNode("Box")

-- 1. 空动画没有 clip,先补一个片段
local anim = fe.anim.createAnimation()
anim:setName("ManualMove")
anim:setClip(fe.anim.createClip())
local clip = anim:clip()

-- 2. 手动创建 real3 轨道,绑定节点位移属性并填关键帧
local track = fe.anim.createTrack(fe.anim.PROP_TRANSFORM_XYZ, "real3")
track:setName("Position")
track:setInterpolation(fe.anim.IT_LINEAR)   -- IT_BEZIER(默认) / IT_LINEAR / IT_CONSTANT
track:addKeyFrame(0.0, {0, 0, 0})
track:addKeyFrame(1.0, {0, 3, 0})
track:addKeyFrame(2.0, {0, 0, 0})
clip:addTrack(track)

-- 3. 只读检查
fe.log(string.format("keys=%d range=[%g,%g]",
        track:keyCount(), clip:range().min, clip:range().max))

-- 4. 挂组件、分发、播放
node:addComponent(anim)
fe.dispatchNode(node)
local act = fe.anim.action("moveup")
act:addAnimation(anim)
act:setMode(fe.anim.PT_LOOP)
act:play()
```

### 7.5 动画：从节点记录关键帧

```lua
local node = fe.findNode("Box")

-- 属性位可用 + 叠加;不传参则缺省 NP_Default(位移+缩放+旋转 XYZ)
local anim = fe.anim.createNodeAnimation(
    fe.anim.NP_TRANSFORM_XYZ + fe.anim.NP_SCALE_XYZ)

-- 把节点当前状态按时间记录为关键帧(bits 缺省记录全部属性)
fe.anim.addKeyFrameFromNode(anim, 0.0, node)

node:setLocalTranslation({4, 0, 0})
node:setLocalScaling({2, 2, 2})
fe.anim.addKeyFrameFromNode(anim, 2.0, node)

-- 也可更新/移除某个时刻的关键帧
fe.anim.updateKeyFrameFromNode(anim, 2.0, node)
-- fe.anim.removeKeyFrameFromNode(anim, 2.0)

node:addComponent(anim)
fe.dispatchNode(node)
fe.anim.action():play()      -- action() 不传参即 "default"
```

### 7.6 播放控制

```lua
local act = fe.anim.action("default")

act:setMode(fe.anim.PT_PINGPONG)       -- 乒乓循环:0->T->0->T...;另有 PT_ONCE / PT_LOOP
act:play()

if act:status() == 0 then              -- 0=Running 1=Stoped
    fe.log("playing")
end

-- 手动拖动进度:先停止,再逐时刻设置动画时间
act:stop()
for t = 0.0, 2.0, 0.5 do
    act:setClipTime(t)
end

-- 性能优化:只读浏览场景可开启播放缓存;需要继续编辑时先 clearCache
act:buildCache()
fe.log(act:isCacheMode() and "cache on" or "cache off")
-- act:clearCache()
-- act:refreshRange()                  -- 编辑过关键帧后重算范围并清缓存
```

### 7.7 综合示例：从场景搭建到播放

```lua
-- 地面:带颜色的参考网格(输出 POSITION + COLOR0)
local ground = fe.createNode("Ground")
ground:setMesh(fe.geo.grid{ size = 20, divs = 20 })

-- 小球挂在地面下
local ball = fe.createNode("Ball")
ball:setMesh(fe.geo.sphere{ radius = 0.5 })
ball:setLocalTranslation({0, 0.5, 0})
ground:addChild(ball)

-- 弹跳动画:单轴 real 轨道,Bezier 插值 + 张力
local anim  = fe.anim.createAnimation()
anim:setName("Bounce")
anim:setClip(fe.anim.createClip())
local track = fe.anim.createTrack(fe.anim.PROP_TRANSFORM_Y, "real")
track:setInterpolation(fe.anim.IT_BEZIER)
track:setTension(0.5)
track:addKeyFrame(0.0, 0.5)
track:addKeyFrame(0.5, 3.0)
track:addKeyFrame(1.0, 0.5)
anim:clip():addTrack(track)

-- 挂组件并分发,循环播放
ball:addComponent(anim)
fe.dispatchNode(ball)
local act = fe.anim.action("bounce")
act:addAnimation(anim)
act:setMode(fe.anim.PT_LOOP)
act:play()

fe.log("scene ready: bouncing ball on grid")
```

---

## 附录 A：table 参数约定

### 数学 table 双格式

所有接受 vec3/quat 的函数都兼容两种写法（定义于 `FELuaHelper.hpp`）：

```lua
node:setLocalTranslation({x = 0, y = 1, z = 2})   -- 分量式
node:setLocalTranslation({0, 1, 2})               -- 数组式
node:setLocalTranslation(fe.vec3(0, 1, 2))        -- math userdata
-- 四元数:{x,y,z,w} 或 {1,2,3,4} 或 quat
-- 颜色:  {r,g,b,a} 或 {1,2,3,4}，分量 0-255
```

### 关键帧值（KFValue）自动推断

`addKeyFrame` / `updateKeyFrame` 的值参数按以下优先级自动推断类型（`toKFValue`）：

| 传入值 | 推断为 |
|--------|--------|
| math userdata（vec2/vec3/vec4/quat） | 对应数学类型 |
| boolean | uint8（true=1，false=0） |
| number | real |
| table `{r,g,b,a}`（可被颜色解析） | uint8x4 颜色 |
| table 含 x,y,z 无 w（或仅 3 个数） | real3 |
| table 含 x,y,z,w（或 4 个数） | real4 |
| table 仅 x,y（或 2 个数） | real2 |

## 附录 B：值类型字符串

`fe.anim.createTrack(propIndex, valueType)` 的 `valueType` 取值（大小写敏感）：

| 类别 | 取值 |
|------|------|
| double 族 | `"real"` `"real2"` `"real3"` `"real4"` |
| 四元数 | `"quat"`（quatr/double） `"quatf"`（float） |
| float 族 | `"float"` `"float2"` `"float3"` `"float4"` |
| 颜色/透明 | `"color"` `"alpha"` |
| 布尔 | `"bool"` |
| 有符号整型 | `"int"` `"int2"` `"int3"` `"int4"` |
| 无符号整型 | `"uint"` `"uint2"` `"uint3"` `"uint4"` |

传入未知类型会抛出 Lua 错误 `createTrack: unknown value type '...'`。

## 附录 C：源码索引

| 文件 | 内容 |
|------|------|
| `source/core/script/FELuaVM.cpp` | VM 生命周期、fe 核心表、Node 绑定 |
| `source/core/script/FELuaMath.cpp` | vec2/vec3/vec4/quat/mat3/mat4 绑定 |
| `source/core/script/FELuaGeo.cpp` | FEMesh 绑定、fe.geo 几何工厂 |
| `source/core/script/FELuaAnim.cpp` | fe.anim 模块、动画对象绑定与常量 |
| `source/core/script/FELuaHelper.hpp` | 对象包装、table 转换等内部辅助 |
| `source/test/test.lua.geo/geo.lua` | 数学/几何绑定自测脚本 |
| `source/test/test.lua.anim/anim.lua` | 动画绑定自测脚本 |
