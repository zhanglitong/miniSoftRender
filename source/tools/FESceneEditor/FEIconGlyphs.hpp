/// summary
/// FEIconGlyphs
/// 编辑器使用的 Font Awesome 5 Solid 码位与 .ui 图标名映射,集中定义避免各控件重复硬编码。
/// </summary>

#pragma     once

namespace   FE
{
namespace   ui
{
    /// <summary>
    /// 未登记的图标统一回退到该字形(shapes)
    /// </summary>
    constexpr   unsigned int    kIconDefault    =   0xF61F;

    /// <summary>
    /// 常用字形:节点/对象
    /// </summary>
    constexpr   const char*     kGlyphCube     =   u8"\uf1b2";
    /// <summary>
    /// 常用字形:显示
    /// </summary>
    constexpr   const char*     kGlyphEye      =   u8"\uf06e";
    /// <summary>
    /// 常用字形:隐藏
    /// </summary>
    constexpr   const char*     kGlyphEyeOff   =   u8"\uf070";
    /// <summary>
    /// 常用字形:大小写匹配(Aa)
    /// </summary>
    constexpr   const char*     kGlyphCaseMatch    =   u8"\uf031";
    /// <summary>
    /// 常用字形:全匹配
    /// </summary>
    constexpr   const char*     kGlyphWholeMatch   =   u8"\uf52c";
    /// <summary>
    /// 时间线:上一帧
    /// </summary>
    constexpr   const char*     kGlyphStepBack     =   u8"\uf048";
    /// <summary>
    /// 时间线:下一帧
    /// </summary>
    constexpr   const char*     kGlyphStepForward  =   u8"\uf051";
    /// <summary>
    /// 时间线:播放
    /// </summary>
    constexpr   const char*     kGlyphPlay         =   u8"\uf04b";
    /// <summary>
    /// 时间线:暂停
    /// </summary>
    constexpr   const char*     kGlyphPause        =   u8"\uf04c";
    /// <summary>
    /// 时间线:添加关键帧(加号)
    /// </summary>
    constexpr   const char*     kGlyphAddKey       =   u8"\uf067";

    /// <summary>
    /// 图标文件名(不含目录与扩展名)到 Font Awesome 码位的映射。
    /// 键取自 bin/data/ui/res 下的图标文件名,新增图标时在此登记即可
    /// </summary>
    struct  FEIconGlyph
    {
        /// <summary>
        /// 图标文件名(不含 .png),对应 .ui 里 :/EditorRes/res/&lt;name&gt;.png
        /// </summary>
        const   char*           _name;
        /// <summary>
        /// Font Awesome 私有区码位
        /// </summary>
        unsigned int            _code;
    };

    const   FEIconGlyph     kIconGlyphs[]   =
    {
        { "about",              0xF05A },   ///< info-circle
        { "align_back",         0xF070 },   ///< eye-slash
        { "align_bottom",       0xF063 },   ///< arrow-down
        { "align_front",        0xF06E },   ///< eye
        { "align_left",         0xF036 },   ///< align-left
        { "align_right",        0xF038 },   ///< align-right
        { "align_rotate",       0xF2F1 },   ///< sync-alt
        { "align_scale",        0xF31E },   ///< expand-arrows-alt
        { "align_top",          0xF062 },   ///< arrow-up
        { "align_x",            0xF337 },   ///< arrows-alt-h
        { "align_y",            0xF338 },   ///< arrows-alt-v
        { "align_z",            0xF14E },   ///< compass
        { "bbox_center",        0xF140 },   ///< bullseye
        { "cone_frustum",       0xF1CE },   ///< circle-notch
        { "cover.reset",        0xF2EA },   ///< undo-alt
        { "cube",               0xF1B2 },   ///< cube
        { "cylinder",           0xF1C0 },   ///< database
        { "hyperboloid",        0xF5FD },   ///< layer-group
        { "ico_FirstFrame",     0xF048 },   ///< step-backward
        { "ico_LastFrame",      0xF051 },   ///< step-forward
        { "ico_Move",           0xF0B2 },   ///< arrows-alt
        { "ico_NextFrame",      0xF04E },   ///< forward
        { "ico_Pause",          0xF04C },   ///< pause
        { "ico_PreFrame",       0xF04A },   ///< backward
        { "ico_Redo",           0xF01E },   ///< redo
        { "ico_Rotate",         0xF021 },   ///< refresh
        { "ico_RunBack",        0xF049 },   ///< fast-backward
        { "ico_RunFront",       0xF050 },   ///< fast-forward
        { "ico_Scale",          0xF065 },   ///< expand
        { "ico_Undo",           0xF0E2 },   ///< undo
        { "mark",               0xF3C5 },   ///< map-marker-alt
        { "maximum",            0xF102 },   ///< angle-double-up
        { "minimum",            0xF103 },   ///< angle-double-down
        { "model_load",         0xF56F },   ///< file-import
        { "node_close",         0xF00D },   ///< times
        { "pyramid_frustum",    0xF61F },   ///< shapes
        { "scene_open",         0xF07C },   ///< folder-open
        { "scene_save",         0xF0C7 },   ///< save
        { "sphere",             0xF111 },   ///< circle
        { "spherical_cap",      0xF192 },   ///< dot-circle
        { "square_pyramid",     0xF0D8 },   ///< caret-up
        { "torus",              0xF1CE },   ///< circle-notch
        { "torus_knot",         0xF126 },   ///< code-branch
        { "torus_sector",       0xF200 },   ///< chart-pie
        { "torus_solid",        0xF111 },   ///< circle
    };
}
}
