/// FEQuickJSGeo 实现:FEMesh 绑定 + fe.geo 参数化几何工厂
/// 工厂:mesh = fe.geo.box({size:[1,1,1], segs:[1,1,1]}) 等,参数对象全部可选
/// 默认输出属性流:POSITION + NORMAL + TEXCOORD0(grid 为 POSITION + COLOR0)

#include    "FEQuickJS.hpp"
#include    "FEQuickJSHelper.hpp"

#include    "../../inc/geometry/FEGeometryBox.hpp"
#include    "../../inc/geometry/FEGeometrySphere.hpp"
#include    "../../inc/geometry/FEGeometryCylinder.hpp"
#include    "../../inc/geometry/FEGeometryCone.hpp"
#include    "../../inc/geometry/FEGeometryPyramid.hpp"
#include    "../../inc/geometry/FEGeometryTorus.hpp"
#include    "../../inc/geometry/FEGeometryTorusKnot.hpp"
#include    "../../inc/geometry/FEGeometryCircle.hpp"
#include    "../../inc/geometry/FEGeometryRing.hpp"
#include    "../../inc/geometry/FEGeometryPlane.hpp"
#include    "../../inc/geometry/FEGeometryGrid.hpp"
#include    "../../inc/geometry/FEGeometryEllipsoidEGM.hpp"
#include    "../../inc/geometry/FEGeometryTube.hpp"

namespace   FE
{
    namespace
    {
        // ==================== 参数对象读取辅助(全部可选,缺省用默认值) ====================

        static  real    optNum(JSContext* ctx,JSValueConst opts,const char* k,real def)
        {
            if (!JS_IsObject(opts))
                return  def;
            JSValue v   =   JS_GetPropertyStr(ctx,opts,k);
            real    r   =   def;
            if (JS_IsNumber(v))
            {
                double  d;
                JS_ToFloat64(ctx,&d,v);
                r   =   (real)d;
            }
            JS_FreeValue(ctx,v);
            return  r;
        }

        static  int     optInt(JSContext* ctx,JSValueConst opts,const char* k,int def)
        {
            if (!JS_IsObject(opts))
                return  def;
            JSValue v   =   JS_GetPropertyStr(ctx,opts,k);
            int     r   =   def;
            if (JS_IsNumber(v))
            {
                int64_t i;
                JS_ToInt64(ctx,&i,v);
                r   =   (int)i;
            }
            JS_FreeValue(ctx,v);
            return  r;
        }

        static  bool    optBool(JSContext* ctx,JSValueConst opts,const char* k,bool def)
        {
            if (!JS_IsObject(opts))
                return  def;
            JSValue v   =   JS_GetPropertyStr(ctx,opts,k);
            bool    r   =   def;
            if (!JS_IsUndefined(v) && !JS_IsNull(v))
                r   =   JS_ToBool(ctx,v) ? true : false;
            JS_FreeValue(ctx,v);
            return  r;
        }

        static  float3  optFloat3(JSContext* ctx,JSValueConst opts,const char* k,const float3& def)
        {
            if (!JS_IsObject(opts))
                return  def;
            JSValue v   =   JS_GetPropertyStr(ctx,opts,k);
            float3  result  =   def;
            real3   t;
            if (toReal3(ctx,v,t))
                result  =   float3((float)t.x,(float)t.y,(float)t.z);
            JS_FreeValue(ctx,v);
            return  result;
        }

        static  uint3   optUint3(JSContext* ctx,JSValueConst opts,const char* k,const uint3& def)
        {
            if (!JS_IsObject(opts))
                return  def;
            JSValue v   =   JS_GetPropertyStr(ctx,opts,k);
            uint3   result  =   def;
            if (JS_IsObject(v))
            {
                for (int i = 0; i < 3; ++i)
                {
                    JSValue e   =   JS_GetPropertyUint32(ctx,v,(uint32_t)i);
                    if (JS_IsNumber(e))
                    {
                        int64_t n;
                        JS_ToInt64(ctx,&n,e);
                        result[i]   =   (uint)n;
                    }
                    JS_FreeValue(ctx,e);
                }
            }
            JS_FreeValue(ctx,v);
            return  result;
        }

