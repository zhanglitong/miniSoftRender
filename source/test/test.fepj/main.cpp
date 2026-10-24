#include    <cstdio>
#include    <cstring>
#include    <filesystem>
#include    <string>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
#include    "../../inc/FENodeHelper.hpp"
#include    "../../inc/FEProperty.hpp"
#include    "../../inc/stream/FEWriterMemrory.hpp"
#include    "../../inc/stream/FEReaderMemory.hpp"

using   namespace   FE;

namespace
{
    int     g_pass  =   0;
    int     g_fail  =   0;

    void    check(bool ok,const char* label)
    {
        if (ok)     {   ++  g_pass;     printf("[PASS] %s\n",label);    }
        else        {   ++  g_fail;     printf("[FAIL] %s\n",label);    }
        fflush(stdout);
    }
    template<typename T>
    bool    valueIs(const FEProperty& prop,const char* key,const T& expected)
    {
        const   auto*   obj =   prop.values().isExist(key);
        if (obj == nullptr)
            return  false;
        if (!std::holds_alternative<T>(obj->value()))
            return  false;
        return  std::get<T>(obj->value()) == expected;
    }
};

/// ==================== FEProperty 各类型往返 ====================
static  void    testPropertyRoundTrip(FEContext& ctx)
{
    FEProperty  src(ctx);
    src.setValue("int",     int64(-9876543210LL));
    src.setValue("real",    real(3.25));
    src.setValue("str",     String("hello fepj"));
    src.setValue("empty",   String());
    src.setValue("strs",    Strings{"a","bb","ccc"});
    src.setValue("u8s",     uint8s{1,2,3,255});
    src.setValue("reals",   reals{1.5,-2.5,3.0,1000000.0});
    src.setValue("uuid",    UUIDOF(FENode));

    Buffer          buf =   new FEBuffer(ctx);
    buf->cInfo()._buffer.resize(4096);
    FEWriterMemory  writer(ctx,buf);
    FESerializeCtx  xsCtx;
    uint            version =   0;
    src.serialize(writer,version,xsCtx);
    const   uint64  written =   writer.tell();
    check(written > 0 && written < 4096,"property: serialize bytes");
    /// 收缩到实际写入长度,读取越界会直接断言
    buf->cInfo()._buffer.resize(size_t(written));

    FEProperty      dst(ctx);
    FEReaderMemory  reader(ctx,buf);
    dst.deserialize(reader,version,xsCtx);

    check(dst.values().data().size() == src.values().data().size(),"property: value count");
    check(valueIs<int64>(dst,"int",     int64(-9876543210LL)),      "property: int64");
    check(valueIs<real>(dst,"real",     real(3.25)),                "property: real");
    check(valueIs<String>(dst,"str",    String("hello fepj")),      "property: string");
    check(valueIs<String>(dst,"empty",  String()),                  "property: empty string");
    check(valueIs<Strings>(dst,"strs",  Strings{"a","bb","ccc"}),   "property: string array");
    check(valueIs<uint8s>(dst,"u8s",    uint8s{1,2,3,255}),         "property: uint8 array");
    check(valueIs<reals>(dst,"reals",   reals{1.5,-2.5,3.0,1000000.0}),"property: real array");
    check(valueIs<FEUuid>(dst,"uuid",   UUIDOF(FENode)),            "property: uuid");
    check(reader.tell() == written,"property: stream consumed exactly");
}

/// ==================== 节点树构造与校验 ====================
static  Node    makeTree(FEContext& ctx,const char* rootName,size_t childCount)
{
    Node    root = new FENode(ctx);
    root->setName(rootName);
    root->setLocalTranslation(real3(real(1),real(2),real(3)));
    for (size_t i = 0; i < childCount; ++ i)
    {
        Node    child = new FENode(ctx);
        child->setName("child_" + std::to_string(i));
        child->setLocalTranslation(real3(real(i),real(i) * 2,real(i) * 3));
        child->setLocalScaling(real3(real(i) + 1,real(i) + 1,real(i) + 1));
        root->addChild(child);
    }
    return  root;
}
static  bool    verifyTree(const Node& root,const char* rootName,size_t childCount)
{
    if (root == nullptr)
    {   printf("       verify: root is null\n");    return  false;    }
    if (root->children().size() != childCount)
    {   printf("       verify: child count %zu != %zu\n",root->children().size(),childCount);  return  false;  }
    if (strcmp(root->name(),rootName) != 0)
    {   printf("       verify: root name '%s' != '%s'\n",root->name(),rootName);   return  false;  }
    const   auto    rootTrans =   root->localTranslation();
    if (rootTrans.x != 1.0 || rootTrans.y != 2.0 || rootTrans.z != 3.0)
    {
        printf("       verify: root pos (%f,%f,%f) != (1,2,3)\n",
            double(rootTrans.x),double(rootTrans.y),double(rootTrans.z));
        return  false;
    }
    for (size_t i = 0; i < childCount; ++ i)
    {
        const   auto&   child =   root->children()[i];
        if (child == nullptr)
        {   printf("       verify: child[%zu] is null\n",i);    return  false;    }
        const   String  expectedName =   "child_" + std::to_string(i);
        if (strcmp(child->name(),expectedName.c_str()) != 0)
        {
            printf("       verify: child[%zu] name '%s' != '%s'\n",i,child->name(),expectedName.c_str());
            return  false;
        }
        const   auto    pos =   child->localTranslation();
        if (pos.x != real(i) || pos.y != real(i) * 2 || pos.z != real(i) * 3)
        {
            printf("       verify: child[%zu] pos (%f,%f,%f) != (%f,%f,%f)\n",i,
                double(pos.x),double(pos.y),double(pos.z),
                double(real(i)),double(real(i) * 2),double(real(i) * 3));
            return  false;
        }
        if (child->localScaling().x != real(i) + 1)
        {
            printf("       verify: child[%zu] scale %f != %f\n",i,
                double(child->localScaling().x),double(real(i) + 1));
            return  false;
        }
    }
    return  true;
}
static  FEFileFormat   makeFormat()
{
    FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
    fmt._type   =   FEFileFormat::DT_Model;
    fmt._mode   =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
    return  fmt;
}

