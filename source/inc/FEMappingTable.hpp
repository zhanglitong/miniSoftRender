#pragma     once

#include    <array>
#include    <unordered_map>
#include    "FEObject.h"
#include    "FEUuid.hpp"

namespace FE
{
    /// <summary>
    /// 设计该类的主要目的是:每一个类型都对应一个classId,大小是16字节
    /// 占用较多磁盘空间，所以写入的是文件为了减少磁盘设计，适用一个索引对应一个UUid,降低存储
    /// 写入过程，写入index
    /// 读取过程，读取index,查询Uuid在使用
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T>
    class   FEMappingTable :public FEObject
    {
    protected:
        /// <summary>
        /// 原始数据，主要用作保存数据
        /// </summary>
        std::vector<T>                  _data;
        /// <summary>
        /// 主要用作查询使用,不直接使用，解决数据索引唯一性，例如频繁的插入删除，数据索引会发生变化
        /// </summary>
        std::unordered_map<T,size_t>    _index;
    public:
        FEMappingTable(size_t reserve = 0)
        {
            if (reserve != 0)
                _data.reserve(reserve);
        }
        /// <summary>
        /// 当向系统中增加了数据后，建议在添加完所有的Id后，调用该函数实现快速索引数据
        /// 典型场景:FEMappingTable<FEUuid> idMap;
        /// idMap.add(FEUuid);
        /// idMap.add(FEUuid)
        /// ...
        /// idMap.buidIndex()
        /// idMap.indexOf(CELLUuid) 返回id的索引数据
        /// </summary>
        void    buidIndex()
        {
            for (size_t i = 0; i < _data.size(); i++)
            {
                _index[_data[i]]    =   i;
            }
        }
        /// <summary>
        /// 根据值获取索引
        /// </summary>
        /// <param name="val"></param>
        /// <returns></returns>
        size_t  indexOf(const T& val) const
        {
            auto    itr =   _index.find(val);
            if (itr == _index.end())
                return  size_t(-1);
            else
                return  itr->second;
        }
        /// <summary>
        /// 添加数据到系统,返回索引号
        /// </summary>
        /// <param name="val"></param>
        /// <returns></returns>
        size_t  add(const T& val)
        {
            auto    itr     =   std::find(_data.begin(),_data.end(),val);
            if (itr != _data.end())
                return  std::distance(itr,_data.begin());
            auto    size    =   _data.size();
            _data.emplace_back(val);
            return  size;
        }
        /// <summary>
        /// 获取数据
        /// </summary>
        /// <param name="index"></param>
        /// <param name="data"></param>
        /// <returns></returns>
        bool    get(size_t index,T& data)
        {
            if (index < _data.size())
            {
                data  = _data[index];
                return  true;
            }
            return  false;
        }
    };
}


