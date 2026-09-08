#pragma     once

#include    "FEAxis.h"
/// <summary>
/// FEEditAxisMove.h
/// 定义移动编辑坐标轴
/// </summary>
namespace   FE
{
    class   FEEditAxisMovePrivate;
    class   FE_API  FEEditAxisMove: public FEEditAxis
    {
    public:
        /// <summary>
        /// 轴标志
        /// </summary>
        enum AXIS
        {
            AXIS_NULL = 0,
            AXIS_X,
            AXIS_Y,
            AXIS_Z,
            AXIS_XY,//xoy平面
            AXIS_YZ,//yoz平面
            AXIS_XZ,//xoz平面
        };
        /// <summary>
        /// 移动吸附标志
        /// </summary>
        enum AdsorptionFlag
        {
            ///无
            AF_None         =   0, 
            ///几何体顶点吸附标志
            AF_Points       =   1 << 0,
            ///几何体边线吸附标志
            AF_Edge         =   1 << 1,
            ///几何体边线中心点吸附标志
            AF_EdgeCenter   =   1 << 2,
            ///几何体三角面吸附标志
            AF_Face         =   1 << 3,
            ///几何体三角面中心点吸附标志
            AF_FaceCenter   =   1 << 4,
            ///节点位置吸附标志
            AF_Position     =   1 << 5,
            ///节点包围盒吸附标志(顶点以及包围盒中心点)
            AF_Aabb         =   1 << 6,
        };
        using AdsorptionFlags = FEFlags<AdsorptionFlag, uint>;
        /// <summary>
        /// 移动通知
        /// </summary>
        /// <param name="status">编辑状态</param>
        /// <param name="relativeOffset">相对偏移量(世界坐标)</param>
        /// <param name="absoluteOffset">绝对偏移量(世界坐标)</param>
        /// <param name="sender">调用通知的轴对象</param>
        using MDelegate = FETMultiDelegate<void(EditStatus status
            , const real3& relativeOffset
            , const real3& absoluteOffset
            , FEEditAxisMove& sender)>;
    private:
        FEEditAxisMovePrivate* _p;
        friend class FEEditAxisMovePrivate;
    public:
        FEEditAxisMove(FEContext& ctx);

        FEEditAxisMove(const FEEditAxisMove& other);

        ~FEEditAxisMove();
    public:
        /// <summary>
        /// 移动操作回调,当轴发生移动操作时，触发该回调
        /// </summary>
        MDelegate& mDelegate();
        /// <summary>
        /// 获取当前已经被高亮的轴
        /// </summary>
        /// <returns>返回高亮的轴索引</returns>
        FEEditAxisMove::AXIS hoveredAxis() const;
        /// <summary>
        /// 获取当前已经被选中的轴
        /// </summary>
        /// <returns>返回选中的轴索引</returns>
        FEEditAxisMove::AXIS selectedAxis() const;
        /// <summary>
        /// 给定一个单位向量，计算当沿着着该单位向量移动时，移动偏移量相对于X轴的移动偏移量的权重
        /// </summary>
        inline real weightX(const real3& vec) const
        {
            return dot(vec, this->axisX());
        }
        /// <summary>
        /// 给定一个单位向量，计算当沿着着该单位向量移动时，移动偏移量相对于Y轴的移动偏移量的权重
        /// </summary>
        inline real weightY(const real3& vec) const
        {
            return dot(vec, this->axisY());
        }
        /// <summary>
        /// 给定一个单位向量，计算当沿着着该单位向量移动时，移动偏移量相对于Z轴的移动偏移量的权重
        /// </summary>
        inline real weightZ(const real3& vec) const
        {
            return dot(vec, this->axisZ());
        }

        /// <summary>
        /// 启用/禁用移动吸附
        /// </summary>
        void    setAdsorptionEnabled(bool enabled);
        /// <summary>
        /// 获取移动吸附是否启用
        /// </summary>
        bool    adsorptionEnabled() const;
        /// <summary>
        /// 获取移动吸附标志
        /// </summary>
        AdsorptionFlags adsorptionFlags() const;
        /// <summary>
        /// 设置移动吸附标志
        /// </summary>
        void    setAdsorptionFlags(AdsorptionFlags flags) ;
        /// <summary>
        /// 获取移动吸附点像素范围
        /// </summary>
        real    adsorptionPixel() const;
        /// <summary>
        /// 设置移动吸附点像素范围
        /// </summary>
        void    setAdsorptionPixel(real pixel) ;
        /// <summary>
        /// 每一帧调用该函数,会自动生成 moveAxis,axisArray,indexs 数据
        /// </summary>
        /// <param name="camera"></param>
        void    update(FECamera& camera);
        ///轴顶点
        const   float3s&    moveAxis() const;
        ///箭头顶点
        const   float3s&    axisArray() const;
        /// <summary>
        /// 索引数据
        /// </summary>
        const   uint16s&    indexs() const;
    public:
        /// <summary>
        /// 是否有轴被高亮
        /// </summary>
        /// <returns>如果有轴被高亮，则返回true,否则返回false</returns>
        virtual bool isAxisHovered() const override
        {
            return hoveredAxis() != FEEditAxisMove::AXIS::AXIS_NULL;
        }
        /// <summary>
        /// 是否有轴被选中
        /// </summary>
        /// <returns>如果有轴被选中,则返回true,否则返回false</returns>
        virtual bool isAxisSelected() const override
        {
            return selectedAxis() != FEEditAxisMove::AXIS::AXIS_NULL;
        }
        
        /// <summary>
        /// 取消轴的高亮状态
        /// </summary>
        virtual void cancelHovered() override;
        /// <summary>
        /// 取消轴的选中状态
        /// </summary>
        virtual void cancelSelected()override;
    protected:
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisX(int value) const override
        {
            return value == FEEditAxisMove::AXIS::AXIS_X;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisY(int value) const override
        {
            return value == FEEditAxisMove::AXIS::AXIS_Y;
        }
        /// <summary>
        /// 判断某个值是否是X轴的枚举值,子类实现
        /// </summary>
        virtual bool isEnumAxisZ(int value) const override
        {
            return value == FEEditAxisMove::AXIS::AXIS_Z;
        }
    public:
        /// <summary>
        /// 需要被其他子类重写，处理消息
        /// </summary>
        /// <param name=""></param>
        virtual void    onMessage(const FEMessage& ) override;
    protected:
        virtual bool mouseButtonPress(const int2& pos)  ;
        virtual bool mouseButtonRelease(const int2& pos);
        virtual bool mouseMove(const int2& pos)         ;
        virtual bool touchDown(const int2& pos)         ;
        virtual bool touchUp(const int2& pos)           ;
        virtual bool touchMove(const int2& pos)         ;
    };

    using   EditAxisMove    =   SharedPtr<FEEditAxisMove>;
}

