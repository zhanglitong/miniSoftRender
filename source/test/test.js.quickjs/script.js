// test.js.quickjs driver script (executed via doFile, ASCII only:
// quickjs rejects non-UTF-8 bytes).
// Full-coverage assertions over fe bindings: math(vec/quat/mat) /
// geo(13 factories + Mesh) / node(scene glue + transforms + hierarchy) /
// anim(clip/track/KFValue shapes/node keyframes/action error path).
// Final verdict: globalThis.__t === true when every check passed;
// failing messages are joined into globalThis.__fails ("" when none).

var __fails = [];
function T(cond, msg) { if (!cond) __fails.push(msg); }
function TT(fn, msg) { var threw = false; try { fn(); } catch (e) { threw = true; } if (!threw) __fails.push(msg + " did not throw"); }
function near(a, b, eps) { return Math.abs(a - b) <= (eps === undefined ? 1e-6 : eps); }
function isObj(v) { return v !== null && typeof v === 'object'; }

// ---------- 1. vec2/vec3/vec4 ----------
fe.log("script: section 1 vec begin");
(function () {
    var v0 = fe.vec3();
    T(near(v0.x, 0) && near(v0.y, 0) && near(v0.z, 0), "vec3() default");
    var v1 = fe.vec3(5);
    T(near(v1.x, 5) && near(v1.y, 5) && near(v1.z, 5), "vec3(scalar)");
    var v2 = fe.vec3(1, 2, 3);
    T(near(v2.x, 1) && near(v2.y, 2) && near(v2.z, 3), "vec3(x,y,z)");
    T(near(fe.vec3({ x: 1, y: 2, z: 3 }).z, 3), "vec3({x,y,z})");
    T(near(fe.vec3([1, 2, 3]).y, 2), "vec3([x,y,z])");
    T(near(fe.vec2({ x: 1, y: 2 }).y, 2), "vec2({x,y})");
    T(near(fe.vec2(3).x, 3), "vec2(scalar)");
    T(near(fe.vec4({ x: 1, y: 2, z: 3 }).w, 0), "vec4({x,y,z}) pads w=0");
    T(near(fe.vec4(1, 2, 3, 4).w, 4), "vec4(x,y,z,w)");

    v2.x = 9;
    T(near(v2.at(0), 9), "vec3.x write + at(0)");
    T(v2.at(9) === undefined, "vec3.at(out of range) undefined");

    var a = fe.vec3(1, 2, 3);
    T(a.add(fe.vec3(1, 1, 1)).equals(fe.vec3(2, 3, 4)), "vec3.add");
    T(a.sub(fe.vec3(1, 1, 1)).equals(fe.vec3(0, 1, 2)), "vec3.sub");
    T(a.mul(fe.vec3(2, 2, 2)).equals(fe.vec3(2, 4, 6)), "vec3.mul(vec)");
    T(a.mul(2).equals(fe.vec3(2, 4, 6)), "vec3.mul(scalar)");
    T(a.div(fe.vec3(2, 2, 2)).equals(fe.vec3(0.5, 1, 1.5)), "vec3.div(vec)");
    T(a.div(2).equals(fe.vec3(0.5, 1, 1.5)), "vec3.div(scalar)");
    T(a.neg().equals(fe.vec3(-1, -2, -3)), "vec3.neg");
    T(!a.equals(fe.vec3(1, 2, 4)), "vec3.equals false");
    T(a.clone() !== a && a.clone().equals(a), "vec3.clone");
    T(near(fe.vec3(1, 2, 2).length(), 3), "vec3.length");
    T(near(a.dot(fe.vec3(2, 0, 0)), 2), "vec3.dot");
    T(near(fe.vec3(3, 0, 0).distance(fe.vec3(0, 0, 0)), 3), "vec3.distance");
    T(near(fe.vec3(0, 3, 0).normalize().y, 1), "vec3.normalize");
    T(a.lerp(fe.vec3(3, 3, 3), 0.5).equals(fe.vec3(2, 2.5, 3)), "vec3.lerp");
    T(fe.vec3(1, 0, 0).cross(fe.vec3(0, 1, 0)).equals(fe.vec3(0, 0, 1)), "vec3.cross");
    T(a.toString().indexOf("vec3") === 0, "vec3.toString");
    T(fe.vec2(1, 2).toString().indexOf("vec2") === 0, "vec2.toString");
    T(fe.vec4(1, 2, 3, 4).toString().indexOf("vec4") === 0, "vec4.toString");
})();

