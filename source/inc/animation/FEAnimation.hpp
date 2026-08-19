#pragma     once
#include    <functional>
#include    "FEClip.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAnimation, "{D0749EE9-7126-4A57-B4F2-84798E4F40F2}");

    /// <summary>
    /// 瀵硅薄涓寘鍚渶瑕佽鍔ㄧ敾鐨勫璞?
    /// 浠ュ強clip淇℃伅锛宑lip 鍔ㄧ敾鐗囨锛屽寘鍚疄闄呯殑鍔ㄧ敾鍏抽敭甯х粍鏁版嵁
    /// 鍙互瀹炵幇鍔ㄧ敾鏁版嵁涓庡姩鐢诲璞″垎绂?璁╁姩鐢绘暟鎹湁澶嶇敤浠峰€?
    /// 鍏稿瀷搴旂敤锛氫竴涓璞★紝鍦ㄦ椂闂寸嚎涓婏紝鍙互鏂紑
    /// </summary>
    class   FEAnimation : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimation)
    public:
        FEAnimation(FEContext& ctx)  
            :FEObject(ctx)
        {}
        FEAnimation(const FEAnimation& other)
            :FEObject(other)
        {
            _clip       =   other._clip;
            _object     =   other._object;
            _results    =   other._results;
        }
        ~FEAnimation()   =   default;
    public:
        /// <summary>
        /// 鏄惁鏈夋晥
        /// </summary>
        /// <returns></returns>
        bool    isValid() const
        {
            return  _clip != nullptr &&  _object != nullptr;
        }
        ClipPtr clip() const
        {
            return  _clip;
        }
        void    setClip(ClipPtr clip)
        {
            _clip   =   clip;
        }

        Object  object() const
        {
            return  _object;
        }
        void    setObject(Object object)
        {
            _object =   object;
        }
        /// <summary>
        /// 鏇存柊,浼氭妸鍔ㄧ敾鏁版嵁搴旂敤鍒板璞′笂
        /// </summary>
        /// <param name="tmDelat"></param>
        bool    update(const real& tmDelta)
        {
            if (_clip == nullptr || _object == nullptr)
                return  false;
            _clip->update(tmDelta,_results);

            _object->beginSetProp();

            bool    bModify =   false;

            for (auto& var: _results)
            {
                if (!var._valid)
                    continue;
                bModify |=  _object->setProperty(var._prop,var._value);
            }
            _object->endSetProp(bModify);
            return  true;
        }
    public:
        ClipPtr         _clip;
        Object          _object;
        TrackResults    _results; 
    };

    using   Animation       =   SharedPtr<FEAnimation>;
    using   Animations      =   std::vector<Animation>;
}
