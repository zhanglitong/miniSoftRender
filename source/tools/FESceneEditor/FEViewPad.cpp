/// <summary>
/// FEViewPad 实现:用当前相机 right/up/dir 把立方体与罗盘环投影成屏幕空间图元,
/// 由 ImDrawList 绘制;鼠标命中在屏幕空间做(可见角 → 可见面 → 罗盘环),
/// 拖动走 FECamera 的轨道旋转,单击面/角走 setEye/setUp 的短促平滑过渡。
/// </summary>

#include    "FEViewPad.hpp"
#include    "FEUiMetrics.hpp"

#include    "../../../inc/FEContext.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    <algorithm>
#include    <cmath>
#include    <cstdio>

#include    <imgui.h>

namespace   FE
{
namespace
{
    /// <summary>
    /// 操作盘窗口边长与距屏幕右上角的边距
    /// </summary>
    const   float   kPadSize        =   150.0f;
    const   float   kPadMarginRight =   10.0f;
    const   float   kPadMarginTop   =   12.0f;
    /// <summary>
    /// 立方体半边长的屏幕像素(顶点坐标以半边长为 1 个单位)
    /// </summary>
    const   float   kCubeHalfPx     =   24.0f;
    /// <summary>
    /// 罗盘环半径(立方体半边长为单位)与所在的高度
    /// </summary>
    const   float   kRingRadius     =   2.0f;
    const   float   kRingZ          =   -1.1f;
    /// <summary>
    /// 罗盘环采样段数(逐段按深度分前后两色)
    /// </summary>
    const   int     kRingSamples    =   64;
    /// <summary>
    /// 背景圆盘半径与整体纵向偏移(给罗盘环留出下方空间)
    /// </summary>
    const   float   kBackdropRadius =   64.0f;
    const   float   kCenterOffsetY  =   -6.0f;
    /// <summary>
    /// 视图切换过渡时长(秒)与拖拽判定阈值(像素)
    /// </summary>
    const   real    kAnimDuration   =   0.25;
    const   float   kDragThreshold  =   3.0f;
    /// <summary>
    /// 面命中时角区域优先的半径(像素)与罗盘环命中宽度(像素)
    /// </summary>
    const   float   kCornerHitRadius=   8.0f;
    const   float   kRingHitWidth   =   6.0f;
    /// <summary>
    /// 面标签绘制的最小面积(像素²)与最小正对程度
    /// </summary>
    const   float   kLabelMinArea   =   300.0f;
    const   real    kLabelMinFacing =   0.18;

    /// <summary>
    /// 光照方向(世界坐标,左上偏前):决定六个面的明暗
    /// </summary>
    const   real3   kLightDir   =   normalize(real3(-0.35,-0.6,0.72));
    /// <summary>
    /// 立方体面:法线、面名、对应轴视图名与四个顶点下标
    /// 顶点下标按 (x 正, y 正, z 正) 三个位组合:
    /// 0:(-,-,-) 1:(-,-,+) 2:(-,+,-) 3:(-,+,+)
    /// 4:(+,-,-) 5:(+,-,+) 6:(+,+,-) 7:(+,+,+)
    /// </summary>
    struct  FEPadFaceInfo
    {
        real3       _normal;
        const char* _label;
        const char* _viewName;
        int         _verts[4];
    };
    const   FEPadFaceInfo   kPadFaces[6] =
    {
        {   real3( 0,-1, 0),    "前",   "前视图",   {   0,4,5,1 } },
        {   real3( 0, 1, 0),    "后",   "后视图",   {   2,3,7,6 } },
        {   real3(-1, 0, 0),    "左",   "左视图",   {   0,1,3,2 } },
        {   real3( 1, 0, 0),    "右",   "右视图",   {   4,5,7,6 } },
        {   real3( 0, 0, 1),    "上",   "顶视图",   {   1,3,7,5 } },
        {   real3( 0, 0,-1),    "下",   "底视图",   {   0,2,6,4 } },
    };
    /// <summary>
    /// 每个面对应的相机朝向(eye→target)与 up,取值与 FECamera::lookAtTo 一致
    /// </summary>
    const   real3   kFaceDirs[6]    =
    {
        real3(0, 1,0),  real3(0,-1,0),  real3(1,0,0),
        real3(-1,0,0),  real3(0,0,-1),  real3(0,0,1),
    };
    const   real3   kFaceUps[6]     =
    {
        real3(0,0,1),   real3(0,0,1),   real3(0,0,1),
        real3(0,0,1),   real3(0,1,0),   real3(0,-1,0),
    };

