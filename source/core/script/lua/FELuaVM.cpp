/// FELuaVM 实现:VM 生命周期 + fe 核心表(场景胶水/日志) + Node 绑定
/// 动画/数学/几何绑定分别在 FELuaAnim.cpp/FELuaMath.cpp/FELuaGeo.cpp 中实现,
/// 由 init() 调用各注册入口

#include    "FELuaVM.hpp"
#include    "FELuaHelper.hpp"

#include    <stdio.h>
#include    <string.h>

namespace   FE
{
    /// 动画模块注册入口(FELuaAnim.cpp)
    void    FELuaRegisterAnim(lua_State* L);
    /// 数学模块注册入口(FELuaMath.cpp)
    void    FELuaRegisterMath(lua_State* L);
    /// 几何模块注册入口(FELuaGeo.cpp)
    void    FELuaRegisterGeo(lua_State* L);

    /// aabb3dr 压栈为 {min=vec3,max=vec3}
    static  void    pushAabb3dr(lua_State* L,const aabb3dr& box)
    {
        lua_createtable(L,0,2);
        pushReal3(L,real3(box.getMinimum().x,box.getMinimum().y,box.getMinimum().z));
        lua_setfield(L,-2,"min");
        pushReal3(L,real3(box.getMaximum().x,box.getMaximum().y,box.getMaximum().z));
        lua_setfield(L,-2,"max");
    }

    // ==================== Node 绑定 ====================

    static  int Node_name(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        lua_pushstring(L,node->name());
        return  1;
    }

    /// setter 统一返回 self,支持链式调用
    static  int Node_setName(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        node->setName(luaL_checkstring(L,2));
        lua_settop(L,1);
        return  1;
    }

