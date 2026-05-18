pragma Singleton
import QtQuick 2.15

// Dark theme colors and font constants
QtObject {
    // ── Backgrounds ───────────────────────────────────────────
    readonly property color bgDark:    "#2B2B2B"
    readonly property color bgMedium:  "#333333"
    readonly property color bgLight:   "#3C3C3C"
    readonly property color bgPanel:   "#252525"

    // ── Text ──────────────────────────────────────────────────
    readonly property color textPrimary:   "#FFFFFF"
    readonly property color textSecondary: "#BBBBBB"
    readonly property color textDisabled:  "#666666"

    // ── Buttons ───────────────────────────────────────────────
    readonly property color btnNormal: "#367BEB"
    readonly property color btnHover:  "#5CBCE6"
    readonly property color btnPress:  "#5CE6E3"
    readonly property color btnDanger: "#C0392B"

    // ── Accent colors ─────────────────────────────────────────
    readonly property color accent:    "#5B9BD5"
    readonly property color accentAlt: "#2ECC71"
    readonly property color warning:   "#F39C12"
    readonly property color error:     "#E74C3C"

    // ── Chat bubbles ──────────────────────────────────────────
    readonly property color bubbleUser:      "#1A3A6B"
    readonly property color bubbleAssistant: "#121212"
    readonly property color bubbleBorder:    "#404040"

    // ── Divider ───────────────────────────────────────────────
    readonly property color divider: "#404040"

    // ── Input ─────────────────────────────────────────────────
    readonly property color inputBg:     "#1E1E1E"
    readonly property color inputBorder: "#505050"

    // ── Fonts ─────────────────────────────────────────────────
    readonly property int fontSizeSmall:  11
    readonly property int fontSizeNormal: 13
    readonly property int fontSizeLarge:  15
    readonly property int fontSizeTitle:  18

    readonly property string fontFamily: "나눔고딕"

    // ── Menu ──────────────────────────────────────────────────
    readonly property int menuBarHeight:  24
    readonly property int menuItemWidth: 100
    readonly property int menuItemHeight: 24

    // ── Layout ────────────────────────────────────────────────
    readonly property int radiusSmall:  4
    readonly property int radiusNormal: 8
    readonly property int radiusLarge:  12

    readonly property int marginSmall:  6
    readonly property int marginNormal: 10
    readonly property int marginLarge:  16

    readonly property int panelMinWidth: 320
    readonly property int panelMaxWidth: 600
}
