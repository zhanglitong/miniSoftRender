#pragma     once
#include    "TValueArray.hpp"

/// <summary>
/// 值数组对象统一声明列表
/// 参数依次为: 类名, 智能指针别名, 元素类型, 类UUID
/// 类UUID 已写入文件格式,不要修改
/// 新增值数组对象只需要在该列表中添加一行
/// </summary>
#define FE_VALUE_ARRAY_OBJECT_LIST(X)                                                                   \
    X(FERealsObject,    RealsObject,    real,       "{81E552B1-58DE-4394-8B1F-D95FB092DBDF}")           \
    X(FEReal2sObject,   Real2sObject,   real2,      "{DA8B20FA-F296-4AEC-9C3C-A162BF5ED89A}")           \
    X(FEReal3sObject,   Real3sObject,   real3,      "{39994444-4CD5-41A6-BEC9-A18F863EEC85}")           \
    X(FEReal4sObject,   Real4sObject,   real4,      "{E3F86B94-D7E3-4A8E-B85E-BC880C551588}")           \
    X(FEQuatrsObject,   QuatrsObject,   quatr,      "{2D4104C9-3103-4DEA-910A-B7891F2F9C24}")           \
    X(FEFloatsObject,   FloatsObject,   float,      "{16D4E9B9-5FD5-4ECC-854A-B0326BAF3808}")           \
    X(FEFloat2sObject,  Float2sObject,  float2,     "{4ED63D9F-EEB4-4506-8A77-50BE2CD4529D}")           \
    X(FEFloat3sObject,  Float3sObject,  float3,     "{024FE907-6DA1-4A65-A7BD-6899D7CE98C5}")           \
    X(FEFloat4sObject,  Float4sObject,  float4,     "{6FE87B8C-F5EB-420D-8164-7D53842C9841}")           \
    X(FEQuatfsObject,   QuatfsObject,   quatf,      "{66E5B138-056B-4596-9689-D425682E5430}")           \
    X(FEBoolsObject,    BoolsObject,    uint8,      "{44C6E51E-DFF9-4F06-8E36-F9BFB06BF9B4}")           \
    X(FEColorObject,    RgbaObject,     uint8x4,    "{63370FBC-85AC-4784-B5DC-2D78AB9A69AF}")           \
    X(FEAlphaObject,    AlphaObject,    uint8,      "{9089721A-3877-4CA4-8ECD-79983A22603F}")           \
    X(FEIntsObject,     IntsObject,     int,        "{6BFCE40C-DD7E-4BC1-9A79-4EBC0D108983}")           \
    X(FEInt2sObject,    Int2sObject,    int2,       "{5D81178E-4BA0-4C35-9197-406EDD79EFF1}")           \
    X(FEInt3sObject,    Int3sObject,    int3,       "{18E77FDE-7233-4675-B24D-3B8F86C3111C}")           \
    X(FEInt4sObject,    Int4sObject,    int4,       "{615EC614-B01B-4CF6-98D8-C8A09D6D6E55}")           \
    X(FEUintsObject,    UintsObject,    uint,       "{CC7A473F-076A-4D9D-9946-49279D0629B0}")           \
    X(FEUint2sObject,   Uint2sObject,   uint2,      "{84024FFE-2DAD-4EF3-BF25-B0A570438E23}")           \
    X(FEUint3sObject,   Uint3sObject,   uint3,      "{4616B45E-7905-47DF-8A49-890C8EBDC608}")           \
    X(FEUint4sObject,   Uint4sObject,   uint4,      "{85F070F6-7361-4AE0-9694-E5275C33D38C}")

namespace FE
{
    #define FE_DECLARE_VALUE_ARRAY_OBJECT(ObjectClass, AliasName, ValueT, uuid) FE_DEFINE_VALUE_ARRAY_OBJECT(ObjectClass, AliasName, ValueT, uuid);

    FE_VALUE_ARRAY_OBJECT_LIST(FE_DECLARE_VALUE_ARRAY_OBJECT)

    #undef  FE_DECLARE_VALUE_ARRAY_OBJECT
}
