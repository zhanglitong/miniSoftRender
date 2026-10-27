/// FEQuickJS 实现:VM 生命周期 + fe 核心表(场景胶水/日志) + Node 绑定
/// 数学/几何/动画绑定分别在 FEQuickJSMath.cpp/FEQuickJSGeo.cpp/FEQuickJSAnim.cpp 中实现,
/// 由 init() 调用各注册入口

#include    "FEQuickJS.hpp"

#include    "FEQuickJSHelper.hpp"

#include    <quickjs.h>

#include    <stdio.h>
#include    <string.h>
#include    <string>
#include    <vector>

namespace   FE
{
    /// 各绑定模块注册入口(FEQuickJSGeo.cpp/FEQuickJSAnim.cpp;
    /// Math 声明于 FEQuickJSMath.hpp)
    void    FEQJSRegisterGeo(JSContext* ctx,JSEnv* env,JSValue fe);
    void    FEQJSRegisterAnim(JSContext* ctx,JSEnv* env,JSValue fe);

    /// 深度优先按名字查找节点(含顶层),找不到返回 false
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

    // ==================== Node 绑定 ====================

    static  JSValue Node_name(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        JSValue ret =   JS_NewString(ctx,node->name());
        return  ret;
    }

    /// setter 统一返回 self,支持链式调用
    static  JSValue Node_setName(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        if (!JS_IsString(argv[0]))
            return  JS_ThrowTypeError(ctx,"setName: expect string");
        const char* name    =   JS_ToCString(ctx,argv[0]);
        if (name == nullptr)
            return  JS_EXCEPTION;
        node->setName(name);
        JS_FreeCString(ctx,name);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_localTranslation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushReal3(ctx,env,node->localTranslation());
    }

    static  JSValue Node_setLocalTranslation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        real3   pos;
        if (!toReal3(ctx,argv[0],pos))
            return  JS_ThrowTypeError(ctx,"setLocalTranslation: expect vec3 or {x,y,z} / [x,y,z]");
        node->setLocalTranslation(pos);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_localScaling(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushReal3(ctx,env,node->localScaling());
    }

    static  JSValue Node_setLocalScaling(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        real3   scale;
        if (!toReal3(ctx,argv[0],scale))
            return  JS_ThrowTypeError(ctx,"setLocalScaling: expect vec3 or {x,y,z} / [x,y,z]");
        node->setLocalScaling(scale);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_localRotation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushQuatr(ctx,env,node->localRotation());
    }

    static  JSValue Node_setLocalRotation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        quatr   rot;
        if (!toQuatr(ctx,argv[0],rot))
            return  JS_ThrowTypeError(ctx,"setLocalRotation: expect quat or {x,y,z,w} / [x,y,z,w]");
        node->setLocalRotation(rot);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_localTransform(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushMat4(ctx,env,node->localTransform());
    }

