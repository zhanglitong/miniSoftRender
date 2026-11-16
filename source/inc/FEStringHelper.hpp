#pragma     once

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    <unordered_map>
#include    "FEDefine.h"
#include    "FEGBKTable.hpp"

namespace   FE
{
    using   WString     =    std::wstring;
    using   String      =    std::string;
    using   Strings     =    std::vector<String>;
    using   CodeMap     =    std::unordered_map<uint32, uint16>;
    using   CCodeMap    =    const std::unordered_map<uint32, uint16>;
    /// <summary>
    /// 字符串帮助类，提供字符串操作的静态方法
    /// </summary>
    class    FEStringHelper
    {
    public:
        static    Strings   split(const String& str, const char delim = ',')
        {
            if (str.empty())
                return    {};
            Strings result;
            size_t    si    =    0;
            size_t    ei    =    str.find(delim, si);
            if (ei == String::npos) 
            {
                return {str};
            }
            while (ei != String::npos) 
            {
                result.push_back(str.substr(si, ei - si));
                si = ei + 1;
                ei = str.find(delim, si);
            }

            if (si < str.length()) 
            {
                result.push_back(str.substr(si));
            }
            return result;
        }
        /// <summary>
        /// 将 UTF-8 string 解码为 Unicode（Windows 下 WString 存放 UTF-16）
        /// </summary>
        /// <param name="str">UTF-8 字符串</param>
        /// <returns>Unicode 字符串</returns>
        static    WString   utf8ToWide(const String& str)
        {
            WString    out;
            out.reserve(str.size());

            for (size_t i = 0; i < str.size();)
            {
                const uint8        c    =    static_cast<uint8>(str[i]);
                uint32    cp    =    0;
                size_t    len    =    0;

                if (c <= 0x7F)                { cp = c;            len = 1; }
                else if ((c & 0xE0) == 0xC0)    { cp = c & 0x1F;    len = 2; }
                else if ((c & 0xF0) == 0xE0)    { cp = c & 0x0F;    len = 3; }
                else if ((c & 0xF8) == 0xF0)    { cp = c & 0x07;    len = 4; }
                else
                {
                    ++i;    // 非法首字节，跳过
                    continue;
                }

                if (i + len > str.size())
                {
                    ++i;    // 序列被截断，跳过
                    continue;
                }

                bool    valid    =    true;
                for (size_t k = 1; k < len; ++k)
                {
                    const uint8    cc    =    static_cast<uint8>(str[i + k]);
                    if ((cc & 0xC0) != 0x80)
                    {
                        valid    =    false;
                        break;
                    }
                    cp    =    (cp << 6) | (cc & 0x3F);
                }

                // 非法续字节 / 过长编码 / 越界 / 代理区码点均视为非法
                if (!valid || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)
                    || (len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000))
                {
                    ++i;
                    continue;
                }

                if (sizeof(wchar_t) == 2 && cp > 0xFFFF)
                {
                    // UTF-16：编码为代理对
                    cp    -=    0x10000;
                    out.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
                    out.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
                }
                else
                {
                    out.push_back(static_cast<wchar_t>(cp));
                }
                i    +=    len;
            }
            return out;
        }
        /// <summary>
        /// 将 wstring 转换为 UTF-8 string
        /// wchar_t 在 Windows 下为 2 字节（UTF-16），在 Linux/macOS 下为 4 字节（UTF-32）
        /// </summary>
        /// <param name="wstr">Unicode 字符串</param>
        /// <returns>UTF-8 字符串</returns>
        static    String    wideToUTF8(const WString& wstr)
        {
            String    out;
            out.reserve(wstr.size() * 2);

            for (size_t i = 0; i < wstr.size(); ++i)
            {
                uint32    cp    =    static_cast<uint32>(wstr[i]);

                if constexpr(sizeof(wchar_t) == 2)
                {
                    // UTF-16：处理代理对（surrogate pair）
                    if (cp >= 0xD800 && cp <= 0xDBFF)
                    {
                        // 高位代理，需要与下一个低位代理组合
                        if (i + 1 < wstr.size())
                        {
                            uint32    low    =    static_cast<uint32>(wstr[i + 1]);
                            if (low >= 0xDC00 && low <= 0xDFFF)
                            {
                                cp    =    0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                                ++i;    // 消耗了下一个 wchar_t
                            }
                            else
                            {
                                continue;    // 非法代理对，跳过
                            }
                        }
                        else
                        {
                            continue;    // 孤立的高位代理，跳过
                        }
                    }
                    else if (cp >= 0xDC00 && cp <= 0xDFFF)
                    {
                        continue;    // 孤立的低位代理，跳过
                    }
                }
                // sizeof(wchar_t) == 4 时，cp 直接就是 Unicode 码点

                appendUTF8(cp, out);
            }
            return out;
        }
        /// <summary>
        /// 将 GBK 字符串解码为 Unicode，非法字节替换为 U+FFFD
        /// </summary>
        /// <param name="str">GBK 字符串</param>
        /// <returns>Unicode 字符串</returns>
        static    WString   gbkToWide(const String& str)
        {
            WString     out;
            out.reserve(str.size());

            for (size_t i = 0; i < str.size(); ++i)
            {
                const uint8    c    =    static_cast<uint8>(str[i]);
                if (c < 0x80)
                {
                    out.push_back(static_cast<wchar_t>(c));
                    continue;
                }

                bool    decoded    =    false;
                if (c >= GBKLeadFirst && i + 1 < str.size())
                {
                    const uint8    t    =    static_cast<uint8>(str[i + 1]);
                    if (t >= GBKTrailFirst && t <= 0xFE && t != 0x7F)
                    {
                        const uint32    index    =    static_cast<uint32>(c - GBKLeadFirst) * GBKTrailCount
                                                +    static_cast<uint32>(t - GBKTrailFirst - (t > 0x7F ? 1 : 0));
                        const uint16    uc    =    GBKToUnicodeTable[index];
                        if (uc != 0)
                        {
                            out.push_back(static_cast<wchar_t>(uc));
                            ++i;
                            decoded    =    true;
                        }
                    }
                }
                if (!decoded)
                    out.push_back(static_cast<wchar_t>(0xFFFD));    // 非法字节，替换为 U+FFFD
            }
            return out;
        }
        /// <summary>
        /// 将 Unicode 字符串编码为 GBK，无法表示的字符替换为 '?'
        /// </summary>
        /// <param name="wstr">Unicode 字符串</param>
        /// <returns>GBK 字符串</returns>
        static    String    wideToGBK(const WString& wstr)
        {
            String    out;
            out.reserve(wstr.size() * 2);

            for (size_t i = 0; i < wstr.size(); ++i)
            {
                uint32    cp    =    static_cast<uint32>(wstr[i]);
                if (sizeof(wchar_t) == 2 && cp >= 0xD800 && cp <= 0xDBFF && i + 1 < wstr.size())
                {
                    const uint32    low    =    static_cast<uint32>(wstr[i + 1]);
                    if (low >= 0xDC00 && low <= 0xDFFF)
                    {
                        cp    =    0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        ++i;
                    }
                }

                if (cp < 0x80)
                {
                    out.push_back(static_cast<char>(cp));
                    continue;
                }

                uint16    gbk    =    0;
                if (cp <= 0xFFFF)
                {
                    const auto&    table    =    gbkEncodeTable();
                    const auto    it    =    table.find(cp);
                    if (it != table.end())
                        gbk    =    it->second;
                }

                if (gbk != 0)
                {
                    out.push_back(static_cast<char>(gbk >> 8));
                    out.push_back(static_cast<char>(gbk & 0xFF));
                }
                else
                {
                    out.push_back('?');    // GBK 无法表示的字符，替换为默认字符
                }
            }
            return out;
        }
        /// <summary>
        /// 将 UTF-8 字符串转换为 GBK
        /// </summary>
        /// <param name="str">UTF-8 字符串</param>
        /// <returns>GBK 字符串</returns>
        static    String    utf8ToGBK(const String& str)
        {
            if (str.empty())
                return {};
            return    wideToGBK(utf8ToWide(str));
        }
        /// <summary>
        /// 将 GBK 字符串转换为 UTF-8
        /// </summary>
        /// <param name="str">GBK 字符串</param>
        /// <returns>UTF-8 字符串</returns>
        static    String    gbkToUTF8(const String& str)
        {
            if (str.empty())
                return {};
            return    wideToUTF8(gbkToWide(str));
        }
    public:
        /// <summary>
        /// 将单个 UTF-8 字符序列解码为 Unicode 码点
        /// </summary>
        /// <param name="inCp">UTF-8 字节序列（至少可读 6 字节）</param>
        /// <param name="outUc">输出的 Unicode 码点</param>
        /// <returns>消耗的字节数（1~6）；遇到非法续字节时只消耗 1 字节</returns>
        static    size_t    _utf8_to_utf32(const uint8 inCp[6], uint32& outUc)
        {
            const uint8 lead = inCp[0];
            size_t len = 1;
            if (!(lead & 0x80))                 len = 1;
            else if ((lead & 0xE0) == 0xC0)     len = 2;
            else if ((lead & 0xF0) == 0xE0)     len = 3;
            else if ((lead & 0xF8) == 0xF0)     len = 4;
            else if ((lead & 0xFC) == 0xF8)     len = 5;
            else if ((lead & 0xFE) == 0xFC)     len = 6;

            if (len == 1)
            {
                outUc = lead;
                return 1;
            }

            uint32 code = 0;
            switch (len)
            {
            case 2:     code = lead & 0x1F; break;
            case 3:     code = lead & 0x0F; break;
            case 4:     code = lead & 0x07; break;
            case 5:     code = lead & 0x03; break;
            default:    code = lead & 0x01; break;
            }

            for (size_t i = 1; i < len; ++i)
            {
                if ((inCp[i] & 0xC0) != 0x80)
                {
                    outUc = lead;   // 非法续字节，只消耗首字节
                    return 1;
                }
                code = (code << 6) | (inCp[i] & 0x3F);
            }

            outUc = code;
            return len;
        }
        /// <summary>
        /// 将单个 Unicode 码点编码为 UTF-8 字符序列
        /// </summary>
        /// <param name="inUc">输入的 Unicode 码点</param>
        /// <param name="outCp">输出的 UTF-8 字节序列（至少可写 6 字节）</param>
        /// <returns>写入的字节数（1~6）</returns>
        static    size_t    _utf32_to_utf8(const uint32& inUc, uint8 outCp[6])
        {
            size_t len = 1;
            if (inUc <= 0x7F)               len = 1;
            else if (inUc <= 0x7FF)         len = 2;
            else if (inUc <= 0xFFFF)        len = 3;
            else if (inUc <= 0x1FFFFF)      len = 4;
            else if (inUc <= 0x3FFFFFF)     len = 5;
            else if (inUc <= 0x7FFFFFFF)    len = 6;

            uint32 code = inUc;
            for (size_t i = len - 1; i > 0; --i)
            {
                outCp[i] = static_cast<uint8>(0x80 | (code & 0x3F));
                code >>= 6;
            }

            switch (len)
            {
            case 6:     outCp[0] = static_cast<uint8>(0xFC | (code & 0x01)); break;
            case 5:     outCp[0] = static_cast<uint8>(0xF8 | (code & 0x03)); break;
            case 4:     outCp[0] = static_cast<uint8>(0xF0 | (code & 0x07)); break;
            case 3:     outCp[0] = static_cast<uint8>(0xE0 | (code & 0x0F)); break;
            case 2:     outCp[0] = static_cast<uint8>(0xC0 | (code & 0x1F)); break;
            default:    outCp[0] = static_cast<uint8>(code & 0x7F);          break;
            }
            return len;
        }
        /// <summary>
        /// 将单个 UTF-16 字符序列解码为 Unicode 码点
        /// </summary>
        /// <param name="inCp">UTF-16 码元序列（至少可读 2 个码元）</param>
        /// <param name="outUc">输出的 Unicode 码点</param>
        /// <returns>消耗的码元数（2 为代理对，否则为 1）</returns>
        static    size_t    _utf16_to_utf32(const uint16 inCp[2], uint32& outUc)
        {
            const uint32 high = inCp[0];
            const uint32 low = inCp[1];
            if (high >= 0xD800 && high <= 0xDBFF && low >= 0xDC00 && low <= 0xDFFF)
            {
                outUc = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                return 2;
            }
            outUc = high;   // 非代理对，直接返回首码元
            return 1;
        }
        /// <summary>
        /// 将单个 Unicode 码点编码为 UTF-16 字符序列
        /// </summary>
        /// <param name="inUc">输入的 Unicode 码点</param>
        /// <param name="outCp">输出的 UTF-16 码元序列（至少可写 2 个码元）</param>
        /// <returns>写入的码元数（2 为代理对，否则为 1）</returns>
        static    size_t    _utf32_to_utf16(const uint32& inUc, uint16 outCp[2])
        {
            if (inUc <= 0xFFFF)
            {
                outCp[0] = static_cast<uint16>(inUc);
                return 1;
            }

            const uint32 code = inUc - 0x10000;
            outCp[0] = static_cast<uint16>(0xD800 + (code >> 10));
            outCp[1] = static_cast<uint16>(0xDC00 + (code & 0x3FF));
            return 2;
        }
        /// <summary>
        /// 将单个 Unicode 码点编码为 UTF-8，追加到 out
        /// </summary>
        /// <param name="cp">Unicode 码点</param>
        /// <param name="out">输出的 UTF-8 字符串</param>
        static    void      appendUTF8(uint32 cp, String& out)
        {
            if (cp <= 0x7F)
            {
                out.push_back(static_cast<char>(cp));
            }
            else if (cp <= 0x7FF)
            {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0xFFFF)
            {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0x10FFFF)
            {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        /// <summary>
        /// 由 GBK 映射表索引还原出双字节编码
        /// </summary>
        /// <param name="index">映射表中的索引</param>
        /// <returns>GBK 双字节编码</returns>
        static    uint16    gbkIndexToCode(uint32 index)
        {
            const uint32    lead    =    GBKLeadFirst + index / GBKTrailCount;
            const uint32    trail    =    index % GBKTrailCount;
            // 尾字节跳过非法编码 0x7F
            const uint32    code    =    GBKTrailFirst + trail + (trail >= 0x7F - GBKTrailFirst ? 1 : 0);
            return    static_cast<uint16>((lead << 8) | code);
        }
        /// <summary>
        /// Unicode -> GBK 反查表，首次使用时由正向映射表构建
        /// </summary>
        /// <returns>Unicode 到 GBK 的映射表</returns>
        static    CCodeMap& gbkEncodeTable()
        {
            static const CodeMap    table   =   []()
            {
                CodeMap result;
                result.reserve(GBKTableSize);
                for (uint32 i = 0; i < GBKTableSize; ++i)
                {
                    const uint16    uc    =    GBKToUnicodeTable[i];
                    if (uc != 0)
                        result.emplace(uc, gbkIndexToCode(i));
                }
                return result;
            }();
            return  table;
        }
    };
}
