#pragma     once
#include    "FEDefine.h"
namespace   FE
{
    class   FEMd5Key
    {
    public:
        FEMd5Key()
        {
            _uint64[0]      =   0ULL;
            _uint64[1]      =   0ULL;
        }
        FEMd5Key(unsigned buffer[4])
        {
            _bufferInt[0]   =   buffer[0];
            _bufferInt[1]   =   buffer[1];
            _bufferInt[2]   =   buffer[2];
            _bufferInt[3]   =   buffer[3];
        }
        FEMd5Key(const FEMd5Key& right)
        {
            _uint64[0]      =   right._uint64[0];
            _uint64[1]      =   right._uint64[1];
        }
        inline  bool    operator == (const FEMd5Key& right) const
        {
            return  _uint64[0] == right._uint64[0] && _uint64[1] == right._uint64[1] ;
        }
        inline  bool    operator != (const FEMd5Key& right) const
        {
            return  _uint64[0] != right._uint64[0] || _uint64[1] != right._uint64[1] ;
        }

        friend  bool    operator < (const FEMd5Key& left,const FEMd5Key& right)
        {
            if (left._uint64[0] != right._uint64[0])
                return  left._uint64[0] < right._uint64[0];
            else
                return  left._uint64[1] < right._uint64[1];
        }

        friend  bool    operator > (const FEMd5Key& left,const FEMd5Key& right)
        {
            if (left._uint64[0] != right._uint64[0])
                return  left._uint64[0] > right._uint64[0];
            else
                return  left._uint64[1] > right._uint64[1];
        }
        inline  char*   toString2(char buf[36]) const
        {
            sprintf(buf
                , "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"
                , _chBuf[0]
                , _chBuf[1]
                , _chBuf[2]
                , _chBuf[3] 
                , _chBuf[4] 
                , _chBuf[5] 
                , _chBuf[6]
                , _chBuf[7]
                , _chBuf[8]
                , _chBuf[9]
                , _chBuf[10]
                , _chBuf[11]
                , _chBuf[12]
                , _chBuf[13]
                , _chBuf[14]
                , _chBuf[15]
                );
            return  buf;
        }
        inline  bool    isEmpty() const
        {
            return  isNull();
        }
        inline  bool    isNull() const
        {
            return  _uint64[0] == 0ULL &&  _uint64[1] == 0ULL;
        }
        inline  auto    data() const
        {
            return  _bufferInt;
        }
        inline  void*   data()
        {
            return  _bufferInt;
        }
        
    public:
        static  FEMd5Key  from(const void* buffer)
        {
            FEMd5Key  id;
            memcpy(id.data(),buffer,sizeof(FEMd5Key));
            return      id;
        }
        union
        {
            uint32      _bufferInt[4];
            uint64      _uint64[2];
            uint8_t     _chBuf[16];
        };
    };

    using   MD5Key  =   FEMd5Key;

    class   FEMD5
    {
    protected:
        enum
        {
            S11 =   7,
            S12 =   12,
            S13 =   17,
            S14 =   22,
            S21 =   5,
            S22 =   9,
            S23 =   14,
            S24 =   20,
            S31 =   4,
            S32 =   11,
            S33 =   16,
            S34 =   23,
            S41 =   6,
            S42 =   10,
            S43 =   15,
            S44 =   21,
        };
        enum 
        {
            blocksize = 64
        }; 
    public:
        typedef unsigned char   uint1;      //  8bit
        typedef unsigned int    uint4;      // 32bit
    public:
        FEMD5()
        {
        }

        auto    md5(const char* pStr,size_t length)
        {
            init();
            update64((uint8*)pStr, length);
            finalize();
            return  FEMd5Key((unsigned*)_digest);
        }

	    void    init()
        {
            unsigned*   data   =   (unsigned*)_buffer;
            data[0 ]    =   0;
            data[1 ]    =   0;
            data[2 ]    =   0;
            data[3 ]    =   0;
            data[4 ]    =   0;
            data[5 ]    =   0;
            data[6 ]    =   0;
            data[7 ]    =   0;
            data[8 ]    =   0;
            data[9 ]    =   0;
            data[10]    =   0;
            data[11]    =   0;
            data[12]    =   0;
            data[13]    =   0;
            data[14]    =   0;
            data[15]    =   0;

            _count[0]   =   0;
            _count[1]   =   0;

            _state[0]   =   0x67452301;
            _state[1]   =   0xefcdab89;
            _state[2]   =   0x98badcfe;
            _state[3]   =   0x10325476;
        }
	