    /// <summary>
    /// 顶点下标 -> 立方体局部坐标(半边长为 1)
    /// </summary>
    real3   padVertex(int index)
    {
        return  real3((index & 4) ? real(1.0) : real(-1.0),
                      (index & 2) ? real(1.0) : real(-1.0),
                      (index & 1) ? real(1.0) : real(-1.0));
    }

    /// <summary>
    /// 绘制用的投影基:屏幕中心 + 相机三轴
    /// </summary>
    struct  FEPadFrame
    {
        ImVec2  _center;
        real3   _right;
        real3   _up;
        real3   _dir;
    };

    /// <summary>
    /// 立方体局部坐标 -> 屏幕坐标(正交投影,y 轴向下)
    /// </summary>
    ImVec2  projectPad(const FEPadFrame& frame,const real3& local)
    {
        const   real    x   =   dot(local,frame._right);
        const   real    y   =   -dot(local,frame._up);
        return  ImVec2(frame._center.x + float(x * kCubeHalfPx),
                       frame._center.y + float(y * kCubeHalfPx));
    }

    /// <summary>
    /// 点到线段距离(罗盘环命中)
    /// </summary>
    float   distToSegment(const ImVec2& p,const ImVec2& a,const ImVec2& b)
    {
        const   float   abx =   b.x - a.x;
        const   float   aby =   b.y - a.y;
        const   float   apx =   p.x - a.x;
        const   float   apy =   p.y - a.y;
        const   float   len2=   abx * abx + aby * aby;
        float           t   =   (len2 > 0.0f) ? (apx * abx + apy * aby) / len2 : 0.0f;
        t   =   std::max(0.0f,std::min(1.0f,t));
        const   float   dx  =   p.x - (a.x + abx * t);
        const   float   dy  =   p.y - (a.y + aby * t);
        return  std::sqrt(dx * dx + dy * dy);
    }

    /// <summary>
    /// 凸四边形命中(允许任意点序,边界计入)
    /// </summary>
    bool    pointInConvexQuad(const ImVec2& p,const ImVec2* quad)
    {
        int     positive=   0;
        int     negative=   0;
        for (int i = 0; i < 4; ++i)
        {
            const   ImVec2& a   =   quad[i];
            const   ImVec2& b   =   quad[(i + 1) % 4];
            const   float   cross   =   (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
            if (cross > 0.0f)
                ++positive;
            else if (cross < 0.0f)
                ++negative;
        }
        return  (positive == 0 || negative == 0);
    }

    /// <summary>
    /// 按符号面积把多边形调整为顺时针(ImGui 抗锯齿填充要求顺时针点序)
    /// </summary>
    void    makeClockwise(ImVec2* points,int count)
    {
        float   area2   =   0.0f;
        for (int i = 0; i < count; ++i)
        {
            const   ImVec2& a   =   points[i];
            const   ImVec2& b   =   points[(i + 1) % count];
            area2   +=  a.x * b.y - b.x * a.y;
        }
        if (area2 < 0.0f)
            std::reverse(points,points + count);
    }

    /// <summary>
    /// 面填充色:按法线与光照计算明暗,悬停时混入主题强调色
    /// </summary>
    ImU32   faceFillColor(const real3& normal,float hover)
    {
        const   float   light   =   float(std::max(real(0.0),dot(normal,kLightDir)));
        const   float   k       =   0.55f + 0.45f * light;
        float           r       =   190.0f * k;
        float           g       =   196.0f * k;
        float           b       =   208.0f * k;
        if (hover > 0.0f)
        {
            r   =   r * (1.0f - hover) + 74.0f * hover;
            g   =   g * (1.0f - hover) + 144.0f * hover;
            b   =   b * (1.0f - hover) + 226.0f * hover;
        }
        return  IM_COL32(int(r),int(g),int(b),255);
    }

    /// <summary>
    /// 平滑缓动(3t²-2t³)
    /// </summary>
    real    smoothStep(real t)
    {
        return  t * t * (real(3.0) - real(2.0) * t);
    }
}

