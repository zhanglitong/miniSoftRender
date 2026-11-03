#pragma     once

#include    "../../../inc/axis/FEAxisSetting.hpp"
#include    "../../../inc/node/FENode.hpp"

namespace   FE
{
    /// <summary>
    /// 轴编辑窗口(标题:坐标轴编辑):编辑 FEAxisSetting
    /// </summary>
    /// <param name="setting">轴配置(输入输出)</param>
    /// <param name="open">窗口开关</param>
    /// <param name="selection">模型树当前选区,用于"填充"轴心/"拾取方向"</param>
    /// <param name="before">返回本次修改前的配置快照(供撤销命令使用)</param>
    /// <returns>true 表示本次绘制修改了配置;一次拖动只在松开时提交一次,不会逐帧返回</returns>
    bool    drawAxisSettingWindow(FEAxisSetting& setting,bool& open,const Objects& selection
                                  ,FEAxisSetting& before);
}