// ---------- 2. quat ----------
fe.log("script: section 2 quat begin");
(function () {
    var id = fe.quat();
    T(near(id.w, 1) && near(id.x, 0) && near(id.y, 0) && near(id.z, 0), "quat() identity");
    var q = fe.quat(1, 2, 3, 4);
    T(near(q.x, 1) && near(q.y, 2) && near(q.z, 3) && near(q.w, 4), "quat(x,y,z,w)");
    T(fe.quat({ x: 1, y: 2, z: 3, w: 4 }).w === 4, "quat({x,y,z,w})");
    T(q.at(3) === 4 && q.at(9) === undefined, "quat.at");

    var qy = fe.quatAngleAxis(Math.PI, { x: 0, y: 1, z: 0 });
    T(near(qy.w, 0, 1e-9) && near(qy.y, 1, 1e-9), "quatAngleAxis 180deg Y");
    var rot = qy.mul(fe.vec3(1, 0, 0));
    T(near(rot.x, -1, 1e-9) && near(rot.y, 0, 1e-9) && near(rot.z, 0, 1e-9), "quat.mul(vec3) rotates");
    T(near(qy.mul(2).y, 2), "quat.mul(scalar)");
    T(id.mul(qy).equals(qy), "quat.mul(quat) identity left");
    T(qy.add(qy).y === 2, "quat.add");
    T(qy.sub(qy).y === 0, "quat.sub");
    T(qy.neg().y === -1, "quat.neg");
    T(qy.equals(qy.clone()), "quat.equals/clone");
    T(!qy.equals({ x: 0, y: 1, z: 0, w: 0 }), "quat.equals plain object is false");
    var cq = fe.quat(1, 2, 3, 4).conj();
    T(near(cq.x, -1) && near(cq.y, -2) && near(cq.z, -3) && near(cq.w, 4), "quat.conj");
    var nq = fe.quat(1, 1, 0, 0).normalize();
    T(near(nq.dot(nq), 1), "quat.normalize + dot");
    T(id.dot(id) === 1, "quat.dot identity");
    var mid = fe.quatAngleAxis(Math.PI / 2, { x: 0, y: 1, z: 0 }).slerp(id, 0.5);
    T(near(mid.angle(), Math.PI / 4, 1e-6), "quat.slerp half angle");
    T(near(qy.angle(), Math.PI, 1e-9), "quat.angle");
    T(near(qy.axis().y, 1, 1e-9), "quat.axis");
    var m4 = qy.toMat4();
    T(isObj(m4.c0) && near(m4.c0.x, -1, 1e-9), "quat.toMat4 rotates X");
    T(isObj(qy.toMat3().c0), "quat.toMat3");
    T(qy.toString().indexOf("quat") === 0, "quat.toString");
})();

