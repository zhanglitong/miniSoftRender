-- geo.lua:test.lua.geo 绑定自测脚本
-- 1) 数学类型(vec2/vec3/vec4/quat/mat3/mat4)构造/运算/方法 assert
-- 2) fe.geo 13 个参数化几何工厂创建 + FEMesh 查询日志
-- 3) Node 几何挂接/变换/层级(mesh/setMesh/链式 setter/setGlobal*/reparent/root/...)
-- 任何 assert 失败都会让 doFile 返回 false 并在引擎日志中输出错误
-- 说明:脚本节点不直接挂材质(材质绑定不在本次范围),渲染分发与材质兜底
--      由 C++ 侧在脚本执行完成后统一进行

local checkCount    =   0
local  function check(cond, msg)
    if not cond then
        error("assert failed: " .. msg)
    end
    checkCount  =   checkCount + 1
end

-- ==================== vec3 ====================
local v     =   fe.vec3(1, 2, 3)
check(v.x == 1 and v.y == 2 and v.z == 3, "vec3(x,y,z) ctor")
check(v[1] == 1 and v[2] == 2 and v[3] == 3, "vec3 1-based index")
local v0    =   fe.vec3()
check(v0.x == 0 and v0.y == 0 and v0.z == 0, "vec3() zero")
local vs    =   fe.vec3(5)
check(vs.x == 5 and vs.y == 5 and vs.z == 5, "vec3(n) fill")
local vt    =   fe.vec3({x = 7, y = 8, z = 9})
check(vt.x == 7 and vt.y == 8 and vt.z == 9, "vec3(table) ctor")

local a     =   fe.vec3(1, 0, 0)
local b     =   fe.vec3(0, 1, 0)
check((a + b).x == 1 and (a + b).y == 1, "vec3 add")
check((a - b).x == 1 and (a - b).y == -1, "vec3 sub")
check((a * 2).x == 2 and (2 * a).x == 2, "vec3 scalar mul")
check((a / 2).x == 0.5, "vec3 scalar div")
check((-a).x == -1, "vec3 unm")
check(a:length() == 1, "vec3 length")
check(a:dot(b) == 0, "vec3 dot")
local cr    =   a:cross(b)
check(cr.x == 0 and cr.y == 0 and cr.z == 1, "vec3 cross")
local n     =   fe.vec3(0, 0, 5):normalize()
check(math.abs(n:length() - 1) < 1e-9, "vec3 normalize")
check(fe.vec3(0, 0, 0):distance(fe.vec3(3, 4, 0)) == 5, "vec3 distance")
local l     =   fe.vec3(0, 0, 0):lerp(fe.vec3(10, 20, 30), 0.5)
check(l.x == 5 and l.y == 10 and l.z == 15, "vec3 lerp")

-- ==================== vec2 / vec4 ====================
local v2    =   fe.vec2(3, 4)
check(v2:length() == 5, "vec2 length")
local v4    =   fe.vec4(1, 2, 3, 4)
check(v4.x == 1 and v4.w == 4 and v4[4] == 4, "vec4 ctor/index")

-- ==================== quat ====================
local qi    =   fe.quat()
check(qi.w == 1, "quat() identity")
local q     =   fe.quat(0, 0, 0, 1)
check(q.x == 0 and q.w == 1, "quat(x,y,z,w) ctor")
local q90   =   fe.quatAngleAxis(math.pi * 0.5, fe.vec3(0, 1, 0))
local r     =   q90 * fe.vec3(1, 0, 0)
check(math.abs(r.x) < 1e-9 and math.abs(r.z + 1) < 1e-9, "quat rotate vec3 (+90 deg around Y)")
check(math.abs(q90:angle() - math.pi * 0.5) < 1e-9, "quat angle")
local qm    =   q90 * q90
check(math.abs(qm:angle() - math.pi) < 1e-9, "quat mul")

-- ==================== mat3 / mat4 ====================
local mi    =   fe.mat3()
check(mi:determinant() == 1, "mat3 identity determinant")
local m4    =   fe.mat4()
local tp    =   m4:transformPoint(fe.vec3(1, 2, 3))
check(tp.x == 1 and tp.y == 2 and tp.z == 3, "mat4 identity transformPoint")
local mt    =   fe.mat4()
mt[4]       =   fe.vec4(5, 6, 7, 1)
check(mt ~= m4, "mat4 not equal")
local tp2   =   mt:transformPoint(fe.vec3(0, 0, 0))
check(tp2.x == 5 and tp2.y == 6 and tp2.z == 7, "mat4 column set + transformPoint")
check((mt * m4):transformPoint(fe.vec3(0, 0, 0)).x == 5, "mat4 mul")
local mp    =   fe.mat4Perspective(math.rad(60), 1280 / 720, 0.1, 100)
check(mp[3][3] ~= 0, "mat4Perspective")
local ml    =   fe.mat4LookAt(fe.vec3(0, 0, 5), fe.vec3(0, 0, 0), fe.vec3(0, 1, 0))
local lo    =   ml:transformPoint(fe.vec3(0, 0, 0))
check(math.abs(lo.z + 5) < 1e-9, "mat4LookAt")

fe.log(string.format("math self-test passed: %d checks", checkCount))

-- ==================== fe.geo 13 种几何 ====================
local function logMesh(name, mesh)
    local box   =   mesh:aabb()
    fe.log(string.format(
        "geo %-10s vertices=%-5d attrs=[%s] aabb=(%.2f,%.2f,%.2f)-(%.2f,%.2f,%.2f)",
        name, mesh:vertexCount(), table.concat(mesh:attributes(), ","),
        box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z))