/// ==================== 节点树内存往返 ====================
static  void    testNodeMemoryRoundTrip(FEContext& ctx)
{
    Node    root = makeTree(ctx,"root_mem",100);
    Buffer  buf  =   new FEBuffer(ctx);
    buf->cInfo()._buffer.resize(1024 * 1024);
    FEWriterMemory  writer(ctx,buf);
    FESerializeCtx  xsCtx;

    FEFileFormat    fmt         =   makeFormat();
    auto            writerFmt   =   FEFileFormatHelper::queryWriter(ctx,fmt);
    check(writerFmt != nullptr,"memory: query writer");
    if (writerFmt == nullptr)
        return;
    check(writerFmt->writeBuffer({root.get()},writer),"memory: write buffer");
    const   uint64  written =   writer.tell();
    check(written > 0,"memory: buffer bytes");
    buf->cInfo()._buffer.resize(size_t(written));

    auto    readerFmt =   FEFileFormatHelper::queryReader(ctx,fmt);
    check(readerFmt != nullptr,"memory: query reader");
    if (readerFmt == nullptr)
        return;
    Objects objects =   readerFmt->readBuffers({buf});
    check(objects.size() == 1,"memory: object count");
    if (objects.size() != 1)
        return;
    Node    loaded =   objects[0]->cast<FENode>();
    check(loaded != nullptr,"memory: root is node");
    if (loaded == nullptr)
        return;
    check(loaded->objectId() == root->objectId(),"memory: object id preserved");
    check(verifyTree(loaded,"root_mem",100),"memory: tree content");
}

/// ==================== 节点树文件往返 ====================
static  void    testNodeFileRoundTrip(FEContext& ctx)
{
    namespace   fs  =   std::filesystem;
    const   fs::path    path =   fs::temp_directory_path() / "fe_test_fepj_node.fepj";
    Node    root = makeTree(ctx,"root_file",64);

    FEFileFormat    fmt         =   makeFormat();
    auto            writerFmt   =   FEFileFormatHelper::queryWriter(ctx,fmt);
    check(writerFmt != nullptr,"file: query writer");
    if (writerFmt == nullptr)
        return;
    check(writerFmt->writeFile({root.get()},path.string()),"file: write");
    check(fs::exists(path),"file: exists");
    if (!fs::exists(path))
        return;
    const   uint64  fileSize    =   uint64(fs::file_size(path));
    check(fileSize > 0,"file: size");
    printf("       file size = %llu bytes\n",(unsigned long long)fileSize);

    auto    readerFmt =   FEFileFormatHelper::queryReader(ctx,fmt);
    check(readerFmt != nullptr,"file: query reader");
    if (readerFmt != nullptr)
    {
        Objects objects =   readerFmt->readFiles({path.string()});
        check(objects.size() == 1,"file: object count");
        if (objects.size() == 1)
        {
            Node    loaded =   objects[0]->cast<FENode>();
            check(loaded != nullptr,"file: root is node");
            if (loaded != nullptr)
            {
                check(loaded->objectId() == root->objectId(),"file: object id preserved");
                check(verifyTree(loaded,"root_file",64),"file: tree content");
            }
        }
    }
    fs::remove(path);
}

int     main(int argc,char** argv)
{
    UNUSED(argc,argv);
    setvbuf(stdout,nullptr,_IONBF,0);
    FEContext&      ctx =   FEContext::instance();

    printf("=== test.fepj: serialization round-trip ===\n");
    testPropertyRoundTrip(ctx);
    testNodeMemoryRoundTrip(ctx);
    testNodeFileRoundTrip(ctx);
    printf("=== %d passed, %d failed ===\n",g_pass,g_fail);

    return  g_fail == 0 ? 0 : 1;
}