// ---------- 3. mat3/mat4 ----------
fe.log("script: section 3 mat begin");
(function () {
    var m = fe.mat4();
    T(near(m.c0.x, 1) && near(m.c3.x, 0) && near(m.c3.w, 1), "mat4() identity");
    T(m.determinant() === 1, "mat4.det identity");
    T(m.equals(m.clone()), "mat4.clone/equals");
    T(m.transpose().equals(m), "mat4.transpose identity");
    T(m.inverse().equals(m), "mat4.inverse identity");

    var m2 = fe.mat4();
    m2.setCol(3, fe.vec4(1, 2, 3, 1));
    T(m2.c3.x === 1 && m2.c3.z === 3, "mat4.setCol in place");
    T(m2.col(3).x === 1, "mat4.col");
    var p = m2.transformPoint(fe.vec3(1, 1, 1));
    T(near(p.x, 2) && near(p.y, 3) && near(p.z, 4), "mat4.transformPoint");
    var d = m2.transformVector(fe.vec3(1, 1, 1));
    T(near(d.x, 1) && near(d.y, 1) && near(d.z, 1), "mat4.transformVector");
    T(m.mul(m2).equals(m2), "mat4.mul identity left");

    var s = fe.mat4();
    s.setCol(0, fe.vec4(2, 0, 0, 0));
    T(s.determinant() === 2, "mat4.det scale2");
    T(near(s.mul(m2).transformPoint(fe.vec3(1, 1, 1)).x, 4), "mat4.mul chain");

    T(fe.mat4(fe.quat()).equals(fe.mat4()), "mat4(quat identity)");
    T(fe.mat3().determinant() === 1, "mat3() identity det");
    T(fe.mat3(fe.quat()).equals(fe.mat3()), "mat3(quat identity)");
    T(fe.mat3().equals(fe.mat3().clone()), "mat3 clone/equals");
    T(fe.mat3().toString().indexOf("mat3") === 0, "mat3.toString");

    var pr = fe.mat4Perspective(Math.PI / 2, 1, 1, 3);
    T(near(pr.c1.y, 1, 1e-6), "perspective f=1");
    T(near(pr.c2.z, -1.5, 1e-6), "perspective zz=-1.5 (ZO)");
    T(near(pr.c3.z, -1.5, 1e-6), "perspective tz=-1.5 (ZO)");
    T(near(pr.c2.w, -1, 1e-6), "perspective wz=-1");

    var lk = fe.mat4LookAt(fe.vec3(0, 0, 5), fe.vec3(0, 0, 0), fe.vec3(0, 1, 0));
    T(near(lk.c3.z, -5, 1e-6), "lookAt translation z=-5");
})();

// ---------- 4. geo factories + Mesh ----------
fe.log("script: section 4 geo begin");
(function () {
    var makers = ["box", "sphere", "cylinder", "cone", "pyramid", "torus", "torusknot",
        "circle", "ring", "plane", "grid", "ellipsoid", "tube"];
    for (var i = 0; i < makers.length; ++i) {
        var name = makers[i];
        // tube needs explicit points (same as Lua: no default fallback)
        var mesh = (name === "tube")
            ? fe.geo.tube({ points: [{ x: 0, y: 0, z: 0 }, { x: 1, y: 0, z: 0 }, { x: 1, y: 1, z: 0 }] })
            : fe.geo[name]();
        T(mesh !== null && typeof mesh.vertexCount === "function", "geo." + name + " returns mesh");
        T(mesh !== null && mesh.vertexCount() > 0, "geo." + name + " vertexCount > 0");
        if (mesh !== null) {
            // factories do not precompute aabb; updateAabb() first (same as Lua test)
            var b = mesh.updateAabb();
            T(isObj(b) && isObj(b.min) && isObj(b.max) && b.max.x >= b.min.x, "geo." + name + " aabb");
        }
    }
    var sp = fe.geo.sphere({ radius: 2, widthSegments: 12, heightSegments: 8 });
    sp.updateAabb();
    T(near(sp.aabb().max.x, 2, 1e-3) && near(sp.aabb().min.x, -2, 1e-3), "geo.sphere radius aabb");
    var bx = fe.geo.box({ size: { x: 1, y: 2, z: 3 }, segs: { x: 2, y: 2, z: 2 } });
    bx.updateAabb();
    var bb = bx.aabb();
    T(near((bb.max.y - bb.min.y) / (bb.max.z - bb.min.z), 2 / 3, 1e-3), "geo.box size ratio y/z");
    T(Array.isArray(bx.attributes()) && bx.attributes().length > 0, "mesh.attributes");
    T(isObj(bx.updateAabb()), "mesh.updateAabb");
    var tube = fe.geo.tube({ radius: 0.2, closed: false, points: [{ x: 0, y: 0, z: 0 }, { x: 1, y: 0, z: 0 }, { x: 1, y: 1, z: 0 }] });
    T(tube !== null && tube.vertexCount() > 0, "geo.tube with points");
})();

