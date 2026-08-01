#ifndef SCENETIMELINEWIDGET_H
#define SCENETIMELINEWIDGET_H

#include <QWidget>
#include <QVector>

#include "../../../model/tools/scenephasemodel.hpp"

class QPainter;
class QMouseEvent;
class QPushButton;

// SceneTimelineWidget is the editor widget for a scene (SCEN) phase
// timeline. It renders each phase as a colored block along a time axis,
// lets the user drag a phase's right edge to resize it (clamped to its
// neighbours via ScenePhaseModel), and adds/removes phases with buttons.
// The phase data is owned externally (the record); this widget mutates it
// through the shared ScenePhaseModel functions.
class SceneTimelineWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneTimelineWidget(QVector<ScenePhase>* phases,
                                 QWidget* parent = nullptr);
    ~SceneTimelineWidget() override;

    void setPhases(QVector<ScenePhase>* phases);
    void setOwnedPhases(QVector<ScenePhase>* phases);

    QSize minimumSizeHint() const override;

signals:
    void phasesChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int xForTime(double time) const;
    double timeForX(int x) const;
    int phaseAtX(int x) const;
    int phaseResizeHandleAtX(int x) const;
    void relayout();

    QVector<ScenePhase>* m_phases;
    bool m_ownsPhases;
    int m_dragIndex;
    bool m_resizing;
    double m_dragStartTime;
    int m_dragStartX;
    int m_clickY;
    double m_maxTime;

    QPushButton* m_addButton;
    QPushButton* m_removeButton;
};

#endif // SCENETIMELINEWIDGET_H
