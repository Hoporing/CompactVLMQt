import QtQuick 2.15
import QtQuick.Controls 2.15

// Shared styled button component
Button {
    id: root

    property color normalColor: Theme.btnNormal
    property color hoverColor:  Theme.btnHover
    property color pressColor:  Theme.btnPress
    property bool  compact:     false

    implicitHeight: compact ? 25 : 34
    implicitWidth:  contentItem.implicitWidth + (compact ? 16 : 24)

    contentItem: Text {
        text:                  root.text
        font.family:           Theme.fontFamily
        font.pixelSize:        compact ? Theme.fontSizeSmall : Theme.fontSizeNormal
        color:                 root.enabled ? Theme.textPrimary : Theme.textDisabled
        horizontalAlignment:   Text.AlignHCenter
        verticalAlignment:     Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: {
            if (!root.enabled)       return Theme.bgLight
            if (root.pressed)        return root.pressColor
            if (root.hovered)        return root.hoverColor
            return root.normalColor
        }
        border.color: root.hovered ? Theme.accent : "transparent"
        border.width: 1

        Behavior on color { ColorAnimation { duration: 100 } }
    }
}
