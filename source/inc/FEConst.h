#pragma     once
#include    <cfloat>
namespace   FE
{
    static  constexpr   float       oneOver127F     =   0.007874015748031f; //  1.0f/127.f;
    static  constexpr   float       oneOver128F     =   0.0078125;          //  1.0f/128.f;
    static  constexpr   float       oneOver255F     =   0.0039215686274509f;//  1.0f/255.f;
    static  constexpr   float       oneOver256F     =   0.00390625;         //  1.0f/256.f;
    static  constexpr   float       oneOver511F     =   0.0019569471624266f;//  1.0f/511.f;
    static  constexpr   float       oneOver512F     =   0.001953125f;       //  1.0f/512.f;

    static  constexpr   float       oneOver1023F    =   float(1.0/1023.0);  //  1.0f/1023.f
    static  constexpr   float       oneOver1024F    =   float(1.0/1024.0);  //  1.0f/1024.f;
    static  constexpr   float       oneOver2048F    =   float(1.0/2048.0);  //  1.0f/2048.f;
    static  constexpr   float       oneOver4096F    =   float(1.0/4096.0);  //  1.0f/4096.f;
    static  constexpr   float       oneOver8192F    =   float(1.0/8192.0);  //  1.0f/4096.f;
    static  constexpr   float       oneOver32767F   =   float(1.0/32767.0); //  1.0f/32767.0f;
    static  constexpr   float       oneOver65535F   =   float(1.0/65535.0); //  1.0f/65535.0f;

    static  constexpr   float       _127F           =   127.0f;
    static  constexpr   float       _128F           =   128.0f;
    static  constexpr   float       _255F           =   255.0f;
    static  constexpr   float       _256F           =   256.0f;
    static  constexpr   float       _511F           =   511.0f;
    static  constexpr   float       _512F           =   512.0f;
    static  constexpr   float       _1023F          =   1023.0f;
    static  constexpr   float       _1024F          =   1024.0f;
    static  constexpr   float       _2047F          =   2047.0f;
    static  constexpr   float       _2048F          =   2048.0f;
    static  constexpr   float       _4096F          =   4096.0f;
    static  constexpr   float       _8192F          =   8192.0f;
    static  constexpr   float       _32767F         =   32767.f;
    static  constexpr   float       _65535F         =   65535.f;

    static  constexpr   uint16_t    _127u16         =   127;
    static  constexpr   uint16_t    _128u16         =   128;
    static  constexpr   uint16_t    _255u16         =   255;
    static  constexpr   uint16_t    _256u16         =   256;
    static  constexpr   uint16_t    _257u16         =   257;
    static  constexpr   uint16_t    _511u16         =   511;
    static  constexpr   uint16_t    _512u16         =   512;
    static  constexpr   uint16_t    _1023u16        =   1023;
    static  constexpr   uint16_t    _1024u16        =   1024;
    static  constexpr   uint16_t    _2047u16        =   2047;
    static  constexpr   uint16_t    _2048u16        =   2048;

    static  constexpr   uint32_t    _1k             =   1024;
    static  constexpr   uint32_t    _2k             =   (_1k)<<1;
    static  constexpr   uint32_t    _4k             =   (_1k)<<2;
    static  constexpr   uint32_t    _8k             =   (_1k)<<3;
    static  constexpr   uint32_t    _16k            =   (_1k)<<4;
    static  constexpr   uint32_t    _32k            =   (_1k)<<5;
    static  constexpr   uint32_t    _64k            =   (_1k)<<6;
    static  constexpr   uint32_t    _128k           =   (_1k)<<7;
    static  constexpr   uint32_t    _256k           =   (_1k)<<8;
    static  constexpr   uint32_t    _512k           =   (_1k)<<9;
    static  constexpr   uint32_t    _1M             =   (_1k)<<10;
    static  constexpr   uint32_t    _2M             =   (_1M)<<1;
    static  constexpr   uint32_t    _4M             =   (_1M)<<2;
    static  constexpr   uint32_t    _8M             =   (_1M)<<3;
    static  constexpr   uint32_t    _16M            =   (_1M)<<4;
    static  constexpr   uint32_t    _32M            =   (_1M)<<5;
    static  constexpr   uint32_t    _64M            =   (_1M)<<6;
    static  constexpr   uint32_t    _1281M          =   (_1M)<<7;
    static  constexpr   uint32_t    _256M           =   (_1M)<<8;
    static  constexpr   uint32_t    _512M           =   (_1M)<<9;
    static  constexpr   uint32_t    _1G             =   (_1M)<<10;
    static  constexpr   uint32_t    _2G             =   (_1G)<<1;


    static constexpr    double      PI                      =   3.14159265358979323846;
    static constexpr    double      TWO_PI                  =   PI * 2;
    static constexpr    double      HALF_PI                 =   PI * 0.5;
    static constexpr    double      PI_OVER_4               =   PI * 0.25;
    static constexpr    double      PI_OVER_180             =   PI / 180.0;

    static constexpr    double      RAD2DEG_FACTOR          =   57.29577951308232;

    static constexpr    double      EARTH_PERIMETER         =   (2.0 * PI * 6378137.0);
    static constexpr    double      EARTH_HALF_PERIMETER    =   (PI * 6378137.0);

    static constexpr    double      WGS_84_RADIUS_EQUATOR   =   6378137;
    static constexpr    double      WGS_84_RADIUS_POLAR     =   6356752.3142;

    #define     DEG2RAD(degree)         (PI_OVER_180 * (degree))
    #define     RAD2DEG(rad)            (RAD2DEG_FACTOR * (rad))

    #define     FE_NAME_LENGTH          64
    #define     FE_TYPE_LENGTH          64
    #define     FE_GUID_LENGTH          64
    #define     FE_PATH_LENGTH          1024

    #ifndef     OFFSET_STRUCT
    #define     OFFSET_STRUCT(stru, mem)    ((char*)(&((structure*)0)->member))
    #endif

    #define     MAKE_UINT4(r,g,b,a)         ((uint32_t)(((unsigned char)(a)|((uint16_t)((unsigned char)(b))<<8))|(((uint32_t)(unsigned char)(g))<<16)) | (((uint32_t)(unsigned char)(r))<<24))
    #define     MAKE_VERSION(v0,v1,v2,v3)   MAKE_UINT4(v0,v1,v2,v3)
    #define     V0_0_0_0                    MAKE_VERSION(0,0,0,0)
    #define     V1_0_0_0                    MAKE_VERSION(1,0,0,0)

  
}


