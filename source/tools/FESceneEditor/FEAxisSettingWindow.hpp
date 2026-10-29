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
    /// <param name="selection">模型树当前选区,用于"填充"轴心/轴位置</param>
    void    drawAxisSettingWindow(FEAxisSetting& setting,bool& open,const Objects& selection);
}
