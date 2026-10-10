#include "gamepadnavigator.hpp"

#include "nifviewportwidget.hpp"

#include <QTimer>
#include <QLoggingCategory>

#include "../../libs/files/log/logger.hpp"

#if defined(OPENCK_WITH_QT_GAMEPAD)
#  include <QGamepadManager>
#endif

namespace
{
// Tunable mapping constants. Pan/orbit are per-second rates at full stick
// deflection; they scale linearly with axis magnitude so fine aim works.
constexpr qreal PanUnitsPerSecond     = 40.0;
constexpr qreal OrbitDegreesPerSecond = 120.0;
constexpr qreal ZoomFactorPerSecond   = 3.0;
constexpr qreal DeadZone              = 0.12;
} // namespace

GamepadNavigator::GamepadNavigator(QObject* parent)
    : QObject(parent)
{
#if defined(OPENCK_WITH_QT_GAMEPAD)
    // Gamepad 0 is the first connected device; the manager reports -1 when
    // nothing is attached, and it emits when that changes.
    const int deviceId = QGamepadManager::instance()->connectedGamepads().value(0, -1);
    if (deviceId >= 0)
    {
        mGamepad = new QGamepad(deviceId, this);
        mDeviceConnected = true;
        LOG_INFO(QStringLiteral("GamepadNavigator: attached to gamepad %1").arg(deviceId));
    }
    else
    {
        LOG_INFO(QStringLiteral("GamepadNavigator: no gamepad connected"));
    }
#else
    LOG_INFO(QStringLiteral("GamepadNavigator: Qt Gamepad module not built; gamepad navigation disabled"));
#endif
}

GamepadNavigator::~GamepadNavigator() = default;

bool GamepadNavigator::isAvailable() const
{
#if defined(OPENCK_WITH_QT_GAMEPAD)
    return mGamepad != nullptr && mDeviceConnected;
#else
    return false;
#endif
}

QString GamepadNavigator::backendName() const
{
#if defined(OPENCK_WITH_QT_GAMEPAD)
    return QStringLiteral("Qt Gamepad");
#else
    return QStringLiteral("unavailable (Qt Gamepad not built)");
#endif
}

void GamepadNavigator::setViewport(NifViewportWidget* viewport)
{
    mViewport = viewport;
}

NifViewportWidget* GamepadNavigator::viewport() const
{
    return mViewport;
}

void GamepadNavigator::advance(qreal dtSeconds)
{
    if (!mViewport || dtSeconds <= 0.0) {
        return;
    }

#if defined(OPENCK_WITH_QT_GAMEPAD)
    if (!mGamepad || !mDeviceConnected) {
        return;
    }

    // Radial dead zone: an axis that is nearly centred contributes nothing, so
    // resting the thumb never drifts the camera.
    auto axis = [](double raw) -> qreal {
        const qreal v = qBound(-1.0, raw, 1.0);
        if (qAbs(v) < DeadZone)
            return 0.0;
        return (v - (v < 0 ? -DeadZone : DeadZone)) / (1.0 - DeadZone);
    };

    const qreal leftX  = axis(mGamepad->axisLeftX());
    const qreal leftY  = axis(mGamepad->axisLeftY());
    const qreal rightX = axis(mGamepad->axisRightX());
    const qreal rightY = axis(mGamepad->axisRightY());
    const qreal l2     = qBound(0.0, mGamepad->buttonL2() ? 1.0 : 0.0, 1.0);
    const qreal r2     = qBound(0.0, mGamepad->buttonR2() ? 1.0 : 0.0, 1.0);

    // Left stick: pan in the view plane. Screen-down is +Y in game space.
    if (leftX != 0.0 || leftY != 0.0)
    {
        mViewport->nudgeCamera(leftX * PanUnitsPerSecond * dtSeconds,
                               -leftY * PanUnitsPerSecond * dtSeconds,
                               0.0f);
    }

    // Right stick: orbit. Pitch is clamped inside orbitCamera().
    if (rightX != 0.0 || rightY != 0.0)
    {
        mViewport->orbitCamera(static_cast<float>(-rightY * OrbitDegreesPerSecond * dtSeconds),
                               static_cast<float>(rightX * OrbitDegreesPerSecond * dtSeconds));
    }

    // Triggers: L2/R2 raise/lower the camera (vertical pan).
    if (l2 != r2)
    {
        mViewport->nudgeCamera(0.0f, 0.0f, static_cast<float>((r2 - l2) * PanUnitsPerSecond * dtSeconds));
    }

    // Buttons: edge-triggered actions poll here because QGamepad has no
    // convenient per-button signal that the navigator needs to subscribe to
    // individually.
    static bool aHeld = false;
    static bool bHeld = false;
    static bool xHeld = false;
    const bool aDown = mGamepad->buttonA();
    const bool bDown = mGamepad->buttonB();
    const bool xDown = mGamepad->buttonX();
    if (aDown && !aHeld)
        mViewport->resetCamera();
    if (bDown && !bHeld)
        mViewport->zoomCamera(ZoomFactorPerSecond * dtSeconds + 1.0f);
    if (xDown && !xHeld)
        mViewport->zoomCamera(1.0f / (ZoomFactorPerSecond * dtSeconds + 1.0f));
    aHeld = aDown;
    bHeld = bDown;
    xHeld = xDown;
#endif
}
