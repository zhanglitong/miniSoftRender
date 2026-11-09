#pragma     once

#include    "FEGeometryParam.hpp"
#include    "detail/FESVGGeometryImpl.hpp"
#include    "../fileFormat/svg/FESVGLoader.hpp"
#include    "../mesh/FEMeshHelper.hpp"
#include    <fstream>
#include    <sstream>

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometrySVG,"{7E3A9C41-5B62-4D8F-A1C7-2F9B4E6D8A30}");

    /// <summary>
    /// SVG 几何体:加载并解析 SVG 文件,生成可直接渲染的二维三角网格。
    /// 参考 three.js SVGLoader + SVGGeometry 的实现方式。
    /// </summary>
    class   FE_API  FEGeometrySVG :public FEGeometryParam
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometrySVG)
    public:
        FEGeometrySVG(FEContext& ctx)
            :FEGeometryParam(ctx)
        {}
        FEGeometrySVG(const FEGeometrySVG& other) = default;
        virtual ~FEGeometrySVG()
        {}
        /// <summary>
        /// 从文件加载 SVG 并解析
        /// </summary>
        /// <param name="file">SVG 文件路径</param>
        /// <returns>是否成功</returns>
        inline  bool    loadFile(const String& file)
        {
            const   String  text    =   readAll(file);
            if (text.empty())
                return  false;
            return  parse(text);
        }
        /// <summary>
        /// 直接从文本解析 SVG
        /// </summary>
        /// <param name="text">SVG 全文</param>
        /// <returns>是否成功</returns>
        inline  bool    parse(const String& text)
        {
            FESVGParseOptions   opt;
            opt._curveSegments  =   _options._curveSegments;
            _source             =   text;
            return  FESVGLoader::parseText(text,_document,opt);
        }
        /// <summary>
        /// 解析结果
        /// </summary>
        inline  const   FESVGDocument&  document() const
        {
            return  _document;
        }
        /// <summary>
        /// 构建选项
        /// </summary>
        inline  FESVGGeometryOptions&   options()
        {
            return  _options;
        }
        inline  const   FESVGGeometryOptions&   options() const
        {
            return  _options;
        }
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="inputs">属性集</param>
        /// <returns>网格对象</returns>
        virtual Mesh    triangular(const Attrs& inputs) override
        {
            /// 未显式加载过时,尝试用保存的源文本重建
            if (_document.paths.empty() && !_source.empty())
            {
                FESVGParseOptions   opt;
                opt._curveSegments  =   _options._curveSegments;
                FESVGLoader::parseText(_source,_document,opt);
            }

            FESVGGeometryData   data;
            FESVGGeometryImpl::build(_document,_options,data);
            if (data.indices.empty())
                return  nullptr;

            return  FEMeshBuilder::makeMesh(_ctx,inputs,data.vertices,data.normals,
                                            data.colors,data.uvs,data.indices);
        }

    protected:
        /// <summary>
        /// 序列化:保存 SVG 源文本与构建选项
        /// </summary>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chk,
                                        uint version,FESerializeCtx& ctx) const override
        {
            UNUSED(chk,version,ctx);
            writer.write(_source);
            writer.write(_options);
        }
        /// <summary>
        /// 反序列化
        /// </summary>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chk,
                                          uint version,FESerializeCtx& ctx) override
        {
            UNUSED(chk,version,ctx);
            reader.read(_source);
            reader.read(_options);
            if (!_source.empty())
                parse(_source);
        }

    private:
        /// <summary>
        /// 读取整个文件(用于序列化时保存源文本)
        /// </summary>
        static  String  readAll(const String& file)
        {
            std::ifstream       stream(file,std::ios::binary);
            if (!stream.is_open())
                return  String();
            std::stringstream   buffer;
            buffer << stream.rdbuf();
            return  buffer.str();
        }

        /// SVG 源文本(用于序列化)
        String                  _source;
        /// 解析结果
        FESVGDocument           _document;
        /// 构建选项
        FESVGGeometryOptions    _options;
    };
}
