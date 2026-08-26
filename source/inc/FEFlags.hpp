#pragma     once
#include    <initializer_list>
#include    <cstddef>

namespace   FE
{
    /// <summary>
    /// EnumType:枚举类型
    /// DataType:保存标志的数据类型(通常为int)
    /// eg:
    ///     enum MyFlag
    ///     {
    ///         MF_value0 = 1 << 0,
    ///         MF_value1 = 1 << 1,
    ///         MF_value2 = 1 << 2,
    ///     }
    ///     using MyFlags = FEFlags<MyFlag, int>;
    /// </summary>
    /// <typeparam name="EnumType"></typeparam>
    /// <typeparam name="DataType"></typeparam>
    template<class EnumType, class DataType>
    class   FEFlags
    {
    public:        
        using EnumFlags = std::initializer_list<EnumType>;
        using DataFlags = std::initializer_list<DataType>;
    private:
        DataType _flags;
    public:
        inline FEFlags() 
        {
            _flags = DataType(0);
        }
        inline FEFlags(EnumType flag)
        {
            _flags = (DataType)flag;
        }
        inline FEFlags(DataType flag)
        {
            _flags = flag;
        }
        inline FEFlags(const FEFlags& other)
        {
            _flags = other._flags;
        }
        FEFlags(const EnumFlags& flags)
        {
            _flags = DataType(0);
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                _flags |= (*itr);
            }
        }
        FEFlags(const DataFlags& flags)
        {
            _flags = DataType(0);
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                _flags |= (*itr);
            }
        }
        inline auto&    operator=(const FEFlags& other)
        {
            _flags = other._flags;
            return *this;
        }
        inline ~FEFlags() 
        {}
    public:
        /// <summary>
        /// 获取 flags
        /// </summary>
        /// <returns></returns>
        inline  auto    data() const
        {
            return _flags;
        }
        /// <summary>
        /// 添加flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   addFlag(EnumType flag)
        {
            _flags |= flag;
            return *this;
        }
        /// <summary>
        /// 添加flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   addFlag(DataType flag)
        {
            _flags |= flag;
            return *this;
        }
        inline  auto&   addFlags(DataType flag)
        {
            _flags |= flag;
            return *this;
        }
        /// <summary>
        /// 移除flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   removeFlag(EnumType flag)
        {
            _flags &= ~flag;
            return *this;
        }
        /// <summary>
        /// 移除flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   removeFlag(DataType flag)
        {
            _flags &= ~flag;
            return *this;
        }
        /// <summary>
        /// 设置标记
        /// </summary>
        /// <param name="flag"></param>
        /// <param name="bOn">bOn 如果为true，添加flag，否则移除flag</param>
        /// <returns></returns>
        inline  auto&   setFlag(EnumType flag, bool bOn = true)
        {
            if (bOn)
                return addFlag(flag);
            else
                return removeFlag(flag);
        }
        /// <summary>
        /// bOn 如果为true，添加flag，否则移除flag
        /// </summary>
        /// <param name="flag"></param>
        /// <param name="bOn"></param>
        /// <returns></returns>
        inline  auto    setFlag(DataType flag, bool bOn = true)
        {
            if (bOn)
                return addFlag(flag);
            else
                return removeFlag(flag);
        }
        /// <summary>
        /// 是否包含某个 flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    hasFlag(EnumType flag) const
        {
            return  (_flags & flag) == DataType(flag);
        }
        /// <summary>
        /// 是否包含某个 flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    hasFlag(DataType flag) const
        {
            return  (_flags & flag) == DataType(flag);
        }
        /// <summary>
        /// 是否包含任意一个 flag
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    containFlag(DataType flag) const
        {
            return  (_flags & flag) != 0;
        }
        /// <summary>
        /// 添加 flags
        /// </summary>
        /// <param name="flags"></param>
        /// <returns></returns>
        inline  auto&   addFlags(const EnumFlags& flags)
        {
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                this->addFlag(*itr);
            }
            return *this;
        }
        /// <summary>
        /// 添加 flags
        /// </summary>
        /// <param name="flags"></param>
        /// <returns></returns>
        inline  auto&   addFlags(const DataFlags& flags)
        {
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                this->addFlag(*itr);
            }
            return *this;
        }
        /// <summary>
        /// 移除flags
        /// </summary>
        /// <param name="flags"></param>
        /// <returns></returns>
        inline  auto&   removeFlags(const EnumFlags& flags)
        {
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                this->removeFlag(*itr);
            }
            return *this;
        }
        /// <summary>
        /// 移除flags
        /// </summary>
        /// <param name="flags"></param>
        /// <returns></returns>
        inline  auto&   removeFlags(const DataFlags& flags)
        {
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                this->removeFlag(*itr);
            }
            return *this;
        }
        inline  auto&   removeFlags(DataType flags)
        {
            _flags &= ~flags;
            return *this;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="flags"></param>
        /// <param name="bOn">bOn 如果为true，添加flag，否则移除flag</param>
        /// <returns></returns>
        inline  auto&   setFlags(EnumFlags flags, bool bOn = true)
        {
            if (bOn)
                return addFlags(flags);
            else
                return removeFlags(flags);
        }
        /// <summary>
        /// bOn 如果为true，添加flag，否则移除flag
        /// </summary>
        /// <param name="flags"></param>
        /// <param name="bOn"></param>
        /// <returns></returns>
        inline  auto&   setFlags(DataFlags flags, bool bOn = true)
        {
            if (bOn)
                return addFlags(flags);
            else
                return removeFlags(flags);
        }
        inline  bool   hasFlags(DataType flags)
        {
            return  (_flags & flags) ? true : false;
        }
        /// <summary>
        /// 所有的 flag 均被包含时返回 true, 否则返回false
        /// </summary>
        /// <param name="flags"></param>
        /// <returns></returns>
        inline  bool    hasFlags(const EnumFlags& flags) const
        {
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                if (!this->hasFlag(*itr))
                    return false;
            }
            return true;
        }
        /// <summary>
        /// 是否包含 flags
        /// 所有的 flag 均被包含时返回 true, 否则返回false
        /// </summary>
        /// <param name="flags"></param>
        /// <returns></returns>
        inline  bool    hasFlags(const DataFlags& flags) const
        {
            for (auto itr = flags.begin(); itr != flags.end(); ++itr)
            {
                if (!this->hasFlag(*itr))
                    return false;
            }
            return true;
        }
        /// <summary>
        /// 获取已设置标记位的个数
        /// </summary>
        /// <returns></returns>
        inline  size_t  flagCount() const
        {
            size_t dataSize = sizeof(DataType) * 8;
            size_t flagCount = 0;
            for (size_t i = 0; i < dataSize; ++i)
            {
                DataType d = 1 << i;
                if (hasFlag(d))
                    flagCount++;
            }
            return flagCount;
        }
        /// <summary>
        /// 定义到bool的隐式转换
        /// </summary>
        /// <returns></returns>
        inline  bool    operator !() const
        {
            return _flags == DataType(0);
        }
        /// <summary>
        /// ~ 运算
        /// </summary>
        /// <returns></returns>
        inline  auto    operator~() const
        {
            return FEFlags<EnumType, DataType>(~_flags);
        }
        /// <summary>
        /// &运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  FEFlags operator&(EnumType flag) const
        {
            return FEFlags(this->_flags & flag);
        }
        /// <summary>
        /// & 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  FEFlags operator&(DataType flag) const
        {
            return FEFlags(this->_flags & flag);
        }
        /// <summary>
        /// & 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  FEFlags operator&(const FEFlags& other) const
        {
            return FEFlags(this->_flags & other._flags);
        }
        /// <summary>
        /// &= 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   operator&=(EnumType flag)
        {
            this->_flags &= flag;
            return *this;
        }
        /// <summary>
        /// &= 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   operator&=(DataType flag)
        {
            this->_flags &= flag;
            return *this;
        }
        /// <summary>
        /// &= 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  auto&   operator&=(const FEFlags& other)
        {
            this->_flags &= other._flags;
            return *this;
        }
        /// <summary>
        /// | 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  FEFlags operator|(EnumType flag) const
        {
            return  FEFlags(this->_flags | flag);
        }
        /// <summary>
        /// | 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  FEFlags operator|(DataType flag) const
        {
            return  FEFlags(this->_flags | flag);
        }
        /// <summary>
        /// | 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  FEFlags operator|(const FEFlags& other) const
        {
            return FEFlags(this->_flags | other._flags);
        }
        /// <summary>
        /// |= 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   operator|=(EnumType flag)
        {
            this->_flags |= flag;
            return *this;
        }
        /// <summary>
        /// |= 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   operator|=(DataType flag)
        {
            this->_flags |= flag;
            return *this;
        }
        /// <summary>
        /// |= 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  auto&   operator|=(const FEFlags& other)
        {
            this->_flags |= other._flags;
            return *this;
        }
        /// <summary>
        /// ^ 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  FEFlags operator^(EnumType flag) const
        {
            return  FEFlags(this->_flags ^ flag);
        }
        /// <summary>
        /// ^ 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  FEFlags operator^(DataType flag) const
        {
            return FEFlags(this->_flags ^ flag);
        }
        /// <summary>
        /// ^ 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  FEFlags operator^(const FEFlags<EnumType, DataType>& other) const
        {
            return  FEFlags(this->_flags ^ other._flags);
        }
        /// <summary>
        /// ^ 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   operator^=(EnumType flag)
        {
            this->_flags ^= flag;
            return *this;
        }
        /// <summary>
        /// ^ 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  auto&   operator^=(DataType flag)
        {
            this->_flags ^= flag;
            return *this;
        }
        /// <summary>
        /// ^ 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  auto&   operator^=(const FEFlags<EnumType, DataType>& other)
        {
            this->_flags ^= other._flags;
            return *this;
        }
        /// <summary>
        /// == 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    operator==(EnumType flag) const
        {
            return this->_flags == (DataType)(flag);
        }
        /// <summary>
        /// == 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    operator==(DataType flag) const
        {
            return this->_flags == flag;
        }
        /// <summary>
        ///  == 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  bool    operator==(const FEFlags<EnumType, DataType>& other) const
        {
            return this->_flags == other._flags;
        }
        /// <summary>
        /// != 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    operator!=(EnumType flag) const
        {
            return this->_flags != (DataType)(flag);
        }
        /// <summary>
        /// != 运算
        /// </summary>
        /// <param name="flag"></param>
        /// <returns></returns>
        inline  bool    operator!=(DataType flag) const
        {
            return this->_flags != flag;
        }
        /// <summary>
        /// != 运算
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  bool    operator!=(const FEFlags& other) const
        {
            return this->_flags != other._flags;
        }
    };
}