	    void    update(const unsigned char input[], unsigned length)
        {
            // compute number of bytes mod 64
            unsigned index = _count[0] / 8 % blocksize;

            // Update number of bits
            if ((_count[0] += (length << 3)) < (length << 3))
                _count[1]++;
            _count[1] += (length >> 29);

            // number of bytes we need to fill in buffer
            unsigned firstpart = 64 - index;

            unsigned i;

            // transform as many times as possible.
            if (length >= firstpart)
            {
                // fill buffer first, transform
                memcpy(&_buffer[index], input, firstpart);
                transform(_buffer);
                // transform chunks of blocksize (64 bytes)
                for (i = firstpart; i + blocksize <= length; i += blocksize)
                {
                    transform(&input[i]);
                }
                index = 0;
            }
            else
            {
                i = 0;
            }
            memcpy(&_buffer[index], &input[i], length-i);
        }

        void    update(const char input[], unsigned length)
        {
            update((const unsigned char*)input, length);
        }
        void    update(const void* input, unsigned length)
        {
            update((const unsigned char*)input, length);
        }
        void    update64(const unsigned char input[], size_t length)
        {
            /// 支持内存超过4G的情况
            /// 每次最大1G内存计算
            constexpr   uint  BLOCK   =   1024 * 1024 * 1024;
            /// 块数
            size_t      nBlock  =   length/BLOCK;
            /// 剩余字节
            size_t      nLast   =   length - nBlock * BLOCK;
            uint8*      pBuf    =   (uint8*)input;
            for (size_t i = 0; i < nBlock; i++)
                update(pBuf + i * BLOCK,BLOCK);
            if(nLast != 0)
                update(pBuf + nBlock * BLOCK,(uint)nLast);
        }
        FEMD5&  finalize()
        {
            static unsigned char padding[64] = 
            {
                0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            };
            unsigned char bits[8];
            encode(bits, _count, 8);
            unsigned index  =   _count[0] / 8 % 64;
            unsigned padLen =   (index < 56) ? (56 - index) : (120 - index);
            update(padding, padLen);
            update(bits, 8);
            encode(_digest, _state, 16);
            return *this;
        }

	    auto    getKey()
        {
            return  FEMd5Key((unsigned*)_digest);
        }
    public:
        /// <summary>
        /// 根据字符串产生key
        /// </summary>
        /// <param name="source"></param>
        /// <returns></returns>
        static  FEMd5Key  fromString(const char* source)
        {
            return  fromBuffer(source,(unsigned)strlen(source));
        }
        /// <summary>
        /// 根据缓冲区产生key
        /// </summary>
        /// <param name="buffer"></param>
        /// <param name="length"></param>
        /// <returns></returns>
        static  FEMd5Key  fromBuffer(const void* buffer,unsigned length)
        {
            FEMD5   sInstance;
            return  sInstance.md5((const char*)buffer,length);
        }
        /// <summary>
        /// 计算文件的MD5;
        /// </summary>
        /// <param name="fileName"></param>
        /// <returns></returns>
        static  FEMd5Key  fromFile(const char* fileName)
        {
            FILE*   pFile   =   fopen(fileName,"rb");
            if (pFile == nullptr)
                return  FEMd5Key();
            /// 每次读取的大小
            char    szTemp[4096]    =   {};
            /// 获取文件大小
            _fseeki64(pFile,0,  SEEK_END);
            uint64_t    nFile   =   _ftelli64(pFile);
            _fseeki64(pFile,0,  SEEK_SET);

            uint64_t    nBlock  =   nFile/sizeof(szTemp);
            uint64_t    nLast   =   nFile - nBlock * sizeof(szTemp);

            FEMD5   md5;
            md5.init();
        
            for (size_t i = 0; i < nBlock; i++)
            {
                fread(szTemp,1,sizeof(szTemp),pFile);
                md5.update(szTemp, sizeof(szTemp));
            }
            if (nLast > 0)
            {
                fread(szTemp,1,nLast,pFile);
                md5.update(szTemp, sizeof(szTemp));
            }
            fclose(pFile);

            md5.finalize();
            return  md5.getKey();
        }
    protected:
        static  inline uint4 F(uint4 x, uint4 y, uint4 z) {return (x & y) | ((~x) & z);}