    static  int Node_localTranslation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushReal3(L,node->localTranslation());
        return  1;
    }

    static  int Node_setLocalTranslation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        real3   pos;
        if (!toReal3(L,2,pos))
            return  luaL_error(L,"setLocalTranslation: expect vec3/table {x,y,z} or {1,2,3}");
        node->setLocalTranslation(pos);
        lua_settop(L,1);
        return  1;
    }

    static  int Node_localScaling(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushReal3(L,node->localScaling());
        return  1;
    }

    static  int Node_setLocalScaling(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        real3   scale;
        if (!toReal3(L,2,scale))
            return  luaL_error(L,"setLocalScaling: expect vec3/table {x,y,z} or {1,2,3}");
        node->setLocalScaling(scale);
        lua_settop(L,1);
        return  1;
    }

    static  int Node_localRotation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushQuatr(L,node->localRotation());
        return  1;
    }

    static  int Node_setLocalRotation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        quatr   rot;
        if (!toQuatr(L,2,rot))
            return  luaL_error(L,"setLocalRotation: expect quat/table {x,y,z,w}");
        node->setLocalRotation(rot);
        lua_settop(L,1);
        return  1;
    }

    static  int Node_globalTranslation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushReal3(L,node->globalTranslation());
        return  1;
    }

    static  int Node_color(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushUint8x4(L,node->color().value());
        return  1;
    }

    static  int Node_setColor(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        uint8x4 rgba;
        if (!toUint8x4(L,2,rgba))
            return  luaL_error(L,"setColor: expect table {r,g,b,a} (0-255)");
        node->setColor(Rgba8(rgba));
        lua_settop(L,1);
        return  1;
    }

    /// <summary>
    /// 可见性:visible() 查询;visible(true/false) 设置
    /// </summary>
    static  int Node_visible(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        if (lua_gettop(L) >= 2)
        {
            if (lua_toboolean(L,2))
                node->flags().addFlag(FLAG_VISIBLE);
            else
                node->flags().removeFlag(FLAG_VISIBLE);
            return  0;
        }
        lua_pushboolean(L,node->flags().hasFlag(FLAG_VISIBLE) ? 1 : 0);
        return  1;
    }

    static  int Node_addChild(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto    child   =   checkObj<FENode>(L,2);
        node->addChild(child);
        return  0;
    }

    static  int Node_parent(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto    parent  =   node->parent();
        if (parent == nullptr)
        {
            lua_pushnil(L);
            return  1;
        }
        pushObj<FENode>(L,parent);
        return  1;
    }

    /// <summary>
    /// 添加组件(当前绑定类型中仅 Animation 是组件),成功返回 true
    /// </summary>
    static  int Node_addComponent(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto*   p       =   (LuaPtr<FEAnimation>*)luaL_testudata(L,2,metaName<FEAnimation>());
        if (p == nullptr)
            return  luaL_typeerror(L,2,"FE.Animation");
        lua_pushboolean(L,node->addComponent(p->obj.get()) ? 1 : 0);
        return  1;
    }

    static  int Node_removeComponent(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto*   p       =   (LuaPtr<FEAnimation>*)luaL_testudata(L,2,metaName<FEAnimation>());
        if (p == nullptr)
            return  luaL_typeerror(L,2,"FE.Animation");
        lua_pushboolean(L,node->removeComponent(p->obj.get()) ? 1 : 0);
        return  1;
    }

    // ==================== Node 几何/变换/层级扩展 ====================

    static  int Node_mesh(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushObj<FEMesh>(L,node->mesh());
        return  1;
    }

    static  int Node_setMesh(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto    mesh    =   checkObj<FEMesh>(L,2);
        node->setMesh(mesh.get());
        lua_settop(L,1);
        return  1;
    }

    /// node:update([tm]):刷新节点(tm 为动画时间,缺省 0)
    static  int Node_update(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        node->update(luaL_optnumber(L,2,0.0));
        return  0;
    }

    static  int Node_fireChanged(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        node->fireChanged();
        return  0;
    }

    static  int Node_makeDirty(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        node->makeDirty();
        lua_settop(L,1);
        return  1;
    }

    /// node:updateAabb([recursion]):计算并返回 {min=vec3,max=vec3},recursion 缺省 true
    static  int Node_updateAabb(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        bool    recursion   =   lua_isnoneornil(L,2) ? true : (lua_toboolean(L,2) != 0);
        pushAabb3dr(L,node->updateAabb(recursion));
        return  1;
    }

    static  int Node_globalAabb(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushAabb3dr(L,node->globalAabb());
        return  1;
    }

    static  int Node_localTransform(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushMat4(L,node->localTransform());
        return  1;
    }

    static  int Node_globalTransform(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushMat4(L,node->globalTransform());
        return  1;
    }

    static  int Node_globalRSTransform(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushMat4(L,node->globalRSTransform());
        return  1;
    }

    static  int Node_globalRotation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushQuatr(L,node->globalRotation());
        return  1;
    }

    static  int Node_globalScaling(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushReal3(L,node->globalScaling());
        return  1;
    }

    static  int Node_setGlobalTranslation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        real3   pos;
        if (!toReal3(L,2,pos))
            return  luaL_error(L,"setGlobalTranslation: expect vec3/table {x,y,z} or {1,2,3}");
        node->setGlobalTranslation(pos);
        lua_settop(L,1);
        return  1;
    }

    static  int Node_setGlobalRotation(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        quatr   rot;
        if (!toQuatr(L,2,rot))
            return  luaL_error(L,"setGlobalRotation: expect quat/table {x,y,z,w}");
        node->setGlobalRotation(rot);
        lua_settop(L,1);
        return  1;
    }

    static  int Node_setGlobalScaling(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        real3   scale;
        if (!toReal3(L,2,scale))
            return  luaL_error(L,"setGlobalScaling: expect vec3/table {x,y,z} or {1,2,3}");
        node->setGlobalScaling(scale);
        lua_settop(L,1);
        return  1;
    }

    /// <summary>
    /// node:reparent([parent][,index]):重新挂接父子关系并保持世界变换不变
    /// parent 为 nil 时提升为顶层;index 为新父节点孩子列表的 0-based 插入位置,缺省(SIZE_MAX)追加末尾
    /// </summary>
    static  int Node_reparent(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        FENode* target  =   nullptr;
        if (!lua_isnoneornil(L,2))
        {
            auto    parent  =   checkObj<FENode>(L,2);
            target  =   parent.get();
        }
        size_t  index   =   SIZE_MAX;
        if (lua_gettop(L) >= 3 && !lua_isnil(L,3))
        {
            lua_Integer idx =   luaL_checkinteger(L,3);
            if (idx < 0)
                return  luaL_error(L,"reparent: index must be >= 0");
            index   =   (size_t)idx;
        }
        /// 层级变更统一走节点树:它会同步顶层集合(reparent 只改父子关系)
        auto    scene   =   node->ctx().scene();
        if (scene == nullptr)
            return  luaL_error(L,"reparent: no active scene");
        if (!scene->nodeTree().setParent(node,target,index))
            return  luaL_error(L,"reparent: cannot attach to self or descendant");
        return  0;
    }

    static  int Node_root(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        pushObj<FENode>(L,SharedPtr<FENode>(node->root()));
        return  1;
    }

    static  int Node_children(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto&   childs  =   node->children();
        lua_newtable(L);
        int     i   =   0;
        for (auto& child : childs)
        {
            pushObj<FENode>(L,child);
            lua_seti(L,-2,++i);
        }
        return  1;
    }

    static  int Node_removeChild(lua_State* L)
    {
        auto    node    =   checkObj<FENode>(L,1);
        auto    child   =   checkObj<FENode>(L,2);
        lua_pushboolean(L,node->removeChild(child) ? 1 : 0);
        return  1;
    }

    static  const luaL_Reg  NodeMethods[]   =
    {
        {   "name",                 Node_name               },
        {   "setName",              Node_setName            },
        {   "localTranslation",     Node_localTranslation   },
        {   "setLocalTranslation",  Node_setLocalTranslation},
        {   "localScaling",         Node_localScaling       },
        {   "setLocalScaling",      Node_setLocalScaling    },
        {   "localRotation",        Node_localRotation      },
        {   "setLocalRotation",     Node_setLocalRotation   },
        {   "localTransform",       Node_localTransform     },
        {   "globalTranslation",    Node_globalTranslation  },
        {   "globalRotation",       Node_globalRotation     },
        {   "globalScaling",        Node_globalScaling      },
        {   "setGlobalTranslation", Node_setGlobalTranslation},
        {   "setGlobalRotation",    Node_setGlobalRotation  },
        {   "setGlobalScaling",     Node_setGlobalScaling   },
        {   "globalTransform",      Node_globalTransform    },
        {   "globalRSTransform",    Node_globalRSTransform  },
        {   "updateAabb",           Node_updateAabb         },
        {   "globalAabb",           Node_globalAabb         },
        {   "update",               Node_update             },
        {   "fireChanged",          Node_fireChanged        },
        {   "makeDirty",            Node_makeDirty          },
        {   "mesh",                 Node_mesh               },
        {   "setMesh",              Node_setMesh            },
        {   "color",                Node_color              },
        {   "setColor",             Node_setColor           },
        {   "visible",              Node_visible            },
        {   "addChild",             Node_addChild           },
        {   "removeChild",          Node_removeChild        },
        {   "children",             Node_children           },
        {   "reparent",             Node_reparent           },
        {   "root",                 Node_root               },
        {   "parent",               Node_parent             },
        {   "addComponent",         Node_addComponent       },
        {   "removeComponent",      Node_removeComponent    },
        {   nullptr,                nullptr                 },
    };

    // ==================== fe 核心表 ====================

    /// <summary>
    /// fe.log(msg):写引擎日志
    /// </summary>
    static  int FE_log(lua_State* L)
    {
        auto*   env =   getEnv(L);
        const char* msg  =   luaL_checkstring(L,1);
        if (env != nullptr && env->ctx != nullptr)
            env->ctx->log().infor("[lua] %s",msg);
        else
            printf("[lua] %s\n",msg);
        return  0;
    }

    /// <summary>
    /// 深度优先按名字查找节点(含顶层),找不到返回 nil
    /// </summary>
    static  bool    findNodeDFS(const Nodes& nodes,const char* name,Node& out)
    {
        for (auto& node : nodes)
        {
            if (node == nullptr)
                continue;
            if (node->nameIsValid() && strcmp(node->name(),name) == 0)
            {
                out =   node;
                return  true;
            }
            if (findNodeDFS(node->children(),name,out))
                return  true;
        }
        return  false;
    }

    static  int FE_findNode(lua_State* L)
    {
        auto*   env     =   getEnv(L);
        const char* name =   luaL_checkstring(L,1);
        if (env == nullptr || env->scene == nullptr)
            return  luaL_error(L,"fe.findNode: scene not set");
        Node    found;
        if (findNodeDFS(env->scene->nodeTree().topLevelNodes(),name,found))
            pushObj<FENode>(L,found);
        else
            lua_pushnil(L);
        return  1;
    }

    /// <summary>
    /// fe.createNode([name]):创建节点并加入场景节点树顶层
    /// </summary>
    static  int FE_createNode(lua_State* L)
    {
        auto*   env =   getEnv(L);
        if (env == nullptr || env->scene == nullptr)
            return  luaL_error(L,"fe.createNode: scene not set");
        Node    node    =   new FENode(*env->ctx);
        if (lua_isstring(L,1))
            node->setName(lua_tostring(L,1));
        env->scene->nodeTree().addToplevelNode(node);
        pushObj<FENode>(L,node);
        return  1;
    }

    /// <summary>
    /// fe.dispatchNode(node):把节点分发到渲染工厂与动画等组件系统
    /// 需在节点添加动画组件后、play 之前调用
    /// </summary>
    static  int FE_dispatchNode(lua_State* L)
    {
        auto*   env =   getEnv(L);
        if (env == nullptr || env->scene == nullptr)
            return  luaL_error(L,"fe.dispatchNode: scene not set");
        auto    node    =   checkObj<FENode>(L,1);
        env->scene->dispatchNodesToSystem({node});
        return  0;
    }

    static  const luaL_Reg  FEMethods[] =
    {
        {   "log",          FE_log          },
        {   "findNode",     FE_findNode     },
        {   "createNode",   FE_createNode   },
        {   "dispatchNode", FE_dispatchNode },
        {   nullptr,        nullptr         },
    };

    // ==================== VM ====================

    FELuaVM::FELuaVM(FEContext& ctx)
        :_ctx(&ctx)
    {
        init();
    }

    FELuaVM::~FELuaVM()
    {
        if (_L)
        {
            /// 释放注册表中的宿主环境
            delete  getEnv(_L);
            lua_close(_L);
            _L  =   nullptr;
        }
    }

    void    FELuaVM::setScene(FEScene* scene)
    {
        _scene  =   scene;
        auto*   env =   getEnv(_L);
        if (env)
            env->scene  =   scene;
    }

    FEScene*    FELuaVM::scene() const
    {
        return  _scene;
    }

    lua_State*  FELuaVM::state() const
    {
        return  _L;
    }

    void    FELuaVM::init()
    {
        _L  =   luaL_newstate();
        luaL_openlibs(_L);

        /// 宿主环境写入注册表
        auto*   env     =   new LuaEnv();
        env->ctx        =   _ctx;
        env->scene      =   _scene;
        setEnv(_L,env);

        /// fe 全局表
        lua_newtable(_L);
        luaL_setfuncs(_L,FEMethods,0);

        /// Node 类型
        registerType<FENode>(_L,NodeMethods);

        /// 数学绑定模块(fe.vec2/vec3/vec4/quat/mat3/mat4 与运算)
        FELuaRegisterMath(_L);

        /// 几何绑定模块(FE.Mesh + fe.geo 工厂)
        FELuaRegisterGeo(_L);

        /// 动画绑定模块
        FELuaRegisterAnim(_L);

        lua_setglobal(_L,"fe");
    }

    bool    FELuaVM::doFile(const char* file)
    {
        if (luaL_dofile(_L,file) != LUA_OK)
        {
            const char* err =   lua_tostring(_L,-1);
            _ctx->log().error("[lua] doFile(%s) failed: %s",file,err != nullptr ? err : "unknown");
            lua_pop(_L,1);
            return  false;
        }
        return  true;
    }

    bool    FELuaVM::doString(const char* code)
    {
        if (luaL_dostring(_L,code) != LUA_OK)
        {
            const char* err =   lua_tostring(_L,-1);
            _ctx->log().error("[lua] doString failed: %s",err != nullptr ? err : "unknown");
            lua_pop(_L,1);
            return  false;
        }
        return  true;
    }
}
