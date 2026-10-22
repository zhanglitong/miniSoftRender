// geo.js:test.js.geo 绑定自测脚本(与 geo.lua 逐行对齐)
// 1) 数学类型(vec2/vec3/vec4/quat/mat3/mat4)构造/运算/方法 assert
// 2) fe.geo 13 个参数化几何工厂创建 + FEMesh 查询日志
// 3) Node 几何挂接/变换/层级(mesh/setMesh/链式 setter/setGlobal*/reparent/root/...)
// 任何 assert 失败都会让 doFile 返回 false 并在引擎日志中输出错误
// 与 Lua 版差异:JS 无运算符重载 -> add/sub/mul/div/neg 方法;索引 0-based(at/col/children);
//               mat 列赋值用 setCol;grid color4 用 {r,g,b,a} 对象(optColor 不吃数组)

var checkCount = 0;
function check(cond, msg)
{
    if (!cond)
        throw new Error("assert failed: " + msg);
    checkCount = checkCount + 1;
}

// ==================== vec3 ====================
var v   =   fe.vec3(1, 2, 3);
check(v.x === 1 && v.y === 2 && v.z === 3, "vec3(x,y,z) ctor");
check(v.at(0) === 1 && v.at(1) === 2 && v.at(2) === 3, "vec3 0-based index");
var v0  =   fe.vec3();
check(v0.x === 0 && v0.y === 0 && v0.z === 0, "vec3() zero");
var vs  =   fe.vec3(5);
check(vs.x === 5 && vs.y === 5 && vs.z === 5, "vec3(n) fill");
var vt  =   fe.vec3({ x: 7, y: 8, z: 9 });
check(vt.x === 7 && vt.y === 8 && vt.z === 9, "vec3(object) ctor");

var a   =   fe.vec3(1, 0, 0);
var b   =   fe.vec3(0, 1, 0);
var ab  =   a.add(b);
check(ab.x === 1 && ab.y === 1, "vec3 add");
var amb =   a.sub(b);
check(amb.x === 1 && amb.y === -1, "vec3 sub");
check(a.mul(2).x === 2, "vec3 scalar mul");
check(a.div(2).x === 0.5, "vec3 scalar div");
check(a.neg().x === -1, "vec3 neg");
check(a.length() === 1, "vec3 length");
check(a.dot(b) === 0, "vec3 dot");
var cr  =   a.cross(b);
check(cr.x === 0 && cr.y === 0 && cr.z === 1, "vec3 cross");
var n   =   fe.vec3(0, 0, 5).normalize();
check(Math.abs(n.length() - 1) < 1e-9, "vec3 normalize");
check(fe.vec3(0, 0, 0).distance(fe.vec3(3, 4, 0)) === 5, "vec3 distance");
var l   =   fe.vec3(0, 0, 0).lerp(fe.vec3(10, 20, 30), 0.5);
check(l.x === 5 && l.y === 10 && l.z === 15, "vec3 lerp");

// ==================== vec2 / vec4 ====================
var v2  =   fe.vec2(3, 4);
check(v2.length() === 5, "vec2 length");
var v4  =   fe.vec4(1, 2, 3, 4);
check(v4.x === 1 && v4.w === 4 && v4.at(3) === 4, "vec4 ctor/index");

// ==================== quat ====================
var qi  =   fe.quat();
check(qi.w === 1, "quat() identity");
var q   =   fe.quat(0, 0, 0, 1);
check(q.x === 0 && q.w === 1, "quat(x,y,z,w) ctor");
var q90 =   fe.quatAngleAxis(Math.PI * 0.5, fe.vec3(0, 1, 0));
var r   =   q90.mul(fe.vec3(1, 0, 0));
check(Math.abs(r.x) < 1e-9 && Math.abs(r.z + 1) < 1e-9, "quat rotate vec3 (+90 deg around Y)");
check(Math.abs(q90.angle() - Math.PI * 0.5) < 1e-9, "quat angle");
var qm  =   q90.mul(q90);
check(Math.abs(qm.angle() - Math.PI) < 1e-9, "quat mul");

// ==================== mat3 / mat4 ====================
var mi  =   fe.mat3();
check(mi.determinant() === 1, "mat3 identity determinant");
var m4  =   fe.mat4();
var tp  =   m4.transformPoint(fe.vec3(1, 2, 3));
check(tp.x === 1 && tp.y === 2 && tp.z === 3, "mat4 identity transformPoint");
var mt  =   fe.mat4();
mt.setCol(3, fe.vec4(5, 6, 7, 1));
check(!mt.equals(m4), "mat4 not equal");
var tp2 =   mt.transformPoint(fe.vec3(0, 0, 0));
check(tp2.x === 5 && tp2.y === 6 && tp2.z === 7, "mat4 column set + transformPoint");
check(mt.mul(m4).transformPoint(fe.vec3(0, 0, 0)).x === 5, "mat4 mul");
var mp  =   fe.mat4Perspective(Math.PI / 3, 1280 / 720, 0.1, 100);
check(mp.c2.z !== 0, "mat4Perspective");
var ml  =   fe.mat4LookAt(fe.vec3(0, 0, 5), fe.vec3(0, 0, 0), fe.vec3(0, 1, 0));
var lo  =   ml.transformPoint(fe.vec3(0, 0, 0));
check(Math.abs(lo.z + 5) < 1e-9, "mat4LookAt");

fe.log("math self-test passed: " + checkCount + " checks");