        static  Rgba8   optColor(JSContext* ctx,JSValueConst opts,const char* k,const Rgba8& def)
        {
            if (!JS_IsObject(opts))
                return  def;
            JSValue v   =   JS_GetPropertyStr(ctx,opts,k);
            uint8x4 c;
            Rgba8   result  =   def;
            if (toUint8x4(ctx,v,c))
                result  =   Rgba8(c);
            JS_FreeValue(ctx,v);
            return  result;
        }

        /// 参数对象读 float3 数组(曲线控制点)
        static  float3s optFloat3s(JSContext* ctx,JSValueConst opts,const char* k)
        {
            float3s result;
            if (!JS_IsObject(opts))
                return  result;
            JSValue arr =   JS_GetPropertyStr(ctx,opts,k);
            if (JS_IsObject(arr))
            {
                JSValue len =   JS_GetPropertyStr(ctx,arr,"length");
                uint32_t    n   =   0;
                if (JS_IsNumber(len))
                {
                    int64_t v;
                    JS_ToInt64(ctx,&v,len);
                    n   =   (uint32_t)v;
                }
                JS_FreeValue(ctx,len);
                for (uint32_t i = 0; i < n; ++i)
                {
                    JSValue e   =   JS_GetPropertyUint32(ctx,arr,i);
                    real3   t;
                    if (toReal3(ctx,e,t))
                        result.emplace_back(float3((float)t.x,(float)t.y,(float)t.z));
                    JS_FreeValue(ctx,e);
                }
            }
            JS_FreeValue(ctx,arr);
            return  result;
        }

        static  Attrs   defaultStreams()
        {
            return  Attrs({IS_VERTEX_POS,IS_VERTEX_NOR,IS_VERTEX_TEXCOORD0});
        }

        // ==================== Mesh 绑定 ====================

        static  JSValue Mesh_vertexCount(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            auto    mesh    =   checkObj<FEMesh>(ctx,getEnv(ctx),this_val);
            if (mesh == nullptr)
                return  JS_EXCEPTION;
            auto*   buf     =   mesh->get(IS_VERTEX_POS);
            return  JS_NewInt32(ctx,buf != nullptr ? (int32_t)buf->count() : 0);
        }

        static  JSValue pushAabb(JSContext* ctx,JSEnv* env,float mnX,float mnY,float mnZ,float mxX,float mxY,float mxZ)
        {
            JSValue obj =   JS_NewObject(ctx);
            JS_SetPropertyStr(ctx,obj,"min",pushReal3(ctx,env,real3(mnX,mnY,mnZ)));
            JS_SetPropertyStr(ctx,obj,"max",pushReal3(ctx,env,real3(mxX,mxY,mxZ)));
            return  obj;
        }

        static  JSValue Mesh_aabb(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            auto    mesh    =   checkObj<FEMesh>(ctx,getEnv(ctx),this_val);
            if (mesh == nullptr)
                return  JS_EXCEPTION;
            const auto& box =   mesh->aabb();
            return  pushAabb(ctx,getEnv(ctx),box.getMinimum().x,box.getMinimum().y,box.getMinimum().z,
                             box.getMaximum().x,box.getMaximum().y,box.getMaximum().z);
        }

        static  JSValue Mesh_updateAabb(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            auto    mesh    =   checkObj<FEMesh>(ctx,getEnv(ctx),this_val);
            if (mesh == nullptr)
                return  JS_EXCEPTION;
            const auto& box =   mesh->updateAabb();
            return  pushAabb(ctx,getEnv(ctx),box.getMinimum().x,box.getMinimum().y,box.getMinimum().z,
                             box.getMaximum().x,box.getMaximum().y,box.getMaximum().z);
        }

        static  const char* slotName(uint32 slot)
        {
            if (slot == (uint32)IS_VERTEX_POS)          return  "POSITION";
            if (slot == (uint32)IS_VERTEX_NOR)          return  "NORMAL";
            if (slot == (uint32)IS_VERTEX_TEXCOORD0)    return  "TEXCOORD0";
            if (slot == (uint32)IS_VERTEX_COLOR0)       return  "COLOR0";
            return  nullptr;
        }