    void    FEViewPad::bind(FEScene* scene,FEContext* ctx)
    {
        _scene  =   scene;
        _ctx    =   ctx;
        resetState();
    }

    void    FEViewPad::unbind()
    {
        resetState();
        _scene  =   nullptr;
        _ctx    =   nullptr;
    }

    void    FEViewPad::toggle()
    {
        _open   =   !_open;
    }

    void    FEViewPad::setOpen(bool open)
    {
        _open   =   open;
    }

    void    FEViewPad::resetState()
    {
        _animating      =   false;
        _animT          =   0.0;
        _pressPart      =   kPartNone;
        _pressFace      =   -1;
        _pressCorner    =   -1;
        _pressDouble    =   false;
        _dragging       =   false;
    }

    void    FEViewPad::draw()
    {
        if (!_open || _scene == nullptr || _ctx == nullptr)
            return;
        Camera  camera  =   _scene->camera();
        if (camera == nullptr)
            return;

        ImGuiIO&    io  =   ImGui::GetIO();
        /// <summary>
        /// 先推进过渡,后面的投影使用推进后的相机朝向
        /// </summary>
        stepAnimation(*camera,real(io.DeltaTime));

        ImVec2  padPos(io.DisplaySize.x - kPadSize - kPadMarginRight,
                       ui::kRibbonHeight + kPadMarginTop);
        padPos.x    =   std::max(0.0f,padPos.x);
        padPos.y    =   std::max(0.0f,padPos.y);

        ImGui::SetNextWindowPos(padPos,ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(kPadSize,kPadSize),ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0.0f,0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0.0f,0.0f));

