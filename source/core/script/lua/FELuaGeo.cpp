/// FELuaGeo 实现:FEMesh 绑定 + fe.geo 参数化几何工厂
/// 工厂:mesh = fe.geo.box{size={1,1,1}, segs={1,1,1}} 等,参数表全部可选
/// 默认输出属性流:POSITION + NORMAL + TEXCOORD0(grid 为 POSITION + COLOR0)

#include    "FELuaVM.hpp"
#include    "FELuaHelper.hpp"

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
        // ==================== 参数表读取辅助(全部可选,缺省用默认值) ====================

        static  real    optNum(lua_State* L,const char* k,real def)
        {
            if (!lua_istable(L,1))
                return  def;
            lua_getfield(L,1,k);
            real    v   =   lua_isnumber(L,-1) ? (real)lua_tonumber(L,-1) : def;
            lua_pop(L,1);
            return  v;
        }

        static  int     optInt(lua_State* L,const char* k,int def)
        {
            if (!lua_istable(L,1))
                return  def;
            lua_getfield(L,1,k);
            int     v   =   lua_isnumber(L,-1) ? (int)lua_tointeger(L,-1) : def;
            lua_pop(L,1);
            return  v;
        }

        static  bool    optBool(lua_State* L,const char* k,bool def)
        {
            if (!lua_istable(L,1))
                return  def;
            lua_getfield(L,1,k);
            if (lua_isnoneornil(L,-1))
            {
                lua_pop(L,1);
                return  def;
            }
            bool    v   =   lua_toboolean(L,-1) ? true : false;
            lua_pop(L,1);
            return  v;
        }

        static  float3  optFloat3(lua_State* L,const char* k,const float3& def)
        {
            if (!lua_istable(L,1))
                return  def;
            lua_getfield(L,1,k);
            real3   v;
            float3  result  =   def;
            if (toReal3(L,-1,v))
                result  =   float3((float)v.x,(float)v.y,(float)v.z);
            lua_pop(L,1);
            return  result;
        }

        static  uint3   optUint3(lua_State* L,const char* k,const uint3& def)
        {
            if (!lua_istable(L,1))
                return  def;
            lua_getfield(L,1,k);
            uint3   result  =   def;
            if (lua_istable(L,-1))
            {
                for (int i = 0; i < 3; ++i)
                {
                    lua_geti(L,-1,i + 1);
                    if (lua_isnumber(L,-1))
                        result[i]   =   (uint)lua_tointeger(L,-1);
                    lua_pop(L,1);
                }
            }
            lua_pop(L,1);
            return  result;
        }

        static  Rgba8   optColor(lua_State* L,const char* k,const Rgba8& def)
        {
            if (!lua_istable(L,1))
                return  def;
            lua_getfield(L,1,k);
            uint8x4 c;
            Rgba8   result  =   def;
            if (toUint8x4(L,-1,c))
                result  =   Rgba8(c);
            lua_pop(L,1);
            return  result;
        }

        /// 参数表读 float3 数组(曲线控制点)
        static  float3s optFloat3s(lua_State* L,const char* k)
        {
            float3s result;
            if (!lua_istable(L,1))
                return  result;
            lua_getfield(L,1,k);
            if (lua_istable(L,-1))
            {
                size_t  n   =   lua_rawlen(L,-1);
                for (size_t i = 1; i <= n; ++i)
                {
                    lua_geti(L,-1,(lua_Integer)i);
                    real3   v;
                    if (toReal3(L,-1,v))
                        result.emplace_back(float3((float)v.x,(float)v.y,(float)v.z));
                    lua_pop(L,1);
                }
            }
            lua_pop(L,1);
            return  result;
        }

        static  Attrs   defaultStreams()
        {
            return  Attrs({IS_VERTEX_POS,IS_VERTEX_NOR,IS_VERTEX_TEXCOORD0});
        }

        // ==================== Mesh 绑定 ====================

        static  int     Mesh_vertexCount(lua_State* L)
        {
            auto    mesh    =   checkObj<FEMesh>(L,1);
            auto*   buf     =   mesh->get(IS_VERTEX_POS);
            lua_pushinteger(L,buf != nullptr ? (lua_Integer)buf->count() : 0);
            return  1;
        }

        static  void    pushAabb(lua_State* L,float mnX,float mnY,float mnZ,float mxX,float mxY,float mxZ)
        {
            lua_createtable(L,0,2);
            pushReal3(L,real3(mnX,mnY,mnZ));    lua_setfield(L,-2,"min");
            pushReal3(L,real3(mxX,mxY,mxZ));    lua_setfield(L,-2,"max");
        }

        static  int     Mesh_aabb(lua_State* L)
        {
            auto    mesh    =   checkObj<FEMesh>(L,1);
            const auto& box =   mesh->aabb();
            pushAabb(L,box.getMinimum().x,box.getMinimum().y,box.getMinimum().z,
                        box.getMaximum().x,box.getMaximum().y,box.getMaximum().z);
            return  1;
        }

        static  int     Mesh_updateAabb(lua_State* L)
        {
            auto    mesh    =   checkObj<FEMesh>(L,1);
            const auto& box =   mesh->updateAabb();
            pushAabb(L,box.getMinimum().x,box.getMinimum().y,box.getMinimum().z,
                        box.getMaximum().x,box.getMaximum().y,box.getMaximum().z);
            return  1;
        }

        static  const char* slotName(uint32 slot)
        {
            if (slot == (uint32)IS_VERTEX_POS)          return  "POSITION";
            if (slot == (uint32)IS_VERTEX_NOR)          return  "NORMAL";
            if (slot == (uint32)IS_VERTEX_TEXCOORD0)    return  "TEXCOORD0";
            if (slot == (uint32)IS_VERTEX_COLOR0)       return  "COLOR0";
            return  nullptr;
        }

        static  int     Mesh_attributes(lua_State* L)
        {
            auto    mesh    =   checkObj<FEMesh>(L,1);
            auto    inputs  =   mesh->inputs();
            lua_newtable(L);
            int     n   =   0;
            char    buf[32];
            for (auto& attr : inputs)
            {
                uint32  slot    =   (uint32)attr.slot();
                const char* name    =   slotName(slot);
                if (name == nullptr)
                {
                    snprintf(buf,sizeof(buf),"SLOT_0x%x",slot);
                    name    =   buf;
                }
                lua_pushstring(L,name);
                lua_seti(L,-2,++n);
            }
            return  1;
        }

        static  const luaL_Reg  MeshMethods[]   =
        {
            {   "vertexCount",  Mesh_vertexCount    },
            {   "aabb",         Mesh_aabb           },
            {   "updateAabb",   Mesh_updateAabb     },
            {   "attributes",   Mesh_attributes     },
            {   nullptr,        nullptr             },
        };

        // ==================== fe.geo 工厂 ====================

        static  int     geoBox(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryBox   geo(*env->ctx);
            geo.param()._size   =   optFloat3(L,"size",float3(1,1,1));
            geo.param()._segs   =   optUint3(L,"segs",uint3(1,1,1));
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoSphere(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometrySphere    geo(*env->ctx);
            geo.param().radius          =   (float)optNum(L,"radius",1.0);
            geo.param().widthSegments   =   (uint)optInt(L,"widthSegments",16);
            geo.param().heightSegments  =   (uint)optInt(L,"heightSegments",12);
            geo.param().phiStart        =   (float)optNum(L,"phiStart",0.0);
            geo.param().phiLength       =   (float)optNum(L,"phiLength",float(TWO_PI));
            geo.param().thetaStart      =   (float)optNum(L,"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(L,"thetaLength",float(PI));
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoCylinder(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryCylinder  geo(*env->ctx);
            geo.param().radiusTop       =   (float)optNum(L,"radiusTop",1.0);
            geo.param().radiusBottom    =   (float)optNum(L,"radiusBottom",1.0);
            geo.param().height          =   (float)optNum(L,"height",1.0);
            geo.param().radialSegments  =   (uint)optInt(L,"radialSegments",16);
            geo.param().heightSegments  =   (uint)optInt(L,"heightSegments",1);
            geo.param().thetaStart      =   (float)optNum(L,"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(L,"thetaLength",float(TWO_PI));
            geo.param().openEnded       =   optBool(L,"openEnded",false);
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoCone(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryCone  geo(*env->ctx);
            geo.param().radiusBottom    =   (float)optNum(L,"radius",optNum(L,"radiusBottom",1.0));
            geo.param().height          =   (float)optNum(L,"height",1.0);
            geo.param().radialSegments  =   (uint)optInt(L,"radialSegments",32);
            geo.param().heightSegments  =   (uint)optInt(L,"heightSegments",1);
            geo.param().openEnded       =   optBool(L,"openEnded",false);
            geo.param().radiusTop       =   0.0f;
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoPyramid(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryPyramid   geo(*env->ctx);
            geo.param().radiusBottom    =   (float)optNum(L,"radius",optNum(L,"radiusBottom",1.0));
            geo.param().height          =   (float)optNum(L,"height",1.0);
            geo.param().radialSegments  =   (uint)optInt(L,"radialSegments",4);
            geo.param().openEnded       =   optBool(L,"openEnded",false);
            geo.param().radiusTop       =   0.0f;
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoTorus(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryTorus geo(*env->ctx);
            geo.param().radius          =   (float)optNum(L,"radius",1.0);
            geo.param().tube            =   (float)optNum(L,"tube",0.4);
            geo.param().radialSegments  =   (uint)optInt(L,"radialSegments",20);
            geo.param().tubularSegments =   (uint)optInt(L,"tubularSegments",64);
            geo.param().arc             =   (float)optNum(L,"arc",float(TWO_PI));
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoTorusKnot(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryTorusKnot    geo(*env->ctx);
            geo.param().radius          =   (float)optNum(L,"radius",1.0);
            geo.param().tube            =   (float)optNum(L,"tube",0.4);
            geo.param().tubularSegments =   (uint)optInt(L,"tubularSegments",64);
            geo.param().radialSegments  =   (uint)optInt(L,"radialSegments",16);
            geo.param().p               =   (uint)optInt(L,"p",2);
            geo.param().q               =   (uint)optInt(L,"q",3);
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoCircle(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryCircle    geo(*env->ctx);
            geo.param().radius          =   (float)optNum(L,"radius",1.0);
            geo.param().thetaStart      =   (float)optNum(L,"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(L,"thetaLength",float(TWO_PI));
            geo.param().segments        =   (uint)optInt(L,"segments",16);
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoRing(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryRing   geo(*env->ctx);
            geo.param().innerRadius     =   (float)optNum(L,"innerRadius",0.5);
            geo.param().outerRadius     =   (float)optNum(L,"outerRadius",1.0);
            geo.param().thetaStart      =   (float)optNum(L,"thetaStart",0.0);
            geo.param().thetaLength     =   (float)optNum(L,"thetaLength",float(TWO_PI));
            geo.param().thetaSegments   =   (uint)optInt(L,"thetaSegments",16);
            geo.param().phiSegments     =   (uint)optInt(L,"phiSegments",1);
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoPlane(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryPlane geo(*env->ctx);
            geo.param()._width  =   (float)optNum(L,"width",1.0);
            geo.param()._height =   (float)optNum(L,"height",1.0);
            geo.param()._wSegs  =   (uint)optInt(L,"wSegs",1);
            geo.param()._hSegs  =   (uint)optInt(L,"hSegs",1);
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  int     geoGrid(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryGrid  geo(*env->ctx);
            Rgba8   lineColor   =   Rgba8(uint8x4(180,180,180,255));
            geo.param()._size   =   (uint32_t)optInt(L,"size",10);
            geo.param()._divs   =   (uint32_t)optInt(L,"divs",10);
            geo.param()._color1 =   optColor(L,"color1",Rgba8(uint8x4(255,0,0,255)));
            geo.param()._color2 =   optColor(L,"color2",Rgba8(uint8x4(0,255,0,255)));
            geo.param()._color3 =   optColor(L,"color3",Rgba8(uint8x4(0,0,255,255)));
            geo.param()._color4 =   optColor(L,"color4",lineColor);
            geo.param()._dir    =   optInt(L,"dir",0);
            Attrs   streams =   Attrs({IS_VERTEX_POS,IS_VERTEX_COLOR0});
            pushObj<FEMesh>(L,geo.triangular(streams));
            return  1;
        }

        static  int     geoEllipsoid(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryEllipsoidEGM  geo(*env->ctx);
            geo.param()._size   =   optFloat3(L,"size",float3(1,1,1));
            geo.param()._segs   =   optUint3(L,"segs",uint3(10,10,10));
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  bool    parseCurveType(const char* s,FECurveType& out)
        {
            if (_stricmp(s,"line") == 0)                {   out =   FECurveType::Line;              return  true;   }
            if (_stricmp(s,"quadratic") == 0)           {   out =   FECurveType::QuadraticBezier;   return  true;   }
            if (_stricmp(s,"cubic") == 0)               {   out =   FECurveType::CubicBezier;       return  true;   }
            if (_stricmp(s,"catmullrom") == 0)          {   out =   FECurveType::CatmullRom;        return  true;   }
            return  false;
        }

        static  bool    parseCatmullRomType(const char* s,FECatmullRomType& out)
        {
            if (_stricmp(s,"centripetal") == 0)         {   out =   FECatmullRomType::Centripetal;  return  true;   }
            if (_stricmp(s,"chordal") == 0)             {   out =   FECatmullRomType::Chordal;      return  true;   }
            if (_stricmp(s,"uniform") == 0)             {   out =   FECatmullRomType::Uniform;      return  true;   }
            return  false;
        }

        /// fe.geo.tube{type="catmullrom", points={{0,0,0},{1,1,0},...}, radius=0.1, ...}
        static  int     geoTube(lua_State* L)
        {
            auto*   env =   getEnv(L);
            FEGeometryTube  geo(*env->ctx);
            geo.param().type             =   FECurveType::CatmullRom;
            if (lua_istable(L,1))
            {
                lua_getfield(L,1,"type");
                if (lua_isstring(L,-1))
                    parseCurveType(lua_tostring(L,-1),geo.param().type);
                lua_pop(L,1);
                lua_getfield(L,1,"catmullType");
                if (lua_isstring(L,-1))
                    parseCatmullRomType(lua_tostring(L,-1),geo.param().catmullType);
                lua_pop(L,1);
            }
            geo.param().points           =   optFloat3s(L,"points");
            geo.param().radius           =   (float)optNum(L,"radius",0.1);
            geo.param().tubularSegments  =   (uint)optInt(L,"tubularSegments",64);
            geo.param().radialSegments   =   (uint)optInt(L,"radialSegments",8);
            geo.param().closed           =   optBool(L,"closed",false);
            geo.param().tension          =   (float)optNum(L,"tension",0.5);
            pushObj<FEMesh>(L,geo.triangular(defaultStreams()));
            return  1;
        }

        static  const luaL_Reg  GeoFuncs[]   =
        {
            {   "box",          geoBox          },
            {   "sphere",       geoSphere       },
            {   "cylinder",     geoCylinder     },
            {   "cone",         geoCone         },
            {   "pyramid",      geoPyramid      },
            {   "torus",        geoTorus        },
            {   "torusknot",    geoTorusKnot    },
            {   "circle",       geoCircle       },
            {   "ring",         geoRing         },
            {   "plane",        geoPlane        },
            {   "grid",         geoGrid         },
            {   "ellipsoid",    geoEllipsoid    },
            {   "tube",         geoTube         },
            {   nullptr,        nullptr         },
        };
    }

    /// 注册 FEMesh 类型与 fe.geo 工厂表(要求 fe 表在栈顶)
    void    FELuaRegisterGeo(lua_State* L)
    {
        registerType<FEMesh>(L,MeshMethods);
        lua_newtable(L);
        luaL_setfuncs(L,GeoFuncs,0);
        lua_setfield(L,-2,"geo");
    }
}
