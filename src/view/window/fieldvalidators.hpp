#ifndef FIELDVALIDATORS_HPP
#define FIELDVALIDATORS_HPP

#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QObject>

inline void setHexFormIdValidator(QLineEdit* edit, QObject* parent)
{
    edit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("0x[0-9A-Fa-f]{0,8}"), parent));
    edit->setMaxLength(10);
}

inline void setFormIdValidator(QLineEdit* edit, QObject* parent)
{
    edit->setValidator(new QRegularExpressionValidator(
        QRegularExpression("0x[0-9A-Fa-f]{8}"), parent));
    edit->setMaxLength(10);
}

inline void setIntRangeValidator(QSpinBox* spin, int min, int max, int singleStep = 1)
{
    if (spin) {
        spin->setRange(min, max);
        spin->setSingleStep(singleStep);
    }
}

inline void setDoubleRangeValidator(QDoubleSpinBox* spin, double min, double max, int decimals = 2, double singleStep = 0.1)
{
    if (spin) {
        spin->setRange(min, max);
        spin->setDecimals(decimals);
        spin->setSingleStep(singleStep);
    }
}

inline void setPositionValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, -999999.0, 999999.0, 3, 10.0);
}

inline void setRotationValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 360.0, 2, 0.1);
}

inline void setScaleValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 10.0, 2, 0.1);
}

inline void setHealthValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 65535, 10);
}

inline void setMagickaValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 65535, 10);
}

inline void setStaminaValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 65535, 10);
}

inline void setDamageValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 99999.0, 1, 10.0);
}

inline void setSpeedValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.1, 10.0, 2, 0.1);
}

inline void setReachValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.1, 10.0, 2, 0.1);
}

inline void setWeightValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 9999.0, 2, 10.0);
}

inline void setValueValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, -999999, 999999, 10);
}

inline void setArmorRatingValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 9999, 1);
}

inline void setCostValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 999999, 1);
}

inline void setMagnitudeValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 9999.0, 2, 1.0);
}

inline void setDurationValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 999999, 1);
}

inline void setAreaValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 999999, 1);
}

inline void setLevelValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 65535, 1);
}

inline void setLockLevelValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 100, 1);
}

inline void setChargesValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 9999, 1);
}

inline void setAttackTypeValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 9999, 1);
}

inline void setPercentageValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 100.0, 2, 1.0);
}

inline void setAngleValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, -360.0, 360.0, 2, 0.1);
}

inline void setFloatPositiveValidator(QDoubleSpinBox* spin)
{
    setDoubleRangeValidator(spin, 0.0, 999999.0, 4, 0.01);
}

inline void setIntNonNegativeValidator(QSpinBox* spin)
{
    setIntRangeValidator(spin, 0, 999999, 1);
}

#endif // FIELDVALIDATORS_HPP