// ==================== fe.geo 13 种几何 ====================
function logMesh(name, mesh)
{
    var box =   mesh.aabb();
    fe.log("geo " + name + " vertices=" + mesh.vertexCount() +
           " attrs=[" + mesh.attributes().join(",") + "]" +
           " aabb=(" + box.min.x.toFixed(2) + "," + box.min.y.toFixed(2) + "," + box.min.z.toFixed(2) +
           ")-(" + box.max.x.toFixed(2) + "," + box.max.y.toFixed(2) + "," + box.max.z.toFixed(2) + ")");
}

var defs    =
[
    { name: "box",       make: function () { return fe.geo.box({ size: fe.vec3(1, 1, 1), segs: { x: 2, y: 2, z: 2 } }); } },
    { name: "sphere",    make: function () { return fe.geo.sphere({ radius: 0.6 }); } },
    { name: "cylinder",  make: function () { return fe.geo.cylinder({ radiusTop: 0.5, radiusBottom: 0.5, height: 1.2 }); } },
    { name: "cone",      make: function () { return fe.geo.cone({ radius: 0.6, height: 1.2 }); } },
    { name: "pyramid",   make: function () { return fe.geo.pyramid({ radius: 0.8, height: 1.2 }); } },
    { name: "torus",     make: function () { return fe.geo.torus({ radius: 0.7, tube: 0.25 }); } },
    { name: "torusknot", make: function () { return fe.geo.torusknot({ radius: 0.55, tube: 0.18 }); } },
    { name: "circle",    make: function () { return fe.geo.circle({ radius: 0.7 }); } },
    { name: "ring",      make: function () { return fe.geo.ring({ innerRadius: 0.3, outerRadius: 0.8 }); } },
    { name: "plane",     make: function () { return fe.geo.plane({ width: 1.4, height: 1.4 }); } },
    { name: "grid",      make: function () { return fe.geo.grid({ size: 2, divs: 8, color4: { r: 180, g: 180, b: 180, a: 255 } }); } },
    { name: "ellipsoid", make: function () { return fe.geo.ellipsoid({ size: fe.vec3(1.2, 0.8, 1.2) }); } },
    { name: "tube",      make: function ()
        {
            return fe.geo.tube({
                points: [fe.vec3(-0.6, 0, 0), fe.vec3(0, 0.6, 0),
                         fe.vec3(0.6, 0, 0),  fe.vec3(0, -0.6, 0)],
                radius: 0.12, closed: true });
        } },
];

// 4x4 网格摆位(x/y 平面,正对默认相机)
var nodes   =   [];
var cols    =   4;
var spacing =   2.4;
for (var i = 0; i < defs.length; ++i)
{
    var mesh    =   defs[i].make();
    check(mesh.vertexCount() > 0, defs[i].name + " has vertices");
    logMesh(defs[i].name, mesh);
    var node    =   fe.createNode(defs[i].name);
    node.setMesh(mesh);
    var col     =   i % cols;
    var row     =   Math.floor(i / cols);
    node.setLocalTranslation(fe.vec3((col - (cols - 1) * 0.5) * spacing,
                                     ((cols - 1) * 0.5 - row) * spacing, 0));
    nodes.push(node);
}

// ==================== Node 层级/变换 ====================
var parent  =   fe.createNode("Hierarchy");
var c0      =   fe.createNode("Child0");
var c1      =   fe.createNode("Child1");
parent.addChild(c0);
parent.addChild(c1);
check(parent.children().length === 2, "children count");
check(c0.parent().name() === "Hierarchy", "parent backref");
check(c0.root().name() === "Hierarchy", "root");

// reparent(null):提升为顶层,保持世界变换
// setGlobal* 只写本地变换,先 update() 刷新缓存的全局变换,再 reparent
c1.setGlobalTranslation(fe.vec3(1, 2, 3));
c1.update();
c1.reparent(null);
check(c1.parent() === null, "reparent(null) -> toplevel");
var g   =   c1.globalTranslation();
fe.log("after reparent(null): global=(" + g.x + "," + g.y + "," + g.z + ") local=(" +
       c1.localTranslation().x + "," + c1.localTranslation().y + "," + c1.localTranslation().z + ")");
check(Math.abs(g.x - 1) < 1e-6 && Math.abs(g.y - 2) < 1e-6 && Math.abs(g.z - 3) < 1e-6,
      "reparent(null) keeps world transform");

// reparent(parent,0):插到首位
c1.reparent(parent, 0);
check(c1.parent().name() === "Hierarchy", "reparent under parent");
check(parent.children()[0].name() === "Child1", "reparent index 0 inserts first");

// removeChild
check(parent.removeChild(c0) === true, "removeChild");
check(parent.children().length === 1, "children after remove");
check(parent.removeChild(c0) === false, "removeChild twice -> false");

// 变换矩阵回读:globalTransform 平移列 == globalTranslation
var gt  =   parent.globalTransform();
var gp  =   gt.transformPoint(fe.vec3(0, 0, 0));
check(gp.x === 0 && gp.y === 0 && gp.z === 0, "globalTransform origin");
var lt  =   c1.localTransform();
check(lt.transformPoint(fe.vec3(0, 0, 0)).x === 1, "localTransform after reparent");

// 包围盒:几何节点 updateAabb 返回 {min,max}
var box =   nodes[0].updateAabb();
check(box.max.x > box.min.x, "updateAabb valid");
check(nodes[0].globalAabb().max.x > nodes[0].globalAabb().min.x, "globalAabb valid");

// 链式 setter
var chained =   fe.createNode("Chained");
chained.setName("Chained2").setLocalTranslation(fe.vec3(9, 9, 9)).setLocalScaling(fe.vec3(2, 2, 2));
check(chained.name() === "Chained2" && chained.localTranslation().x === 9, "chained setters");

fe.log("geo.js done: " + checkCount + " math checks, " + nodes.length + " geometry nodes, hierarchy OK");
