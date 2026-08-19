#pragma     once

#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
namespace   FE
{
    class   FEString
    {
    public:
        using   PCSTR   =   const char*;
    protected:
        char*   _buffer =   nullptr;
    public:
        FEString(const char* buffer = nullptr)
        {
            if (buffer)
            {
                size_t  len =   strlen(buffer);
                if (len > 0)
                {
                    _buffer =   new char[len + 1];
                    strcpy(_buffer,buffer);
                }
            }
        }
        FEString(const FEString& right)
        {
            if (right._buffer)
            {
                size_t  len =   strlen(right._buffer);
                if (len > 0)
                {
                    _buffer =   new char[len + 1];
                    strcpy(_buffer,right._buffer);
                }
            }
        }

        FEString(FEString&& right) noexcept
        {
            _buffer         =   right._buffer;
            right._buffer   =   nullptr;  
        }
        FEString(const std::string& right)
        {
            if (!right.empty())
            {
                _buffer =   new char[right.size() + 1];
                strcpy(_buffer,right.c_str());
            }
        }
        ~FEString()
        {
            delete  []_buffer;
        }
        /// <summary>
        /// 是否为空
        /// </summary>
        /// <returns></returns>
        inline  bool    empty() const
        {
            return  _buffer == nullptr || _buffer[0]=='\0';
        }
        /// <summary>
        /// 函数不会返回nullptr,如果是nullptr,则返回""
        /// </summary>
        /// <returns>值总是有效</returns>
        inline  PCSTR   c_str() const
        {
            return  _buffer == nullptr ? "" : _buffer;
        }
        /// <summary>
        /// 返回字符串长度
        /// </summary>
        /// <returns></returns>
        inline  size_t  size() const
        {
            if (_buffer == nullptr)
                return  0;
            else
                return  strlen(_buffer);
        }
        /// <summary>
        /// 数据不一定有效，如果没有初始化，返回值内部指针对象
        /// </summary>
        /// <returns>内部数据指针</returns>
        inline  char*   data()
        {
            return  _buffer;
        }
        /// <summary>
        /// 只读方式获取数据
        /// </summary>
        /// <returns></returns>
        inline  PCSTR   data() const
        {
            return  _buffer;
        }
        /// <summary>
        /// 重新分配内存大小，会用老的数据填充新缓冲区
        /// </summary>
        /// <param name="len"></param>
        inline  void    resize(size_t len)
        {
            char*   pStr    =   nullptr;
            if (len > 0)
                pStr    =   new char[len + 1];
            if (len)
                memset(pStr,0,len + 1);
            if (pStr && _buffer)
                strncpy(pStr,_buffer,std::min<size_t>(len,size()));
            delete  _buffer;
            _buffer =   pStr;
        }
        /// <summary>
        /// 分配内存,不会赋值内存，需要外部处理
        /// </summary>
        /// <param name="len"></param>
        inline  void    alloc(size_t len)
        {
            delete  []_buffer;
            _buffer         =   new char[len + 1];
            _buffer[len]    =   '\0';
        }
        /// <summary>
        /// 赋值函数
        /// </summary>
        /// <param name="right"></param>
        /// <returns></returns>
        inline  FEString& operator = (const char* right)
        {
            if (right == _buffer || right == nullptr)
                return  *this;
            /// 释放buffer
            delete  []_buffer;
            _buffer     =   nullptr;
            size_t  len =   strlen(right);
            if (len > 0)
            {
                _buffer =   new char[len + 1];
                strcpy(_buffer,right);
            }
            return  *this;
        }
        inline  FEString& operator = (const std::string& right)
        {
            return  (operator =(right.c_str()));
        }
        inline  FEString& operator = (const FEString& right)
        {
            return  (operator =(right.c_str()));
        }
        /// <summary>
        /// 是否相等
        /// </summary>
        /// <param name="pStr"></param>
        /// <returns></returns>
        inline  bool operator == (const char* pStr) const
        {
            /// pStr == _buffer 与自己比较 则是相等
            /// pStr == _buffer = nullptr 都是nullptr,则认为是相等
            if (pStr == _buffer)
                return  true;
            /// pStr != _buffer  pStr == nullptr
            /// 其中一个是nullptr,则认为不相等
            else if(pStr == nullptr)
                return  false;
            /// pStr != _buffer  _buffer == nullptr
            /// 其中一个是nullptr,则认为不相等
            else if(_buffer == nullptr)
                return  false;
            /// 数据有效，比较内容
            else
                return  strcmp(pStr,_buffer) == 0;
        }
        inline  bool operator == (const std::string& str) const
        {
            return  operator == (str.c_str());
        }
        inline  bool operator == (const FEString& str) const
        {
            return  operator == (str.c_str());
        }
        /// <summary>
        /// 不相等
        /// </summary>
        /// <param name="pStr"></param>
        /// <returns></returns>
        inline  bool operator != (const char* pStr) const
        {
            return  !(operator ==(pStr));
        }
        inline  bool operator != (const std::string& str) const
        {
            return  !(operator ==(str.c_str()));
        }
        inline  bool operator != (const FEString& str) const
        {
            return  !(operator ==(str.c_str()));
        }
        inline  void  clear()
        {
            delete  []_buffer;
            _buffer =   nullptr;
        }
    };
}
