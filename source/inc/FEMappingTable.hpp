#pragma     once

#include    <array>
#include    <unordered_map>
#include    "FEObject.h"
#include    "FEUuid.hpp"

namespace FE
{
    /// <summary>
    /// 璁捐璇ョ被鐨勪富瑕佺洰鐨勬槸:姣忎竴涓被鍨嬮兘瀵瑰簲涓€涓猚lassId,澶у皬鏄?6瀛楄妭
    /// 鍗犵敤杈冨纾佺洏绌洪棿锛屾墍浠ュ啓鍏ョ殑鏄枃浠朵负浜嗗噺灏戠鐩樿璁★紝閫傜敤涓€涓储寮曞搴斾竴涓猆Uid,闄嶄綆瀛樺偍
    /// 鍐欏叆杩囩▼锛屽啓鍏ndex
    /// 璇诲彇杩囩▼锛岃鍙杋ndex,鏌ヨUuid鍦ㄤ娇鐢?
    /// </summary>
    /// <typeparam name="T"></typeparam>
    template<typename T>
    class   FEMappingTable :public FEObject
    {
    protected:
        /// <summary>
        /// 鍘熷鏁版嵁锛屼富瑕佺敤浣滀繚瀛樻暟鎹?        /// </summary>
        std::vector<T>                  _data;
        /// <summary>
        /// 涓昏鐢ㄤ綔鏌ヨ浣跨敤,涓嶇洿鎺ヤ娇鐢紝瑙ｅ喅鏁版嵁绱㈠紩鍞竴鎬э紝渚嬪棰戠箒鐨勬彃鍏ュ垹闄わ紝鏁版嵁绱㈠紩浼氬彂鐢熷彉鍖?
        /// </summary>
        std::unordered_map<T,size_t>    _index;
    public:
        FEMappingTable(size_t reserve = 0)
        {
            if (reserve != 0)
                _data.reserve(reserve);
        }
        /// <summary>
        /// 褰撳悜绯荤粺涓鍔犱簡鏁版嵁鍚庯紝寤鸿鍦ㄦ坊鍔犲畬鎵€鏈夌殑Id鍚庯紝璋冪敤璇ュ嚱鏁板疄鐜板揩閫熺储寮曟暟鎹?        /// 鍏稿瀷鍦烘櫙:FEMappingTable<FEUuid> idMap;
        /// idMap.add(FEUuid);
        /// idMap.add(FEUuid)
        /// ...
        /// idMap.buidIndex()
        /// idMap.indexOf(CELLUuid) 杩斿洖id鐨勭储寮曟暟鎹?        /// </summary>
        void    buidIndex()
        {
            for (size_t i = 0; i < _data.size(); i++)
            {
                _index[_data[i]]    =   i;
            }
        }
        /// <summary>
        /// 鏍规嵁鍊艰幏鍙栫储寮?        /// </summary>
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
        /// 娣诲姞鏁版嵁鍒扮郴缁?杩斿洖绱㈠紩鍙?        /// </summary>
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
        /// 鑾峰彇鏁版嵁
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


