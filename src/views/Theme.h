#ifndef THEME_H
#define THEME_H

#include <QString>

// ============================================================================
// College Tracker 统一设计 Token — 基于 Modular Scale (base 16, ratio 1.250)
//
// 目标平台：桌面 (~60 cm 视距)
// 设计语言：Material Design 3 角色映射
// 主色系：Slate/Teal/Cyan — 专业、可信赖、教育感
// ============================================================================

namespace Theme {

// --------------- 排版（Modular Scale: base 16 px, Major Third 1.250）--------
namespace TypeScale {
    constexpr int display = 31;  // ms(3): 英雄标题 / 登录页主标题
    constexpr int h1       = 25;  // ms(2): 页面标题 / currentPageLbl
    constexpr int h2       = 20;  // ms(1): 区块标题 / Card 标题
    constexpr int body     = 16;  // ms(0): 正文 — 基准 16 px（WCAG 建议 ≥ 16px）
    constexpr int caption  = 13;  // ms(-1): 标签 / metadata / 辅助文本
} // namespace TypeScale

namespace FontWeight {
    constexpr int regular  = 400;  // 正文
    constexpr int medium   = 500;  // 标签
    constexpr int demiBold = 630;  // 小标题
    constexpr int bold     = 700;  // 标题
    constexpr int heavy    = 800;  // 页面顶部标题
} // namespace FontWeight

// --------------- 语义颜色 Token (light mode) ---------------
namespace Color {
    // 品牌色（Teal-Cyan 渐变）
    constexpr auto primary       = "#0D9488";  // teal-600: 主按钮 / 链接
    constexpr auto primaryHover  = "#0F766E";  // teal-700
    constexpr auto primaryBg     = "#F0FDFA";  // teal-50: 浅底色

    // Surface
    constexpr auto surface       = "#FFFFFF";
    constexpr auto surfaceVariant = "#F8FAFC";  // slate-50
    constexpr auto background    = "#F1F5F9";   // slate-100
    constexpr auto sidebarBg     = "#FBFDFE";

    // 文字
    constexpr auto onSurface     = "#0F172A";   // slate-900: 正文
    constexpr auto onSurfaceVar  = "#475569";   // slate-600: 辅助文字
    constexpr auto onSurfaceMuted = "#94A3B8";  // slate-400: placeholder

    // 强调色
    constexpr auto accent        = "#06B6D4";   // cyan-500: 高亮边框 / 选中态
    constexpr auto accentLight   = "#ECFEFF";   // cyan-50

    // 语义色
    constexpr auto success       = "#10B981";   // emerald-500
    constexpr auto warning       = "#F59E0B";   // amber-500
    constexpr auto error         = "#EF4444";   // red-500
    constexpr auto errorBg       = "#FEF2F2";   // red-50

    // 边框
    constexpr auto outline       = "#CBD5E1";   // slate-300
    constexpr auto outlineVar    = "#E2E8F0";   // slate-200

    // 表头
    constexpr auto headerBg      = "#1E293B";   // slate-800
} // namespace Color

// --------------- 间距 ---------------
namespace Spacing {
    constexpr int xs  = 4;
    constexpr int sm  = 8;
    constexpr int md  = 14;
    constexpr int lg  = 22;
    constexpr int xl  = 32;
    constexpr int xxl = 48;
} // namespace Spacing

// --------------- 圆角 ---------------
namespace Radius {
    constexpr int sm   = 4;
    constexpr int md   = 8;
    constexpr int lg   = 14;
    constexpr int xl   = 24;
    constexpr int pill = 999;
} // namespace Radius

// --------------- 通用 Stylesheet 片段（由各对话框按需引用）--------------
inline QString inputStyle() {
    return QStringLiteral(
        "QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox {"
        "  border: 1px solid %1; border-radius: %2px;"
        "  padding: 0 12px; min-height: 36px;"
        "  background: %3; color: %4; font-size: %5px;"
        "}"
        "QLineEdit:hover, QComboBox:hover, QDateEdit:hover, QDoubleSpinBox:hover {"
        "  border-color: %6;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDoubleSpinBox:focus {"
        "  border: 2px solid %6;"
        "}"
        "QComboBox::drop-down, QDateEdit::drop-down { border: none; width: 28px; }"
    ).arg(Color::outline)
     .arg(Radius::md)
     .arg(Color::surface)
     .arg(Color::onSurface)
     .arg(TypeScale::body)
     .arg(Color::accent);
}

inline QString primaryBtnStyle() {
    return QStringLiteral(
        "QPushButton {"
        "  background: %1; color: #FFFFFF; border: none;"
        "  border-radius: %2px; min-height: 40px;"
        "  font-size: %3px; font-weight: %4; padding: 0 24px;"
        "}"
        "QPushButton:hover { background: %5; }"
        "QPushButton:pressed { background: #0B5E57; }"
    ).arg(Color::primary)
     .arg(Radius::md)
     .arg(TypeScale::body)
     .arg(FontWeight::bold)
     .arg(Color::primaryHover);
}

inline QString secondaryBtnStyle() {
    return QStringLiteral(
        "QPushButton {"
        "  background: transparent; color: %1;"
        "  border: 2px solid %1; border-radius: %2px;"
        "  min-height: 40px; font-size: %3px; font-weight: %4; padding: 0 24px;"
        "}"
        "QPushButton:hover { background: %5; }"
    ).arg(Color::primary)
     .arg(Radius::md)
     .arg(TypeScale::body)
     .arg(FontWeight::medium)
     .arg(Color::primaryBg);
}

inline QString dangerBtnStyle() {
    return QStringLiteral(
        "QPushButton {"
        "  background: %1; color: %2;"
        "  border: 1px solid %2; border-radius: %3px;"
        "  min-height: 36px; font-size: %4px; font-weight: %5; padding: 0 20px;"
        "}"
        "QPushButton:hover { background: %2; color: #FFFFFF; }"
    ).arg(Color::surface)
     .arg(Color::error)
     .arg(Radius::sm)
     .arg(TypeScale::caption)
     .arg(FontWeight::bold);
}

inline QString dialogStyle() {
    return QStringLiteral(
        "QDialog {"
        "  background: %1;"
        "}"
    ).arg(Color::surfaceVariant);
}

inline QString labelStyle() {
    return QStringLiteral(
        "font-size: %1px; color: %2; font-weight: %3;"
    ).arg(TypeScale::caption)
     .arg(Color::onSurfaceVar)
     .arg(FontWeight::medium);
}

inline QString headingStyle() {
    return QStringLiteral(
        "font-size: %1px; color: %2; font-weight: %3;"
    ).arg(TypeScale::h1)
     .arg(Color::onSurface)
     .arg(FontWeight::heavy);
}

} // namespace Theme

#endif // THEME_H