end

local defs  =
{
    { "box",        function() return fe.geo.box{ size = fe.vec3(1, 1, 1), segs = { 2, 2, 2 } } end },
    { "sphere",     function() return fe.geo.sphere{ radius = 0.6 } end },
    { "cylinder",   function() return fe.geo.cylinder{ radiusTop = 0.5, radiusBottom = 0.5, height = 1.2 } end },
    { "cone",       function() return fe.geo.cone{ radius = 0.6, height = 1.2 } end },
    { "pyramid",    function() return fe.geo.pyramid{ radius = 0.8, height = 1.2 } end },
    { "torus",      function() return fe.geo.torus{ radius = 0.7, tube = 0.25 } end },
    { "torusknot",  function() return fe.geo.torusknot{ radius = 0.55, tube = 0.18 } end },
    { "circle",     function() return fe.geo.circle{ radius = 0.7 } end },
    { "ring",       function() return fe.geo.ring{ innerRadius = 0.3, outerRadius = 0.8 } end },
    { "plane",      function() return fe.geo.plane{ width = 1.4, height = 1.4 } end },
    { "grid",       function() return fe.geo.grid{ size = 2, divs = 8, color4 = { 180, 180, 180, 255 } } end },
    { "ellipsoid",  function() return fe.geo.ellipsoid{ size = fe.vec3(1.2, 0.8, 1.2) } end },
    { "tube",       function() return fe.geo.tube
                    {
                        points  =   { fe.vec3(-0.6, 0, 0), fe.vec3(0, 0.6, 0),
                                      fe.vec3(0.6, 0, 0),  fe.vec3(0, -0.6, 0) },
                        radius  =   0.12, closed = true,
                    } end },
}

-- 4x4 网格摆位(x/y 平面,正对默认相机)
local nodes =   {}
local cols      =   4
local spacing   =   2.4
for i, def in ipairs(defs) do
    local mesh  =   def[2]()
    check(mesh:vertexCount() > 0, def[1] .. " has vertices")
    logMesh(def[1], mesh)
    local node  =   fe.createNode(def[1])
    node:setMesh(mesh)
    local col   =   (i - 1) % cols
    local row   =   math.floor((i - 1) / cols)
    node:setLocalTranslation(fe.vec3((col - (cols - 1) * 0.5) * spacing,
                                     ((cols - 1) * 0.5 - row) * spacing, 0))
    nodes[#nodes + 1]   =   node
end

-- ==================== Node 层级/变换 ====================
local parent    =   fe.createNode("Hierarchy")
local c0        =   fe.createNode("Child0")
local c1        =   fe.createNode("Child1")
parent:addChild(c0)
parent:addChild(c1)
check(#parent:children() == 2, "children count")
check(c0:parent():name() == "Hierarchy", "parent backref")
check(c0:root():name() == "Hierarchy", "root")

-- reparent(nil):提升为顶层,保持世界变换
-- setGlobal* 只写本地变换,先 update() 刷新缓存的全局变换,再 reparent
c1:setGlobalTranslation(fe.vec3(1, 2, 3))
c1:update()
c1:reparent(nil)
check(c1:parent() == nil, "reparent(nil) -> toplevel")
local g     =   c1:globalTranslation()
fe.log(string.format("after reparent(nil): global=(%g,%g,%g) local=(%g,%g,%g)",
                     g.x, g.y, g.z,
                     c1:localTranslation().x, c1:localTranslation().y, c1:localTranslation().z))
check(math.abs(g.x - 1) < 1e-6 and math.abs(g.y - 2) < 1e-6 and math.abs(g.z - 3) < 1e-6,
      "reparent(nil) keeps world transform")

-- reparent(parent,0):插到首位
c1:reparent(parent, 0)
check(c1:parent():name() == "Hierarchy", "reparent under parent")
check(parent:children()[1]:name() == "Child1", "reparent index 0 inserts first")

-- removeChild
check(parent:removeChild(c0) == true, "removeChild")
check(#parent:children() == 1, "children after remove")
check(parent:removeChild(c0) == false, "removeChild twice -> false")

-- 变换矩阵回读:globalTransform 平移列 == globalTranslation
local gt    =   parent:globalTransform()
local gp    =   gt:transformPoint(fe.vec3(0, 0, 0))
check(gp.x == 0 and gp.y == 0 and gp.z == 0, "globalTransform origin")
local lt    =   c1:localTransform()
check(lt:transformPoint(fe.vec3(0, 0, 0)).x == 1, "localTransform after reparent")

-- 包围盒:几何节点 updateAabb 返回 {min,max}
local box   =   nodes[1]:updateAabb()
check(box.max.x > box.min.x, "updateAabb valid")
check(nodes[1]:globalAabb().max.x > nodes[1]:globalAabb().min.x, "globalAabb valid")

-- 链式 setter
local chained    =   fe.createNode("Chained")
chained:setName("Chained2"):setLocalTranslation(fe.vec3(9, 9, 9)):setLocalScaling(fe.vec3(2, 2, 2))
check(chained:name() == "Chained2" and chained:localTranslation().x == 9, "chained setters")

fe.log(string.format("geo.lua done: %d math checks, %d geometry nodes, hierarchy OK",
                     checkCount, #nodes))