// ---------- 5. node / scene ----------
fe.log("script: section 5 node begin");
(function () {
    var n = fe.createNode("js-node");
    T(n !== null && n.name() === "js-node", "createNode named");
    T(fe.createNode() !== null, "createNode anonymous");
    T(fe.findNode("js-node") !== null, "findNode top level");
    T(fe.findNode("js-no-such-node") === null, "findNode missing null");

    T(n.setLocalTranslation({ x: 1, y: 2, z: 3 }) === n, "setLocalTranslation returns self");
    T(near(n.localTranslation().x, 1) && near(n.localTranslation().z, 3), "localTranslation get");
    n.setLocalTranslation([4, 5, 6]);
    T(near(n.localTranslation().z, 6), "setLocalTranslation array form");
    n.setLocalScaling({ x: 2, y: 2, z: 2 });
    T(near(n.localScaling().y, 2), "setLocalScaling");
    n.setLocalRotation({ x: 0, y: 0, z: 0, w: 1 });
    T(near(n.localRotation().w, 1), "setLocalRotation {x,y,z,w}");
    n.setLocalRotation(fe.quatAngleAxis(Math.PI / 2, { x: 0, y: 1, z: 0 }));
    T(near(n.localRotation().w, Math.SQRT1_2, 1e-6), "setLocalRotation quat object");
    n.setGlobalTranslation({ x: 0, y: 0, z: 0 });
    T(near(n.globalTranslation().x, 0), "setGlobalTranslation/globalTranslation");
    n.setGlobalRotation({ x: 0, y: 0, z: 0, w: 1 });
    n.setGlobalScaling({ x: 1, y: 1, z: 1 });
    T(near(n.globalScaling().y, 1), "setGlobalScaling/globalScaling");
    // top-level node: after setGlobal*, local mirrors global -> compare against live value
    T(near(n.localTransform().c3.x, n.localTranslation().x, 1e-6), "localTransform translation column");
    T(isObj(n.globalTransform().c0) && isObj(n.globalRSTransform().c0), "global transforms are mat4");
    fe.log("s5: transform ok");

    n.update();
    fe.log("s5: update() ok");
    n.update(0.5);
    fe.log("s5: update(0.5) ok");
    n.fireChanged();
    fe.log("s5: fireChanged ok");
    T(n.makeDirty() === n, "makeDirty returns self");
    fe.log("s5: makeDirty ok");

    var ab = n.updateAabb();
    fe.log("s5: updateAabb ok");
    T(isObj(ab) && isObj(ab.min) && isObj(ab.max), "node.updateAabb object");
    T(isObj(n.globalAabb().min), "node.globalAabb");
    fe.log("s5: update/aabb ok");

    T(n.setColor({ r: 10, g: 20, b: 30, a: 255 }) === n, "setColor returns self");
    T(n.color().r === 10 && n.color().a === 255, "color get");
    T(n.visible() === true, "node visible default true");
    n.visible(false);
    T(n.visible() === false, "node.visible(false)");
    n.visible(true);
    fe.log("s5: color/visible ok");

    var c = fe.createNode("js-child");
    n.addChild(c);
    T(c.parent() !== null && c.parent().name() === "js-node", "addChild/parent");
    var kids = n.children();
    T(Array.isArray(kids) && kids.length === 1 && kids[0].name() === "js-child", "children list");
    T(c.root().name() === "js-node", "root of child");
    T(n.root().name() === "js-node", "root of self");
    c.reparent(null);
    T(c.parent() === null, "reparent(null) toplevel");
    c.reparent(n, 0);
    T(c.parent().name() === "js-node", "reparent(n,0)");
    T(n.removeChild(c) === true, "removeChild");
    T(n.removeChild(c) === false, "removeChild twice false");
    n.addChild(c);
    T(fe.findNode("js-child") !== null, "findNode DFS child");
    fe.log("s5: hierarchy ok");

    var mesh = fe.geo.box();
    T(n.setMesh(mesh) === n, "setMesh returns self");
    T(n.mesh() !== null && n.mesh().vertexCount() === mesh.vertexCount(), "mesh get");
    fe.log("s5: mesh ok");

    var anim = fe.anim.createNodeAnimation();
    T(n.addComponent(anim) === true, "addComponent animation");
    T(n.removeComponent(anim) === true, "removeComponent animation");
    T(n.removeComponent(anim) === false, "removeComponent twice false");
    fe.log("s5: component ok");
})();

