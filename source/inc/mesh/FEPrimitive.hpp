#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEObject.h"
#include    "../FEWriterHelper.hpp"
#include    "../FEMath.hpp"
#include    "../FEFactory.hpp"

#include    "FELod.hpp"
namespace   FE
{
    enum    EPrimitive:uint8_t
    {
        PRI_POINTS          ,
        PRI_LINES           ,
        PRI_LINE_STRIP      ,
        PRI_TRIANGLES       ,
        PRI_TRIANGLE_STRIP  ,
        PRI_TRIANGLE_FAN    ,
        PRI_MAX             ,
    };

    inline  static  auto    nameOfEnum(EPrimitive pri)
    {
        switch(pri)
        {
        case PRI_POINTS        :    return  "POINTS";
        case PRI_LINES         :    return  "LINES";
        case PRI_LINE_STRIP    :    return  "LINE_STRIP";
        case PRI_TRIANGLES     :    return  "TRIANGLES";
        case PRI_TRIANGLE_STRIP:    return  "TRIANGLE_STRIP";
        case PRI_TRIANGLE_FAN  :    return  "TRIANGLE_FAN";
        }
        return  "POINTS";
    }
    inline  static  auto    primitiveFromName(const char* name)
    {
        if(     _stricmp(name,  nameOfEnum(PRI_POINTS        )) == 0)   return  PRI_POINTS          ;   
        else if(_stricmp(name,  nameOfEnum(PRI_LINES         )) == 0)   return  PRI_LINES           ;
        else if(_stricmp(name,  nameOfEnum(PRI_LINE_STRIP    )) == 0)   return  PRI_LINE_STRIP      ;
        else if(_stricmp(name,  nameOfEnum(PRI_TRIANGLES     )) == 0)   return  PRI_TRIANGLES       ;
        else if(_stricmp(name,  nameOfEnum(PRI_TRIANGLE_STRIP)) == 0)   return  PRI_TRIANGLE_STRIP  ;
        else if(_stricmp(name,  nameOfEnum(PRI_TRIANGLE_FAN  )) == 0)   return  PRI_TRIANGLE_FAN    ;
        else                                                            return  PRI_POINTS          ;
    }

    enum    EDrawType:uint8_t
    {
        DRAW_ARRAY          ,
        DRAW_ELEMENT_UINT8  ,
        DRAW_ELEMENT_UINT16 ,
        DRAW_ELEMENT_UINT32 ,
        DRAW_MAX            ,
    };

    class   FEFactory;
    using   Factory         =   SharedPtr<FEFactory>;

    DEFINE_CLASS_UUID(FEPrimitive,"{7ACF8398-2D48-490C-8F82-F21CDAB62E75}");

    class   FEPrimitive :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEPrimitive)
    public:
        FEPrimitive(FEContext& ctx,EDrawType type = DRAW_ARRAY,EPrimitive pri = PRI_TRIANGLES)
            :FEObject(ctx)
        {
            _primitive  =   pri;
            _type       =   type;
        }
        FEPrimitive(const FEPrimitive& other)
            :FEObject(other)
        {
            _primitive  =   other._primitive;
            _type       =   other._type;
        }
        inline  auto    primitive() const
        {
            return  _primitive;
        }
        inline  auto&   setPrimitive(EPrimitive pri)
        {
            _primitive  =   pri;
            return  *this;
        }
        inline  auto    type() const
        {
            return  _type;
        }
        inline  auto&   setFactory(Factory factory)
        {
            _factory    =   factory;
            return  *this;
        }
        inline  auto    factory() const
        {
            return  _factory;
        }
        inline  void    setVertexOffset(int offset)
        {
            _vertexOffset   =   offset;
        }
        inline  int     vertexOffset() const
        {
            return  _vertexOffset;
        }
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const override
        {
            (void)ctx;
            (void)version;
            (void)chk;
            writer.write(_primitive);
            writer.write(_type);
        }
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chk,uint version,FESerializeCtx& ctx) override
        {
            (void)ctx;
            (void)version;
            (void)chk;

            reader.read(_primitive);
            reader.read(_type);
        }
    protected:
        EPrimitive  _primitive      =   PRI_TRIANGLES;
        EDrawType   _type           =   DRAW_ARRAY;
        int         _vertexOffset   =   0;
        /// <summary>
        /// FEFactoryRender; 用来访问对应GPU数据
        /// </summary>
        Factory     _factory        =   nullptr;
        /// <summary>
        /// 大多数情况下没有lod
        /// </summary>
        Lod         _lod            =   nullptr;

    };
    using   Primitive   =   SharedPtr<FEPrimitive>;
    using   Primitives  =   std::vector<Primitive>;
}