        const   ImGuiWindowFlags    flags   =   ImGuiWindowFlags_NoTitleBar
                                            |   ImGuiWindowFlags_NoResize
                                            |   ImGuiWindowFlags_NoMove
                                            |   ImGuiWindowFlags_NoScrollbar
                                            |   ImGuiWindowFlags_NoScrollWithMouse
                                            |   ImGuiWindowFlags_NoCollapse
                                            |   ImGuiWindowFlags_NoSavedSettings
                                            |   ImGuiWindowFlags_NoBringToFrontOnFocus
                                            |   ImGuiWindowFlags_NoNav
                                            |   ImGuiWindowFlags_NoFocusOnAppearing;
        const   bool    opened  =   ImGui::Begin("##fesr_viewpad",nullptr,flags);
        if (opened)
        {
            ImGui::InvisibleButton("##fesr_viewpad_hit",ImVec2(kPadSize,kPadSize));
            const   bool    hovered =   ImGui::IsItemHovered();
            const   bool    active  =   ImGui::IsItemActive();

            const   ImVec2  winPos  =   ImGui::GetWindowPos();
            FEPadFrame      frame;
            frame._center   =   ImVec2(winPos.x + kPadSize * 0.5f,
                                       winPos.y + kPadSize * 0.5f + kCenterOffsetY);
            frame._right    =   normalize(camera->getRight());
            frame._up       =   normalize(camera->getUp());
            frame._dir      =   normalize(camera->getDir());

            /// <summary>
            /// 立方体顶点投影与可见性:面法线与视线夹角为负表示朝向相机
            /// </summary>
            ImVec2  verts[8];
            bool    cornerVisible[8]    =   {   false   };
            bool    faceVisible[6]      =   {   false   };
            real    faceFacing[6]       =   {   0.0   };
            for (int i = 0; i < 8; ++i)
                verts[i]    =   projectPad(frame,padVertex(i));
            for (int f = 0; f < 6; ++f)
            {
                faceFacing[f]   =   dot(kPadFaces[f]._normal,frame._dir);
                faceVisible[f]  =   faceFacing[f] < real(-0.02);
                if (!faceVisible[f])
                    continue;
                for (int i = 0; i < 4; ++i)
                    cornerVisible[kPadFaces[f]._verts[i]]   =   true;
            }

            /// <summary>
            /// 罗盘环采样点与深度(深度为负表示靠近相机)
            /// </summary>
            ImVec2  ringPoints[kRingSamples];
            real    ringDepth[kRingSamples];
            for (int i = 0; i < kRingSamples; ++i)
            {
                const   real    angle   =   real(6.283185307179586) * real(i) / real(kRingSamples);
                const   real3   local(cos(angle) * real(kRingRadius),
                                      sin(angle) * real(kRingRadius),
                                      real(kRingZ));
                ringPoints[i]   =   projectPad(frame,local);
                ringDepth[i]    =   dot(local,frame._dir);
            }

            /// <summary>
            /// 命中测试:可见角(角优先) → 可见面 → 罗盘环;
            /// 按下那一帧按钮已进入激活态,此时仍要按当前鼠标位置做命中,
            /// 否则 _pressPart 会沿用上一轮的空值,单击/拖动都识别不到部件;
            /// 进入拖拽后才固定为按下的部件(鼠标移出盘面也能继续拖动)
            /// </summary>
            int     hoverPart   =   kPartNone;
            int     hoverFace   =   -1;
            int     hoverCorner =   -1;
            if (_dragging)
            {
                hoverPart   =   _pressPart;
                hoverFace   =   _pressFace;
                hoverCorner =   _pressCorner;
            }
            else if (hovered)
            {
                const   ImVec2  mouse   =   io.MousePos;
                float           best    =   kCornerHitRadius;
                for (int i = 0; i < 8; ++i)
                {
                    if (!cornerVisible[i])
                        continue;
                    const   float   dx  =   mouse.x - verts[i].x;
                    const   float   dy  =   mouse.y - verts[i].y;
                    const   float   d   =   std::sqrt(dx * dx + dy * dy);
                    if (d < best)
                    {
                        best        =   d;
                        hoverPart   =   kPartCorner;
                        hoverCorner =   i;
                    }
                }
                if (hoverPart == kPartNone)
                {
                    for (int f = 0; f < 6; ++f)
                    {
                        if (!faceVisible[f])
                            continue;
                        ImVec2  quad[4];
                        for (int i = 0; i < 4; ++i)
                            quad[i] =   verts[kPadFaces[f]._verts[i]];
                        makeClockwise(quad,4);
                        if (pointInConvexQuad(mouse,quad))
                        {
                            hoverPart   =   kPartFace;
                            hoverFace   =   f;
                            break;
                        }
                    }
                }
                if (hoverPart == kPartNone)
                {
                    float   bestRing    =   kRingHitWidth;
                    for (int i = 0; i < kRingSamples; ++i)
                    {
                        const   float   d   =   distToSegment(mouse,ringPoints[i],
                                                               ringPoints[(i + 1) % kRingSamples]);
                        if (d < bestRing)
                        {
                            bestRing    =   d;
                            hoverPart   =   kPartRing;
                            break;
                        }
                    }
                }
            }

            /// <summary>
            /// 交互:按下记录命中,超过阈值进入拖拽,抬起时未拖拽则切换视图;
            /// 双击回默认视角,滚轮缩放(锚点与视口一致)
            /// </summary>
            if (ImGui::IsItemClicked(0))
            {
                _pressPart      =   hoverPart;
                _pressFace      =   hoverFace;
                _pressCorner    =   hoverCorner;
                _pressDouble    =   ImGui::IsMouseDoubleClicked(0);
                _pressPos       =   io.MousePos;
                _dragging       =   false;
            }
            if (active && ImGui::IsMouseDown(0))
            {
                const   float   dx  =   io.MousePos.x - _pressPos.x;
                const   float   dy  =   io.MousePos.y - _pressPos.y;
                if (!_dragging && dx * dx + dy * dy > kDragThreshold * kDragThreshold)
                    _dragging   =   true;
                if (_dragging)
                {
                    _animating  =   false;
                    orbitByMouse(*camera,ImVec2(-io.MouseDelta.x,-io.MouseDelta.y),_pressPart);
                }
            }
            if (ImGui::IsItemDeactivated())
            {
                if (!_dragging)
                {
                    if (_pressDouble)
                        snapToDefault(*camera);
                    else if (_pressPart == kPartCorner && _pressCorner >= 0)
                        snapToCorner(*camera,_pressCorner);
                    else if (_pressPart == kPartFace && _pressFace >= 0)
                        snapToFace(*camera,_pressFace);
                }
                _pressPart      =   kPartNone;
                _pressFace      =   -1;
                _pressCorner    =   -1;
                _dragging       =   false;
            }
            if (hovered && io.MouseWheel != 0.0f)
            {
                _animating  =   false;
                const   real3   pivot   =   _ctx->anchor().point();
                camera->scaleCameraByPos(pivot,io.MouseWheel > 0.0f ? real(1.2) : real(0.8333));
            }

            /// <summary>
            /// 绘制:背景圆盘 → 罗盘环 → 立方体面 → 面标签 → 悬停高亮
            /// </summary>
            ImDrawList*     dl  =   ImGui::GetWindowDrawList();
            dl->PushClipRect(winPos,ImVec2(winPos.x + kPadSize,winPos.y + kPadSize),true);
            dl->AddCircleFilled(frame._center,kBackdropRadius,IM_COL32(12,14,18,40),48);
            dl->AddCircle(frame._center,kBackdropRadius,IM_COL32(255,255,255,22),48,1.0f);

            const   bool    ringHover   =   (hoverPart == kPartRing);
            dl->AddPolyline(ringPoints,kRingSamples,IM_COL32(10,12,16,170),true,6.0f);
            for (int i = 0; i < kRingSamples; ++i)
            {
                const   int     j       =   (i + 1) % kRingSamples;
                const   ImU32   color   =   ringHover
                                            ? IM_COL32(74,144,226,255)
                                            : ((ringDepth[i] + ringDepth[j]) * real(0.5) < real(0.0)
                                                ? IM_COL32(146,153,166,235)
                                                : IM_COL32(88,94,106,200));
                dl->AddLine(ringPoints[i],ringPoints[j],color,ringHover ? 5.0f : 4.0f);
            }
            /// <summary>
            /// 罗盘刻度:八个方向短刻线,四个斜向刻度略长
            /// </summary>
            for (int i = 0; i < 8; ++i)
            {
                const   real    angle   =   real(6.283185307179586) * real(i) / real(8);
                const   bool    diagonal=   (i % 2) != 0;
                const   real3   inner(cos(angle) * real(kRingRadius + 0.08),
                                      sin(angle) * real(kRingRadius + 0.08),
                                      real(kRingZ));
                const   real3   outer(cos(angle) * real(kRingRadius + (diagonal ? 0.42 : 0.26)),
                                      sin(angle) * real(kRingRadius + (diagonal ? 0.42 : 0.26)),
                                      real(kRingZ));
                const   ImU32   color   =   ringHover ? IM_COL32(120,180,255,255)
                                                     : IM_COL32(120,126,138,220);
                dl->AddLine(projectPad(frame,inner),projectPad(frame,outer),color,2.0f);
            }

            /// <summary>
            /// 面按深度从远到近绘制:正对相机的面在最后覆盖
            /// </summary>
            int     faceOrder[6];
            int     faceCount   =   0;
            for (int f = 0; f < 6; ++f)
            {
                if (faceVisible[f])
                    faceOrder[faceCount++]  =   f;
            }
            std::sort(faceOrder,faceOrder + faceCount,
                      [&](int a,int b) {   return  faceFacing[a] > faceFacing[b];   });
            for (int index = 0; index < faceCount; ++index)
            {
                const   int     f       =   faceOrder[index];
                ImVec2          quad[4];
                for (int i = 0; i < 4; ++i)
                    quad[i] =   verts[kPadFaces[f]._verts[i]];
                makeClockwise(quad,4);
                const   bool    hover   =   (hoverPart == kPartFace && hoverFace == f);
                dl->AddConvexPolyFilled(quad,4,faceFillColor(kPadFaces[f]._normal,hover ? 0.35f : 0.0f));
                dl->AddPolyline(quad,4,hover ? IM_COL32(120,180,255,255) : IM_COL32(26,29,36,255),
                                true,hover ? 2.0f : 1.2f);
            }
            /// <summary>
            /// 面标签:仅面积足够的可见面绘制,文字暗色居中
            /// </summary>
            for (int f = 0; f < 6; ++f)
            {
                if (!faceVisible[f] || std::abs(faceFacing[f]) < kLabelMinFacing)
                    continue;
                ImVec2  quad[4];
                for (int i = 0; i < 4; ++i)
                    quad[i] =   verts[kPadFaces[f]._verts[i]];
                float   area2   =   0.0f;
                ImVec2  center(0.0f,0.0f);
                for (int i = 0; i < 4; ++i)
                {
                    const   ImVec2& a   =   quad[i];
                    const   ImVec2& b   =   quad[(i + 1) % 4];
                    area2   +=  a.x * b.y - b.x * a.y;
                    center.x    +=  a.x * 0.25f;
                    center.y    +=  a.y * 0.25f;
                }
                if (std::abs(area2) * 0.5f < kLabelMinArea)
                    continue;
                const   ImVec2  textSize    =   ImGui::CalcTextSize(kPadFaces[f]._label);
                dl->AddText(ImVec2(center.x - textSize.x * 0.5f,center.y - textSize.y * 0.5f),
                            IM_COL32(28,31,38,235),kPadFaces[f]._label);
            }
            /// <summary>
            /// 悬停角:在顶点处画强调色圆点与细环
            /// </summary>
            if (hoverPart == kPartCorner && hoverCorner >= 0)
            {
                dl->AddCircleFilled(verts[hoverCorner],3.0f,IM_COL32(74,144,226,255),12);
                dl->AddCircle(verts[hoverCorner],6.5f,IM_COL32(120,180,255,220),16,1.5f);
            }
            dl->PopClipRect();

            /// <summary>
            /// 悬停提示:画在盘下方的前景层,避免 ImGui 原生 tooltip 贴边时盖住盘面
            /// </summary>
            if (hovered && !active && hoverPart != kPartNone)
            {
                char    hint[96]    =   {};
                if (hoverPart == kPartFace && hoverFace >= 0)
                    std::snprintf(hint,sizeof(hint),"%s · 单击切换",kPadFaces[hoverFace]._viewName);
                else if (hoverPart == kPartCorner)
                    std::snprintf(hint,sizeof(hint),"等轴视图 · 单击切换");
                else if (hoverPart == kPartRing)
                    std::snprintf(hint,sizeof(hint),"拖动水平环绕");
                if (hint[0] != '\0')
                {
                    const   ImVec2  textSize    =   ImGui::CalcTextSize(hint);
                    const   ImVec2  textPos(winPos.x + kPadSize - textSize.x - 6.0f,
                                            winPos.y + kPadSize + 2.0f);
                    ImDrawList*     fg  =   ImGui::GetOverlayDrawList();
                    fg->AddRectFilled(ImVec2(textPos.x - 6.0f,textPos.y - 2.0f),
                                      ImVec2(textPos.x + textSize.x + 6.0f,textPos.y + textSize.y + 2.0f),
                                      IM_COL32(16,18,22,210),4.0f);
                    fg->AddText(textPos,IM_COL32(226,232,240,255),hint);
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(4);
    }

    void    FEViewPad::stepAnimation(FECamera& camera,real dt)
    {
        if (!_animating)
            return;
        _animT  +=  dt / kAnimDuration;
        _animT  =   std::min(real(1.0),_animT);
        const   real    k   =   smoothStep(_animT);

        const   real3   target  =   _animTarget0 + (_animTarget1 - _animTarget0) * k;
        real3           d0      =   _animEye0 - _animTarget0;
        real3           d1      =   _animEye1 - _animTarget1;
        const   real    len0    =   length(d0);
        const   real    len1    =   length(d1);
        d0  =   (len0 > real(1e-9)) ? d0 / len0 : real3(0,1,0);
        d1  =   (len1 > real(1e-9)) ? d1 / len1 : d0;

        const   real    cosA    =   std::max(real(-1.0),std::min(real(1.0),dot(d0,d1)));
        const   real    angle   =   std::acos(cosA);
        real3           axis    =   cross(d0,d1);
        if (length(axis) < real(1e-9))
        {
            /// <summary>
            /// 180° 翻转时旋转轴退化,取与视线垂直的相机右方向作为转轴
            /// </summary>
            axis    =   (angle > real(1e-6)) ? cross(d0,normalize(_animUp0)) : real3(1,0,0);
            if (length(axis) < real(1e-9))
                axis    =   real3(0,1,0);
        }
        axis    =   normalize(axis);

        /// <summary>
        /// eyeDir 是「目标点 → 相机」的单位方向(由起止两端的 eye-target 插值而来),
        /// 所以相机位置是 target + eyeDir*dist;写成 target - eyeDir*dist
        /// 会把相机放到目标点的另一侧,面/角切换会变成对角另一端的视图
        /// </summary>
        real3   eyeDir  =   d0;
        if (angle > real(1e-6))
        {
            const   mat4r   rot =   FE::rotate(mat4r(1.0),angle * k,axis);
            const   real4   r   =   rot * real4(d0,real(0.0));
            eyeDir  =   normalize(real3(r.x,r.y,r.z));
        }
        const   real    dist    =   len0 + (len1 - len0) * k;
        const   real3   eye     =   target + eyeDir * dist;

        real3   up  =   _animUp0 + (_animUp1 - _animUp0) * k;
        const   real3   dirNow  =   normalize(target - eye);
        up  =   up - dirNow * dot(up,dirNow);
        if (length(up) < real(1e-9))
        {
            up  =   (std::abs(dirNow.z) < real(0.9)) ? real3(0,0,1) : real3(0,1,0);
            up  =   up - dirNow * dot(up,dirNow);
        }
        up  =   normalize(up);

        camera.setEye(eye);
        camera.setTarget(target);
        camera.setUp(up);
        camera.update();
        if (_animT >= real(1.0))
            _animating  =   false;
    }

    void    FEViewPad::beginAnimation(FECamera& camera,const real3& target,const real3& eye,const real3& up)
    {
        _animTarget0    =   camera.getTarget();
        _animEye0       =   camera.getEye();
        _animUp0        =   camera.getUp();
        _animTarget1    =   target;
        _animEye1       =   eye;
        _animUp1        =   up;
        _animT          =   0.0;
        _animating      =   true;
    }

    void    FEViewPad::snapToFace(FECamera& camera,int face)
    {
        if (face < 0 || face >= 6)
            return;
        real    dist    =   length(camera.getEye() - camera.getTarget());
        if (dist < real(0.001))
            dist    =   real(2.5);
        const   real3   target  =   camera.getTarget();
        const   real3   eye     =   target - kFaceDirs[face] * dist;
        beginAnimation(camera,target,eye,kFaceUps[face]);
    }

    void    FEViewPad::snapToCorner(FECamera& camera,int corner)
    {
        if (corner < 0 || corner >= 8)
            return;
        const   real3   dir     =   -normalize(padVertex(corner));
        /// <summary>
        /// 角视图保持世界 Z 朝屏幕上方:把 Z 投影到与视线垂直的平面
        /// </summary>
        real3           up      =   real3(0,0,1) - dir * dot(real3(0,0,1),dir);
        if (length(up) < real(1e-6))
            up  =   real3(0,1,0);
        up  =   normalize(up);
        real    dist    =   length(camera.getEye() - camera.getTarget());
        if (dist < real(0.001))
            dist    =   real(2.5);
        const   real3   target  =   camera.getTarget();
        const   real3   eye     =   target - dir * dist;
        beginAnimation(camera,target,eye,up);
    }

    void    FEViewPad::snapToDefault(FECamera& camera)
    {
        beginAnimation(camera,real3(0,0,0),real3(0,-2.5,0),real3(0,0,1));
    }

    void    FEViewPad::orbitByMouse(FECamera& camera,const ImVec2& offset,int part)
    {
        if (offset.x == 0.0f && offset.y == 0.0f)
            return;
        const   real3   pivot   =   _ctx->anchor().point();
        camera.rotateViewZByCenter(real(offset.x) * real(0.2),pivot);
        if (part != kPartRing)
            camera.rotateViewXByCenter(real(offset.y) * real(0.2),pivot);
    }
}