T(typeof fe.dispatchNode(fe.findNode("js-node")) === "undefined", "dispatchNode plain node");
fe.log("s5: dispatchNode ok");

// ---------- 6. anim ----------
fe.log("script: section 6 anim begin");
(function () {
    T(typeof fe.anim.createAnimation === "function", "fe.anim module ready");
    T(typeof fe.anim.NP_QUAT === "number" && typeof fe.anim.PROP_TRANSFORM_XYZ === "number", "anim constants");
    T(fe.anim.IT_LINEAR !== fe.anim.IT_BEZIER && typeof fe.anim.PT_LOOP === "number" &&
        typeof fe.anim.TM_INVERT === "number", "anim enums");

    var clip = fe.anim.createClip();
    T(clip !== null && clip.trackCount() === 0, "createClip empty");

    var t1 = fe.anim.createTrack(fe.anim.PROP_TRANSFORM_X, "real");
    T(t1 !== null && t1.keyCount() === 0, "createTrack real");
    T(t1.addKeyFrame(0, 1.0) === true, "track addKeyFrame #1");
    T(t1.addKeyFrame(1, 2.0) === true, "track addKeyFrame #2");
    T(t1.keyCount() === 2, "track keyCount");
    var ts = t1.times();
    T(Array.isArray(ts) && ts.length === 2 && ts[0] === 0 && ts[1] === 1, "track times array");
    var vs = t1.values();
    T(Array.isArray(vs) && near(vs[0], 1) && near(vs[1], 2), "track values scalars");
    var rg = t1.range();
    T(isObj(rg) && near(rg.min, 0) && near(rg.max, 1), "track range");
    T(t1.updateKeyFrame(0, 5.0) === true, "track updateKeyFrame by index");
    T(near(t1.values()[0], 5), "track updateKeyFrame effect");
    T(t1.removeKeyFrame(1) === true, "track removeKeyFrame by index");
    T(t1.keyCount() === 1, "track keyCount after remove");
    T(t1.addKeyFrame(0.5, 3.0) === true, "track addKeyFrame non-integer time");
    T(t1.keyCount() === 2, "track keyCount after time add");
    T(t1.removeKeyFrame(0.5) === true, "track removeKeyFrame by time");
    T(t1.keyCount() === 1, "track keyCount after time remove");
    t1.setInterpolation(fe.anim.IT_BEZIER);
    T(t1.interpolation() === fe.anim.IT_BEZIER, "track interpolation");
    t1.setTension(0.5);
    T(near(t1.tension(), 0.5), "track tension");
    t1.setName("trk");
    T(t1.name() === "trk", "track name");
    T(t1.isValid() === true, "track isValid");
    t1.setPropertyIndex(fe.anim.PROP_TRANSFORM_Y);
    T(t1.propertyIndex() === fe.anim.PROP_TRANSFORM_Y, "track propertyIndex");

    T(clip.addTrack(t1) === 1, "clip.addTrack returns count");
    T(clip.trackCount() === 1, "clip.trackCount");
    T(clip.getTrack(0).name() === "trk", "clip.getTrack");
    TT(function () { clip.getTrack(9); }, "clip.getTrack out of range");
    T(isObj(clip.range()), "clip range object");
    clip.simplifyTimeline();
    // Lua contract: toKFValue accepts {x,y,z} as real3, no throw;
    // the only track is "real" -> engine addKeyFrame returns false on type mismatch
    T(clip.addKeyFrame(0.25, { x: 1, y: 2, z: 3 }) === false, "clip.addKeyFrame type mismatch returns false");

    // KFValue shapes
    var tc = fe.anim.createTrack(fe.anim.PROP_COLOR_RGB, "color");
    T(tc.addKeyFrame(0, { r: 255, g: 128, b: 64, a: 255 }) === true, "color KFValue");
    T(tc.values()[0].r === 255 && tc.values()[0].g === 128, "color KFValue read");

    var tq = fe.anim.createTrack(fe.anim.PROP_QUAT, "quat");
    T(tq.addKeyFrame(0, fe.quat(1, 0, 0, 0)) === true, "quat object KFValue");
    T(tq.values()[0].equals(fe.quat(1, 0, 0, 0)) === true, "quat KFValue has quat proto");

    var tv = fe.anim.createTrack(fe.anim.PROP_TRANSFORM_XYZ, "real3");
    T(tv.addKeyFrame(0, { x: 1, y: 2, z: 3 }) === true, "real3 KFValue object");
    T(tv.addKeyFrame(1, fe.vec3(4, 5, 6)) === true, "real3 KFValue vec3");
    T(near(tv.values()[0].x, 1) && near(tv.values()[1].z, 6), "real3 KFValue read");

    var tw = fe.anim.createTrack(fe.anim.PROP_SCALE_XYZ, "real4");
    T(tw.addKeyFrame(0, { x: 1, y: 2, z: 3, w: 4 }) === true, "real4 KFValue");
    T(near(tw.values()[0].w, 4), "real4 KFValue read");

    var t2d = fe.anim.createTrack(fe.anim.PROP_SCALE_X, "real2");
    T(t2d.addKeyFrame(0, { x: 1, y: 2 }) === true, "real2 KFValue");
    T(near(t2d.values()[0].y, 2), "real2 KFValue read");

    var tb = fe.anim.createTrack(fe.anim.PROP_VISIBLE, "bool");
    T(tb.addKeyFrame(0, true) === true, "bool KFValue");

    TT(function () { fe.anim.createTrack(0, "no-such-type"); }, "createTrack unknown type throws");

    // Animation object
    var a = fe.anim.createAnimation();
    T(a !== null, "createAnimation");
    a.setName("A");
    T(a.name() === "A", "anim name");
    a.setOffset(0.5);
    T(near(a.offset(), 0.5), "anim offset");
    a.setClip(clip);
    T(a.clip() !== null && a.clip().trackCount() === 1, "anim setClip/clip");
    T(isObj(a.range()), "anim range object");
    T(typeof a.isValid() === "boolean", "anim isValid");
    a.clearChanged();
    T(typeof a.isChanged() === "boolean", "anim isChanged/clearChanged");

    // node keyframe helpers need anim with clip + tracks
    var na = fe.anim.createNodeAnimation();
    T(na !== null, "createNodeAnimation default bits");
    var nc = fe.anim.createClip();
    var nt = fe.anim.createTrack(fe.anim.PROP_TRANSFORM_XYZ, "real3");
    nt.addKeyFrame(0, { x: 0, y: 0, z: 0 });
    nc.addTrack(nt);
    na.setClip(nc);
    var tn = fe.findNode("js-node");
    // track nt is pre-seeded at time 0 -> add must use a fresh time (0 would return false)
    T(fe.anim.addKeyFrameFromNode(na, 1, tn) === true, "addKeyFrameFromNode");
    T(fe.anim.updateKeyFrameFromNode(na, 1, tn) === true, "updateKeyFrameFromNode");
    T(fe.anim.removeKeyFrameFromNode(na, 1) === true, "removeKeyFrameFromNode");
    T(fe.anim.createNodeAnimation(fe.anim.NP_Default) !== null, "createNodeAnimation explicit bits");

    // no window -> animation system not registered -> action must throw
    var actionThrew = false;
    try { fe.anim.action("default"); } catch (e) { actionThrew = true; }
    T(actionThrew, "anim.action throws when system not ready");
})();

// final verdict
var __failCount = __fails.length;
globalThis.__t = __failCount === 0;
globalThis.__fails = __fails.join(" | ");
globalThis.__marker = 42;
fe.log("script.js done, fails =", __failCount);
