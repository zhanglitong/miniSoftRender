#pragma     once
#include    "FEMath.hpp"

namespace   FE
{
    template<typename TKey,typename TValue,typename TTree = std::map<TKey,TValue>>
    class   FEKeyValues
    {
    public:
        using   KeyValueMap =   TTree;
    public:
        FEKeyValues()
        {}
        /// <summary>
        /// 添加，如果存在返回false
        /// </summary>
        /// <param name="key"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        inline  bool    add(const TKey& key,const TValue& value)
        {
            auto    itr =   _valueMap.find(key);
            if (itr != _valueMap.end())
                return  false;
            else
                _valueMap[key]  =   value;
            return  true;
        }
        /// <summary>
        /// 移除,如果不存在,返回false
        /// </summary>
        /// <param name="key"></param>
        inline  bool    remove(const TKey& key)
        {
            auto    itr =   _valueMap.find(key);
            if (itr == _valueMap.end())
                return  false;
            else
                _valueMap.erase(itr);
            return  true;
        }
        /// <summary>
        /// 是否存在
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        inline  TValue* isExist(const TKey& key) 
        {
            auto    itr =   _valueMap.find(key);
            if (itr == _valueMap.end())
                return  nullptr;
            else
                return  &itr->second;
        }
        const   TValue* isExist(const TKey& key) const
        {
            auto    itr =   _valueMap.find(key);
            if (itr == _valueMap.end())
                return  nullptr;
            else
                return  &itr->second;
        }
        /// <summary>
        /// 查询
        /// </summary>
        /// <param name="key"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        inline  bool    query(const TKey& key,TValue& value)
        {
            auto    itr =   _valueMap.find(key);
            if (itr == _valueMap.end())
                return  false;
            else
                value   =   itr->second;
            return  true;
        }
        /// <summary>
        /// 如果存在返回,不存在创建
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        inline  TValue& queryOrCreate(const TKey& key)
        {
            return  _valueMap[key];
        }
        /// <summary>
        /// 清除
        /// </summary>
        inline  void    clear()
        {
            _valueMap.clear();
        }
        const   auto&   data() const
        {
            return  _valueMap;
        }
        inline  auto&   data()
        {
            return  _valueMap;
        }
        /// <summary>
        /// 检测数据是否发生了变化，需要更新
        /// </summary>
        /// <returns></returns>
        /// inline  bool    isDirty() const
        /// {
        ///     for (auto& var : _valueMap)
        ///     {
        ///         if (var.second->flags().hasFlag(FlagBit::FLAG_UPDATE))
        ///             return  true;
        ///     }
        ///     return  false;
        /// }
    protected:
        KeyValueMap   _valueMap;
    };
}