        static  inline uint4 G(uint4 x, uint4 y, uint4 z) {return (x&z) | (y&~z);}

        static  inline uint4 H(uint4 x, uint4 y, uint4 z) {return x^y^z;}

        static  inline uint4 I(uint4 x, uint4 y, uint4 z) {return y ^ (x | ~z);}

        static  inline uint4 rotate_left(uint4 x, int n) { return (x << n) | (x >> (32-n));}
    
        static  inline void FF(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) { a = rotate_left(a+ F(b,c,d) + x + ac, s) + b;}

        static  inline void GG(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) { a = rotate_left(a + G(b,c,d) + x + ac, s) + b;}

        static  inline void HH(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) { a = rotate_left(a + H(b,c,d) + x + ac, s) + b;}

        static  inline void II(uint4 &a, uint4 b, uint4 c, uint4 d, uint4 x, uint4 s, uint4 ac) {a = rotate_left(a + I(b,c,d) + x + ac, s) + b;}

    protected:
        void    decode(uint4 output[], const uint1 input[], unsigned len)
        {
            for (unsigned i = 0, j = 0; j < len; i++, j += 4)
                output[i] = ((unsigned)input[j]) | (((unsigned)input[j+1]) << 8) |(((unsigned)input[j+2]) << 16) | (((unsigned)input[j+3]) << 24);
        }
        void    encode(uint1 output[], const unsigned input[], unsigned len)
        {
            for (unsigned i = 0, j = 0; j < len; i++, j += 4) 
            {
                output[j]   = input[i] & 0xff;
                output[j+1] = (input[i] >> 8) & 0xff;
                output[j+2] = (input[i] >> 16) & 0xff;
                output[j+3] = (input[i] >> 24) & 0xff;
            }
        }

        void    transform(const uint1 block[blocksize])
        {
            uint4   a = _state[0];
            uint4   b = _state[1];
            uint4   c = _state[2];
            uint4   d = _state[3];
            uint4   x[16];
            decode (x, block, blocksize);

            /* Round 1 */
            FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
            FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
            FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
            FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
            FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
            FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
            FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
            FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
            FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
            FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
            FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
            FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
            FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
            FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
            FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
            FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

            /* Round 2 */
            GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
            GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
            GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
            GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
            GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
            GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
            GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
            GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
            GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
            GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
            GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
            GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
            GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
            GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
            GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
            GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

            /* Round 3 */
            HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
            HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
            HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
            HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
            HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
            HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
            HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
            HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
            HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
            HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
            HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
            HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
            HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
            HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
            HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
            HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

            /* Round 4 */
            II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
            II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
            II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
            II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
            II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
            II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
            II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
            II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
            II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
            II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
            II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
            II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
            II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
            II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
            II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
            II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

            _state[0]    +=  a;
            _state[1]    +=  b;
            _state[2]    +=  c;
            _state[3]    +=  d;
        }

    protected:
        uint1   _buffer[blocksize];
        uint4   _count[2];   // 64bit counter for number of bits (lo, hi)
        uint4   _state[4];   // digest so far
        uint1   _digest[16]; 
    
    };
}

namespace   std
{
    template<>
    class   hash<FE::FEMd5Key>
    {
    public:
        uint64_t operator()(const FE::FEMd5Key& key) const noexcept
        {
            constexpr uint64_t _FNV_offset_basis = 14695981039346656037ULL;
            constexpr uint64_t _FNV_prime        = 1099511628211ULL;
            uint64_t  val    =   _FNV_offset_basis;
            for (uint64_t i = 0; i < 4; ++i)
            {
                val ^= static_cast<uint64_t>(key._bufferInt[i]);
                val *= _FNV_prime;
            }
            return val;
        }
    };
}