    static  JSValue Node_globalTranslation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushReal3(ctx,env,node->globalTranslation());
    }

    static  JSValue Node_globalRotation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushQuatr(ctx,env,node->globalRotation());
    }

    static  JSValue Node_globalScaling(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushReal3(ctx,env,node->globalScaling());
    }

    static  JSValue Node_setGlobalTranslation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        real3   pos;
        if (!toReal3(ctx,argv[0],pos))
            return  JS_ThrowTypeError(ctx,"setGlobalTranslation: expect vec3 or {x,y,z} / [x,y,z]");
        node->setGlobalTranslation(pos);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_setGlobalRotation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        quatr   rot;
        if (!toQuatr(ctx,argv[0],rot))
            return  JS_ThrowTypeError(ctx,"setGlobalRotation: expect quat or {x,y,z,w} / [x,y,z,w]");
        node->setGlobalRotation(rot);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_setGlobalScaling(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        real3   scale;
        if (!toReal3(ctx,argv[0],scale))
            return  JS_ThrowTypeError(ctx,"setGlobalScaling: expect vec3 or {x,y,z} / [x,y,z]");
        node->setGlobalScaling(scale);
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_globalTransform(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushMat4(ctx,env,node->globalTransform());
    }

    static  JSValue Node_globalRSTransform(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushMat4(ctx,env,node->globalRSTransform());
    }

    /// node:updateAabb([recursion]):计算并返回 {min:vec3,max:vec3},recursion 缺省 true
    static  JSValue Node_updateAabb(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        bool    recursion   =   true;
        if (argc >= 1 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0]))
            recursion   =   JS_ToBool(ctx,argv[0]) > 0;
        return  pushAabb3dr(ctx,env,node->updateAabb(recursion));
    }

    static  JSValue Node_globalAabb(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushAabb3dr(ctx,env,node->globalAabb());
    }

    /// node:update([tm]):刷新节点(tm 为动画时间,缺省 0)
    static  JSValue Node_update(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        double  tm  =   0.0;
        if (argc >= 1 && JS_IsNumber(argv[0]))
            JS_ToFloat64(ctx,&tm,argv[0]);
        node->update((real)tm);
        return  JS_UNDEFINED;
    }

    static  JSValue Node_fireChanged(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        node->fireChanged();
        return  JS_UNDEFINED;
    }

    static  JSValue Node_makeDirty(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        node->makeDirty();
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_mesh(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushObj<FEMesh>(ctx,env,node->mesh());
    }

    static  JSValue Node_setMesh(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        auto    mesh    =   checkObj<FEMesh>(ctx,env,argv[0]);
        if (mesh == nullptr)
            return  JS_EXCEPTION;
        node->setMesh(mesh.get());
        return  JS_DupValue(ctx,this_val);
    }

    static  JSValue Node_color(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushUint8x4(ctx,node->color().value());
    }

    static  JSValue Node_setColor(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        uint8x4 rgba;
        if (!toUint8x4(ctx,argv[0],rgba))
            return  JS_ThrowTypeError(ctx,"setColor: expect {r,g,b,a} or [r,g,b,a] (0-255)");
        node->setColor(Rgba8(rgba));
        return  JS_DupValue(ctx,this_val);
    }

    /// <summary>
    /// 可见性:visible() 查询;visible(true/false) 设置并返回 self
    /// </summary>
    static  JSValue Node_visible(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        if (argc >= 1 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0]))
        {
            if (JS_ToBool(ctx,argv[0]) > 0)
                node->flags().addFlag(FLAG_VISIBLE);
            else
                node->flags().removeFlag(FLAG_VISIBLE);
            return  JS_DupValue(ctx,this_val);
        }
        return  JS_NewBool(ctx,node->flags().hasFlag(FLAG_VISIBLE));
    }

    static  JSValue Node_addChild(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        auto    child   =   checkObj<FENode>(ctx,env,argv[0]);
        if (child == nullptr)
            return  JS_EXCEPTION;
        node->addChild(child);
        return  JS_UNDEFINED;
    }

    static  JSValue Node_removeChild(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        auto    child   =   checkObj<FENode>(ctx,env,argv[0]);
        if (child == nullptr)
            return  JS_EXCEPTION;
        return  JS_NewBool(ctx,node->removeChild(child));
    }

    static  JSValue Node_children(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        JSValue arr =   JS_NewArray(ctx);
        if (JS_IsException(arr))
            return  arr;
        uint32_t    i   =   0;
        for (auto& child : node->children())
        {
            if (child == nullptr)
                continue;
            JS_DefinePropertyValueUint32(ctx,arr,i++,pushObj<FENode>(ctx,env,child),JS_PROP_C_W_E);
        }
        return  arr;
    }

    /// <summary>
    /// node:reparent([parent][,index]):重新挂接父子关系并保持世界变换不变
    /// parent 为 null/undefined 时提升为顶层;index 为新父节点孩子列表的 0-based
    /// 插入位置,缺省(SIZE_MAX)追加末尾
    /// </summary>
    static  JSValue Node_reparent(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        FENode* target  =   nullptr;
        if (argc >= 1 && !JS_IsNull(argv[0]) && !JS_IsUndefined(argv[0]))
        {
            auto    parent  =   checkObj<FENode>(ctx,env,argv[0]);
            if (parent == nullptr)
                return  JS_EXCEPTION;
            target  =   parent.get();
        }
        size_t  index   =   SIZE_MAX;
        if (argc >= 2 && !JS_IsNull(argv[1]) && !JS_IsUndefined(argv[1]))
        {
            int64_t idx =   0;
            if (JS_ToInt64(ctx,&idx,argv[1]) < 0)
                return  JS_EXCEPTION;
            if (idx < 0)
                return  JS_ThrowRangeError(ctx,"reparent: index must be >= 0");
            index   =   (size_t)idx;
        }
        /// 层级变更统一走节点树:它会同步顶层集合(reparent 只改父子关系)
        auto    scene   =   node->ctx().scene();
        if (scene == nullptr)
            return  JS_ThrowTypeError(ctx,"reparent: no active scene");
        if (!scene->nodeTree().setParent(node,target,index))
            return  JS_ThrowTypeError(ctx,"reparent: cannot attach to self or descendant");
        return  JS_UNDEFINED;
    }

    static  JSValue Node_root(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        return  pushObj<FENode>(ctx,env,SharedPtr<FENode>(node->root()));
    }

    static  JSValue Node_parent(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc); UNUSED(argv);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        auto    parent  =   node->parent();
        if (parent == nullptr)
            return  JS_NULL;
        return  pushObj<FENode>(ctx,env,parent);
    }

    /// <summary>
    /// 添加组件(当前绑定类型中仅 Animation 是组件),成功返回 true
    /// </summary>
    static  JSValue Node_addComponent(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        if (!isObj<FEAnimation>(env,argv[0]))
            return  JS_ThrowTypeError(ctx,"addComponent: expect FE.Animation");
        auto    anim    =   checkObj<FEAnimation>(ctx,env,argv[0]);
        return  JS_NewBool(ctx,node->addComponent(anim.get()));
    }

    static  JSValue Node_removeComponent(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(argc);
        auto*   env =   getEnv(ctx);
        auto    node    =   checkObj<FENode>(ctx,env,this_val);
        if (node == nullptr)
            return  JS_EXCEPTION;
        if (!isObj<FEAnimation>(env,argv[0]))
            return  JS_ThrowTypeError(ctx,"removeComponent: expect FE.Animation");
        auto    anim    =   checkObj<FEAnimation>(ctx,env,argv[0]);
        return  JS_NewBool(ctx,node->removeComponent(anim.get()));
    }

    static  const JSFuncDef NodeMethods[]   =
    {
        {   "name",                 Node_name               ,   0   },
        {   "setName",              Node_setName            ,   1   },
        {   "localTranslation",     Node_localTranslation   ,   0   },
        {   "setLocalTranslation",  Node_setLocalTranslation,   1   },
        {   "localScaling",         Node_localScaling       ,   0   },
        {   "setLocalScaling",      Node_setLocalScaling    ,   1   },
        {   "localRotation",        Node_localRotation      ,   0   },
        {   "setLocalRotation",     Node_setLocalRotation   ,   1   },
        {   "localTransform",       Node_localTransform     ,   0   },
        {   "globalTranslation",    Node_globalTranslation  ,   0   },
        {   "globalRotation",       Node_globalRotation     ,   0   },
        {   "globalScaling",        Node_globalScaling      ,   0   },
        {   "setGlobalTranslation", Node_setGlobalTranslation,  1   },
        {   "setGlobalRotation",    Node_setGlobalRotation  ,   1   },
        {   "setGlobalScaling",     Node_setGlobalScaling   ,   1   },
        {   "globalTransform",      Node_globalTransform    ,   0   },
        {   "globalRSTransform",    Node_globalRSTransform  ,   0   },
        {   "updateAabb",           Node_updateAabb         ,   0   },
        {   "globalAabb",           Node_globalAabb         ,   0   },
        {   "update",               Node_update             ,   0   },
        {   "fireChanged",          Node_fireChanged        ,   0   },
        {   "makeDirty",            Node_makeDirty          ,   0   },
        {   "mesh",                 Node_mesh               ,   0   },
        {   "setMesh",              Node_setMesh            ,   1   },
        {   "color",                Node_color              ,   0   },
        {   "setColor",             Node_setColor           ,   1   },
        {   "visible",              Node_visible            ,   0   },
        {   "addChild",             Node_addChild           ,   1   },
        {   "removeChild",          Node_removeChild        ,   1   },
        {   "children",             Node_children           ,   0   },
        {   "reparent",             Node_reparent           ,   0   },
        {   "root",                 Node_root               ,   0   },
        {   "parent",               Node_parent             ,   0   },
        {   "addComponent",         Node_addComponent       ,   1   },
        {   "removeComponent",      Node_removeComponent    ,   1   },
        {   nullptr,                nullptr                 ,   0   },
    };

    // ==================== fe 核心表 ====================

    /// <summary>
    /// fe.log(...):任意参数以空格拼接后写引擎日志
    /// </summary>
    static  JSValue FE_log(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(this_val);
        auto*   env =   getEnv(ctx);
        std::string msg;
        for (int i = 0; i < argc; ++i)
        {
            if (i > 0)
                msg +=  ' ';
            const char* str =   JS_ToCString(ctx,argv[i]);
            if (str != nullptr)
            {
                msg +=  str;
                JS_FreeCString(ctx,str);
            }
            else
            {
                JS_FreeValue(ctx,JS_GetException(ctx));
                msg +=  "<exception>";
            }
        }
        if (env != nullptr && env->ctx != nullptr)
            env->ctx->log().infor("[js] %s",msg.c_str());
        else
            printf("[js] %s\n",msg.c_str());
        return  JS_UNDEFINED;
    }

    /// <summary>
    /// fe.findNode(name):深度优先按名字查找节点(含顶层),找不到返回 null
    /// </summary>
    static  JSValue FE_findNode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(this_val); UNUSED(argc);
        auto*   env     =   getEnv(ctx);
        if (!JS_IsString(argv[0]))
            return  JS_ThrowTypeError(ctx,"fe.findNode: expect string name");
        const char* name    =   JS_ToCString(ctx,argv[0]);
        if (name == nullptr)
            return  JS_EXCEPTION;
        if (env == nullptr || env->scene == nullptr)
        {
            JS_FreeCString(ctx,name);
            return  JS_ThrowTypeError(ctx,"fe.findNode: scene not set");
        }
        Node    found;
        bool    ok  =   findNodeDFS(env->scene->nodeTree().topLevelNodes(),name,found);
        JS_FreeCString(ctx,name);
        if (ok)
            return  pushObj<FENode>(ctx,env,found);
        return  JS_NULL;
    }

    /// <summary>
    /// fe.createNode([name]):创建节点并加入场景节点树顶层
    /// </summary>
    static  JSValue FE_createNode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(this_val);
        auto*   env =   getEnv(ctx);
        if (env == nullptr || env->scene == nullptr)
            return  JS_ThrowTypeError(ctx,"fe.createNode: scene not set");
        Node    node    =   new FENode(*env->ctx);
        if (argc >= 1 && JS_IsString(argv[0]))
        {
            const char* name    =   JS_ToCString(ctx,argv[0]);
            if (name != nullptr)
            {
                node->setName(name);
                JS_FreeCString(ctx,name);
            }
        }
        env->scene->nodeTree().addToplevelNode(node);
        return  pushObj<FENode>(ctx,env,node);
    }

    /// <summary>
    /// fe.dispatchNode(node):把节点分发到渲染工厂与动画等组件系统
    /// 需在节点添加动画组件后、play 之前调用
    /// </summary>
    static  JSValue FE_dispatchNode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
    {
        UNUSED(this_val); UNUSED(argc);
        auto*   env =   getEnv(ctx);
        if (env == nullptr || env->scene == nullptr)
            return  JS_ThrowTypeError(ctx,"fe.dispatchNode: scene not set");
        auto    node    =   checkObj<FENode>(ctx,env,argv[0]);
        if (node == nullptr)
            return  JS_EXCEPTION;
        env->scene->dispatchNodesToSystem({node});
        return  JS_UNDEFINED;
    }

    // ==================== VM ====================

    FEQuickJS::FEQuickJS(FEContext& ctx)
        :_ctx(&ctx)
    {
        init();
    }

    FEQuickJS::~FEQuickJS()
    {
        if (_js != nullptr)
        {
            /// 释放数学共享原型与宿主环境(须在 JS_FreeContext 前,
            /// JS_FreeValue 依赖 context 仍存活)
            auto*   env =   (JSEnv*)JS_GetContextOpaque(_js);
            if (env != nullptr)
            {
                JS_FreeValue(_js,env->protoVec2);
                JS_FreeValue(_js,env->protoVec3);
                JS_FreeValue(_js,env->protoVec4);
                JS_FreeValue(_js,env->protoQuat);
                JS_FreeValue(_js,env->protoMat3);
                JS_FreeValue(_js,env->protoMat4);
                delete  env;
            }
            JS_SetContextOpaque(_js,nullptr);
            JS_FreeContext(_js);
            _js =   nullptr;
        }
        if (_rt != nullptr)
        {
            /// finalizer 仅经 JS_GetClassID 取类ID,不回查 env;
            /// 此处清掉 opaque 后再释放 runtime,对象 finalizer 随之执行
            JS_SetRuntimeOpaque(_rt,nullptr);
            JS_FreeRuntime(_rt);
            _rt =   nullptr;
        }
    }

    void    FEQuickJS::setScene(FEScene* scene)
    {
        _scene  =   scene;
        if (_js != nullptr)
        {
            auto*   env =   getEnv(_js);
            if (env != nullptr)
                env->scene  =   scene;
        }
    }

    FEScene*    FEQuickJS::scene() const
    {
        return  _scene;
    }

    JSContext*  FEQuickJS::context() const
    {
        return  _js;
    }

    void    FEQuickJS::init()
    {
        _rt =   JS_NewRuntime();
        _js =   JS_NewContext(_rt);

        /// 宿主环境同时挂 runtime+context opaque,绑定代码经 getEnv(ctx) 取用
        auto*   env     =   new JSEnv();
        env->ctx        =   _ctx;
        env->scene      =   _scene;
        JS_SetRuntimeOpaque(_rt,env);
        JS_SetContextOpaque(_js,env);

        /// fe 全局表
        JSValue global  =   JS_GetGlobalObject(_js);
        JSValue fe      =   JS_NewObject(_js);
        JS_SetPropertyStr(_js,fe,"log",JS_NewCFunction(_js,FE_log,"log",1));
        JS_SetPropertyStr(_js,fe,"findNode",JS_NewCFunction(_js,FE_findNode,"findNode",1));
        JS_SetPropertyStr(_js,fe,"createNode",JS_NewCFunction(_js,FE_createNode,"createNode",1));
        JS_SetPropertyStr(_js,fe,"dispatchNode",JS_NewCFunction(_js,FE_dispatchNode,"dispatchNode",1));

        /// Node 类型(FE.Node 原型方法表)
        registerClass<FENode>(_js,env,NodeMethods);

        /// 数学绑定模块(fe.vec2/vec3/vec4/quat/mat3/mat4 等构造与方法)
        FEQJSRegisterMath(_js,env,fe);

        /// 几何绑定模块(FE.Mesh + fe.geo 13 工厂)
        FEQJSRegisterGeo(_js,env,fe);

        /// 动画绑定模块(FE.Animation 等 5 类 + fe.anim)
        FEQJSRegisterAnim(_js,env,fe);

        JS_SetPropertyStr(_js,global,"fe",fe);
        JS_FreeValue(_js,global);
    }

    /// 异常对象取 stack 字段(仅 Error 类型有),拼为多行字符串
    static  void    exceptionToString(JSContext* ctx,JSValueConst exc,std::string& out)
    {
        const char* str =   JS_ToCString(ctx,exc);
        if (str != nullptr)
        {
            out +=  str;
            JS_FreeCString(ctx,str);
        }
        if (JS_IsError(exc))
        {
            JSValue stack   =   JS_GetPropertyStr(ctx,exc,"stack");
            if (!JS_IsUndefined(stack) && !JS_IsException(stack))
            {
                const char* s   =   JS_ToCString(ctx,stack);
                if (s != nullptr)
                {
                    out +=  "\n";
                    out +=  s;
                    JS_FreeCString(ctx,s);
                }
            }
            JS_FreeValue(ctx,stack);
        }
    }

    /// 取出并记录当前挂起异常,吞掉(返回后栈上不再有异常)
    static  void    logPendingException(JSContext* js,FEContext* feCtx,const char* where)
    {
        JSValue exc     =   JS_GetException(js);
        std::string msg;
        exceptionToString(js,exc,msg);
        JS_FreeValue(js,exc);
        if (feCtx != nullptr)
            feCtx->log().error("[js] %s: %s",where,msg.c_str());
        else
            printf("[js] %s: %s\n",where,msg.c_str());
    }

    /// 读入整个文件并补 NUL 终结符(JS_Eval 要求 input[input_len] = '\0'),
    /// 内容按 UTF-8 交给 JS_Eval;返回的缓冲区长度含 NUL,调用方以 size()-1 为 input_len
    static  bool    readFileAll(const char* file,std::vector<char>& out)
    {
        FILE*   fp  =   fopen(file,"rb");
        if (fp == nullptr)
            return  false;
        fseek(fp,0,SEEK_END);
        long    size    =   ftell(fp);
        fseek(fp,0,SEEK_SET);
        if (size < 0)
        {
            fclose(fp);
            return  false;
        }
        out.resize((size_t)size + 1);
        size_t  read    =   size > 0 ? fread(out.data(),1,(size_t)size,fp) : 0;
        fclose(fp);
        if (read != (size_t)size)
            return  false;
        out[(size_t)size]   =   '\0';
        return  true;
    }

    bool    FEQuickJS::doFile(const char* file)
    {
        std::vector<char>   buf;
        if (!readFileAll(file,buf))
        {
            _ctx->log().error("[js] doFile(%s): open/read failed",file);
            return  false;
        }
        JSValue ret =   JS_Eval(_js,buf.data(),buf.size() - 1,file,JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(ret))
        {
            logPendingException(_js,_ctx,("doFile(" + std::string(file) + ")").c_str());
            return  false;
        }
        JS_FreeValue(_js,ret);
        return  true;
    }

    bool    FEQuickJS::doString(const char* code)
    {
        JSValue ret =   JS_Eval(_js,code,strlen(code),"<string>",JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(ret))
        {
            logPendingException(_js,_ctx,"doString");
            return  false;
        }
        JS_FreeValue(_js,ret);
        return  true;
    }
}
