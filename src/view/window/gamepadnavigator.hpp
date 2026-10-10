#ifndef GAMEPADNAVIGATOR_HPP
#define GAMEPADNAVIGATOR_HPP

#include <QObject>

class NifViewportWidget;

#if defined(OPENCK_WITH_QT_GAMEPAD)
#  include <QGamepad>
#endif

// GamepadNavigator maps a connected gamepad onto render-window camera motion.
//
// Steam Deck hands you thumbsticks and trackpads where the Creation Kit
// expects a mouse, so the mapping is the important part:
//   left stick   -> pan the camera in the view plane
//   right stick  -> orbit (pitch + yaw)
//   triggers     -> zoom (both) / raise-lower (L2+R2 as Z pan)
//   A / B / X    -> reset / zoom in / zoom out
//
// Qt's Gamepad module is optional: without it the navigator compiles to a
// null object that reports isAvailable() == false, and nothing else changes.
class GamepadNavigator : public QObject
{
    Q_OBJECT

public:
    explicit GamepadNavigator(QObject* parent = nullptr);
    ~GamepadNavigator() override;

    /// True when a usable gamepad backend is compiled in AND a device answered.
    bool isAvailable() const;
    /// Human-readable backend name for the preferences page ("Qt Gamepad").
    QString backendName() const;

    void setViewport(NifViewportWidget* viewport);
    NifViewportWidget* viewport() const;

    /// Feed the current axis state. Called from a timer by the owner so the
    /// motion is frame-rate independent; @p dtSeconds is the elapsed time.
    void advance(qreal dtSeconds);

private:
#if defined(OPENCK_WITH_QT_GAMEPAD)
    QGamepad* mGamepad = nullptr;
#endif
    NifViewportWidget* mViewport = nullptr;
    bool mDeviceConnected = false;
};

#endif // GAMEPADNAVIGATOR_HPP
