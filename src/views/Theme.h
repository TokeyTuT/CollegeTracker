#ifndef THEME_H
#define THEME_H

#include <QString>

// College Tracker 统一设计 Token
// 方向：安静的校园档案室。暖纸色画布、墨绿色主色、陶土橙强调。

namespace Theme {

// --------------- 排版（Modular Scale: base 16 px, Major Third 1.250）--------
namespace TypeScale {
    constexpr int display = 32;
    constexpr int h1       = 26;
    constexpr int h2       = 19;
    constexpr int body     = 15;
    constexpr int caption  = 12;
} // namespace TypeScale

namespace FontWeight {
    constexpr int regular  = 400;  // 正文
    constexpr int medium   = 500;  // 标签
    constexpr int demiBold = 630;  // 小标题
    constexpr int bold     = 700;  // 标题
    constexpr int heavy    = 800;  // 页面顶部标题
} // namespace FontWeight

namespace Color {
    constexpr auto primary        = "#1F6B5B";
    constexpr auto primaryHover   = "#174F44";
    constexpr auto primaryBg      = "#DDEBE6";

    constexpr auto surface        = "#FFFEFA";
    constexpr auto surfaceVariant = "#ECE8DF";
    constexpr auto background     = "#F4F1EA";
    constexpr auto sidebarBg      = "#16352F";

    constexpr auto onSurface      = "#17201D";
    constexpr auto onSurfaceVar   = "#59635F";
    constexpr auto onSurfaceMuted = "#8C938F";

    constexpr auto accent         = "#D97745";
    constexpr auto accentLight    = "#F7E4D8";

    constexpr auto success        = "#43846F";
    constexpr auto warning        = "#C98932";
    constexpr auto error          = "#B94B45";
    constexpr auto errorBg        = "#F8E7E4";

    constexpr auto outline        = "#D6D0C4";
    constexpr auto outlineVar     = "#E9E4DA";
    constexpr auto headerBg       = "#E7E2D8";
} // namespace Color

// --------------- 间距 ---------------
namespace Spacing {
    constexpr int xs  = 4;
    constexpr int sm  = 9;
    constexpr int md  = 14;
    constexpr int lg  = 22;
    constexpr int xl  = 34;
    constexpr int xxl = 48;
} // namespace Spacing

// --------------- 圆角 ---------------
namespace Radius {
    constexpr int sm   = 7;
    constexpr int md   = 10;
    constexpr int lg   = 14;
    constexpr int xl   = 20;
    constexpr int pill = 999;
} // namespace Radius

// --------------- 通用 Stylesheet 片段（由各对话框按需引用）--------------
inline QString inputStyle() {
    return QStringLiteral(
        "QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox {"
        "  border: 1px solid %1; border-radius: %2px;"
        "  padding: 0 12px; min-height: 40px;"
        "  background: %3; color: %4; font-size: %5px;"
        "}"
        "QLineEdit:hover, QComboBox:hover, QDateEdit:hover, QDoubleSpinBox:hover {"
        "  border-color: %6;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDoubleSpinBox:focus {"
        "  border: 2px solid %6;"
        "}"
        "QComboBox::drop-down, QDateEdit::drop-down { border: none; width: 30px; }"
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
        "QPushButton:pressed { background: #113F36; }"
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