        static  JSValue Mesh_attributes(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            auto    mesh    =   checkObj<FEMesh>(ctx,getEnv(ctx),this_val);
            if (mesh == nullptr)
                return  JS_EXCEPTION;
            JSValue arr     =   JS_NewArray(ctx);
            uint32_t    n   =   0;
            char    buf[32];
            for (auto& attr : mesh->inputs())
            {
                uint32  slot    =   (uint32)attr.slot();
                const char* name    =   slotName(slot);
                if (name == nullptr)
                {
                    snprintf(buf,sizeof(buf),"SLOT_0x%x",slot);
                    name    =   buf;
                }
                JS_DefinePropertyValueUint32(ctx,arr,n++,JS_NewString(ctx,name),JS_PROP_C_W_E);
            }
            return  arr;
        }

        static  const JSFuncDef MeshMethods[]   =
        {
            {   "vertexCount",  Mesh_vertexCount,   0   },
            {   "aabb",         Mesh_aabb,          0   },
            {   "updateAabb",   Mesh_updateAabb,    0   },
            {   "attributes",   Mesh_attributes,    0   },
            {   nullptr,        nullptr,            0   },
        };

        // ==================== fe.geo 工厂 ====================

        static  JSValue geoBox(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryBox   geo(*env->ctx);
            geo.param()._size   =   optFloat3(ctx,argv[0],"size",float3(1,1,1));
            geo.param()._segs   =   optUint3(ctx,argv[0],"segs",uint3(1,1,1));
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoSphere(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometrySphere    geo(*env->ctx);
            geo.param().radius          =   (float)optNum(ctx,argv[0],"radius",1.0);
            geo.param().widthSegments   =   (uint)optInt(ctx,argv[0],"widthSegments",16);
            geo.param().heightSegments  =   (uint)optInt(ctx,argv[0],"heightSegments",12);
            geo.param().phiStart        =   (float)optNum(ctx,argv[0],"phiStart",0.0);
            geo.param().phiLength       =   (float)optNum(ctx,argv[0],"phiLength",float(TWO_PI));
            geo.param().thetaStart      =   (float)optNum(ctx,argv[0],"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(ctx,argv[0],"thetaLength",float(PI));
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoCylinder(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryCylinder  geo(*env->ctx);
            geo.param().radiusTop       =   (float)optNum(ctx,argv[0],"radiusTop",1.0);
            geo.param().radiusBottom    =   (float)optNum(ctx,argv[0],"radiusBottom",1.0);
            geo.param().height          =   (float)optNum(ctx,argv[0],"height",1.0);
            geo.param().radialSegments  =   (uint)optInt(ctx,argv[0],"radialSegments",16);
            geo.param().heightSegments  =   (uint)optInt(ctx,argv[0],"heightSegments",1);
            geo.param().thetaStart      =   (float)optNum(ctx,argv[0],"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(ctx,argv[0],"thetaLength",float(TWO_PI));
            geo.param().openEnded       =   optBool(ctx,argv[0],"openEnded",false);
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoCone(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryCone  geo(*env->ctx);
            geo.param().radiusBottom    =   (float)optNum(ctx,argv[0],"radius",optNum(ctx,argv[0],"radiusBottom",1.0));
            geo.param().height          =   (float)optNum(ctx,argv[0],"height",1.0);
            geo.param().radialSegments  =   (uint)optInt(ctx,argv[0],"radialSegments",32);
            geo.param().heightSegments  =   (uint)optInt(ctx,argv[0],"heightSegments",1);
            geo.param().openEnded       =   optBool(ctx,argv[0],"openEnded",false);
            geo.param().radiusTop       =   0.0f;
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoPyramid(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryPyramid   geo(*env->ctx);
            geo.param().radiusBottom    =   (float)optNum(ctx,argv[0],"radius",optNum(ctx,argv[0],"radiusBottom",1.0));
            geo.param().height          =   (float)optNum(ctx,argv[0],"height",1.0);
            geo.param().radialSegments  =   (uint)optInt(ctx,argv[0],"radialSegments",4);
            geo.param().openEnded       =   optBool(ctx,argv[0],"openEnded",false);
            geo.param().radiusTop       =   0.0f;
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoTorus(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryTorus geo(*env->ctx);
            geo.param().radius          =   (float)optNum(ctx,argv[0],"radius",1.0);
            geo.param().tube            =   (float)optNum(ctx,argv[0],"tube",0.4);
            geo.param().radialSegments  =   (uint)optInt(ctx,argv[0],"radialSegments",20);
            geo.param().tubularSegments =   (uint)optInt(ctx,argv[0],"tubularSegments",64);
            geo.param().arc             =   (float)optNum(ctx,argv[0],"arc",float(TWO_PI));
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoTorusKnot(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryTorusKnot geo(*env->ctx);
            geo.param().radius          =   (float)optNum(ctx,argv[0],"radius",1.0);
            geo.param().tube            =   (float)optNum(ctx,argv[0],"tube",0.4);
            geo.param().tubularSegments =   (uint)optInt(ctx,argv[0],"tubularSegments",64);
            geo.param().radialSegments  =   (uint)optInt(ctx,argv[0],"radialSegments",16);
            geo.param().p               =   (uint)optInt(ctx,argv[0],"p",2);
            geo.param().q               =   (uint)optInt(ctx,argv[0],"q",3);
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoCircle(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryCircle    geo(*env->ctx);
            geo.param().radius          =   (float)optNum(ctx,argv[0],"radius",1.0);
            geo.param().thetaStart      =   (float)optNum(ctx,argv[0],"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(ctx,argv[0],"thetaLength",float(TWO_PI));
            geo.param().segments        =   (uint)optInt(ctx,argv[0],"segments",16);
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoRing(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryRing  geo(*env->ctx);
            geo.param().innerRadius     =   (float)optNum(ctx,argv[0],"innerRadius",0.5);
            geo.param().outerRadius     =   (float)optNum(ctx,argv[0],"outerRadius",1.0);
            geo.param().thetaStart      =   (float)optNum(ctx,argv[0],"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(ctx,argv[0],"thetaLength",float(TWO_PI));
            geo.param().thetaSegments   =   (uint)optInt(ctx,argv[0],"thetaSegments",16);
            geo.param().phiSegments     =   (uint)optInt(ctx,argv[0],"phiSegments",1);
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoPlane(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryPlane geo(*env->ctx);
            geo.param()._width  =   (float)optNum(ctx,argv[0],"width",1.0);
            geo.param()._height =   (float)optNum(ctx,argv[0],"height",1.0);
            geo.param()._wSegs  =   (uint)optInt(ctx,argv[0],"wSegs",1);
            geo.param()._hSegs  =   (uint)optInt(ctx,argv[0],"hSegs",1);
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  JSValue geoGrid(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryGrid  geo(*env->ctx);
            Rgba8   lineColor   =   Rgba8(uint8x4(180,180,180,255));
            geo.param()._size   =   (uint32_t)optInt(ctx,argv[0],"size",10);
            geo.param()._divs   =   (uint32_t)optInt(ctx,argv[0],"divs",10);
            geo.param()._color1 =   optColor(ctx,argv[0],"color1",Rgba8(uint8x4(255,0,0,255)));
            geo.param()._color2 =   optColor(ctx,argv[0],"color2",Rgba8(uint8x4(0,255,0,255)));
            geo.param()._color3 =   optColor(ctx,argv[0],"color3",Rgba8(uint8x4(0,0,255,255)));
            geo.param()._color4 =   optColor(ctx,argv[0],"color4",lineColor);
            geo.param()._dir    =   optInt(ctx,argv[0],"dir",0);
            Attrs   streams =   Attrs({IS_VERTEX_POS,IS_VERTEX_COLOR0});
            return  pushObj<FEMesh>(ctx,env,geo.triangular(streams));
        }

        static  JSValue geoEllipsoid(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryEllipsoidEGM  geo(*env->ctx);
            geo.param()._size   =   optFloat3(ctx,argv[0],"size",float3(1,1,1));
            geo.param()._segs   =   optUint3(ctx,argv[0],"segs",uint3(10,10,10));
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        static  bool    parseCurveType(const char* s,FECurveType& out)
        {
            if (_stricmp(s,"line") == 0)            {   out =   FECurveType::Line;              return  true;   }
            if (_stricmp(s,"quadratic") == 0)       {   out =   FECurveType::QuadraticBezier;   return  true;   }
            if (_stricmp(s,"cubic") == 0)           {   out =   FECurveType::CubicBezier;       return  true;   }
            if (_stricmp(s,"catmullrom") == 0)      {   out =   FECurveType::CatmullRom;        return  true;   }
            return  false;
        }

        static  bool    parseCatmullRomType(const char* s,FECatmullRomType& out)
        {
            if (_stricmp(s,"centripetal") == 0)     {   out =   FECatmullRomType::Centripetal;  return  true;   }
            if (_stricmp(s,"chordal") == 0)         {   out =   FECatmullRomType::Chordal;      return  true;   }
            if (_stricmp(s,"uniform") == 0)         {   out =   FECatmullRomType::Uniform;      return  true;   }
            return  false;
        }

        /// fe.geo.tube({type:"catmullrom", points:[[0,0,0],[1,1,0],...], radius:0.1, ...})
        static  JSValue geoTube(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            auto*   env =   getEnv(ctx);
            FEGeometryTube  geo(*env->ctx);
            geo.param().type             =   FECurveType::CatmullRom;
            if (JS_IsObject(argv[0]))
            {
                JSValue v   =   JS_GetPropertyStr(ctx,argv[0],"type");
                if (JS_IsString(v))
                {
                    const char* s   =   JS_ToCString(ctx,v);
                    if (s != nullptr)
                        parseCurveType(s,geo.param().type);
                    JS_FreeCString(ctx,s);
                }
                JS_FreeValue(ctx,v);
                v   =   JS_GetPropertyStr(ctx,argv[0],"catmullType");
                if (JS_IsString(v))
                {
                    const char* s   =   JS_ToCString(ctx,v);
                    if (s != nullptr)
                        parseCatmullRomType(s,geo.param().catmullType);
                    JS_FreeCString(ctx,s);
                }
                JS_FreeValue(ctx,v);
            }
            geo.param().points           =   optFloat3s(ctx,argv[0],"points");
            geo.param().radius           =   (float)optNum(ctx,argv[0],"radius",0.1);
            geo.param().tubularSegments  =   (uint)optInt(ctx,argv[0],"tubularSegments",64);
            geo.param().radialSegments   =   (uint)optInt(ctx,argv[0],"radialSegments",8);
            geo.param().closed           =   optBool(ctx,argv[0],"closed",false);
            geo.param().tension          =   (float)optNum(ctx,argv[0],"tension",0.5);
            return  pushObj<FEMesh>(ctx,env,geo.triangular(defaultStreams()));
        }

        /// 向对象写入一个工厂函数
        static  void    setFn(JSContext* ctx,JSValue obj,const char* name,JSCFunction* fn,int length)
        {
            JS_SetPropertyStr(ctx,obj,name,JS_NewCFunction(ctx,fn,name,length));
        }
    }

    /// <summary>
    /// 注册 FEMesh 类型与 fe.geo 工厂表
    /// </summary>
    void    FEQJSRegisterGeo(JSContext* ctx,JSEnv* env,JSValue fe)
    {
        registerClass<FEMesh>(ctx,env,MeshMethods);
        JSValue geo =   JS_NewObject(ctx);
        setFn(ctx,geo,"box",geoBox,1);
        setFn(ctx,geo,"sphere",geoSphere,1);
        setFn(ctx,geo,"cylinder",geoCylinder,1);
        setFn(ctx,geo,"cone",geoCone,1);
        setFn(ctx,geo,"pyramid",geoPyramid,1);
        setFn(ctx,geo,"torus",geoTorus,1);
        setFn(ctx,geo,"torusknot",geoTorusKnot,1);
        setFn(ctx,geo,"circle",geoCircle,1);
        setFn(ctx,geo,"ring",geoRing,1);
        setFn(ctx,geo,"plane",geoPlane,1);
        setFn(ctx,geo,"grid",geoGrid,1);
        setFn(ctx,geo,"ellipsoid",geoEllipsoid,1);
        setFn(ctx,geo,"tube",geoTube,1);
        JS_SetPropertyStr(ctx,fe,"geo",geo);
    }
}
