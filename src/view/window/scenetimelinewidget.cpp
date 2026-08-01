#include "scenetimelinewidget.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

namespace {
constexpr int kRowHeight = 36;
constexpr int kLeftMargin = 56;    // time axis labels
constexpr int kResizeHandle = 6;   // px at a phase's right edge
constexpr int kPhaseColors[] = {
    0x4A90D9, 0x50B86C, 0xD9823B, 0xB84AD9, 0xD94A4A,
    0x4AC7D9, 0x9BD94A, 0xD9C14A, 0x8A6FD9, 0xD96AA0,
};
}

SceneTimelineWidget::SceneTimelineWidget(QVector<ScenePhase>* phases,
                                         QWidget* parent)
    : QWidget(parent)
    , m_phases(phases)
    , m_ownsPhases(false)
    , m_dragIndex(-1)
    , m_resizing(false)
    , m_dragStartTime(0.0)
    , m_dragStartX(0)
    , m_clickY(0)
    , m_maxTime(10.0)
{
    setMinimumHeight(kRowHeight + 30);
    setMouseTracking(true);

    auto* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    auto* buttonRow = new QHBoxLayout();
    m_addButton = new QPushButton(tr("Add Phase"), this);
    m_removeButton = new QPushButton(tr("Remove Phase"), this);
    buttonRow->addWidget(m_addButton);
    buttonRow->addWidget(m_removeButton);
    buttonRow->addStretch();
    auto* hint = new QLabel(tr("Drag a phase's right edge to resize; drag the body to move it."), this);
    buttonRow->addWidget(hint);
    topLayout->addLayout(buttonRow);

    connect(m_addButton, &QPushButton::clicked, this, [this]() {
        if (!m_phases)
            return;
        ScenePhase phase;
        phase.name = tr("Phase %1").arg(m_phases->size() + 1);
        phase.startTime = m_phases->isEmpty() ? 0.0 : m_phases->last().endTime;
        phase.endTime = phase.startTime + 1.0;
        ScenePhaseModel::insertPhase(*m_phases, m_phases->size(), phase);
        emit phasesChanged();
        update();
    });
    connect(m_removeButton, &QPushButton::clicked, this, [this]() {
        if (!m_phases || m_phases->isEmpty())
            return;
        ScenePhaseModel::removePhase(*m_phases, m_phases->size() - 1);
        emit phasesChanged();
        update();
    });

    relayout();
}

void SceneTimelineWidget::setPhases(QVector<ScenePhase>* phases)
{
    m_phases = phases;
    relayout();
    update();
}

void SceneTimelineWidget::setOwnedPhases(QVector<ScenePhase>* phases)
{
    m_phases = phases;
    m_ownsPhases = true;
    relayout();
    update();
}

SceneTimelineWidget::~SceneTimelineWidget()
{
    if (m_ownsPhases)
        delete m_phases;
}

QSize SceneTimelineWidget::minimumSizeHint() const
{
    return QSize(400, kRowHeight + 30);
}

void SceneTimelineWidget::relayout()
{
    double maxEnd = 1.0;
    if (m_phases)
        for (const ScenePhase& p : *m_phases)
            maxEnd = qMax(maxEnd, p.endTime);
    m_maxTime = qMax(1.0, maxEnd * 1.2);
}

int SceneTimelineWidget::xForTime(double time) const
{
    const int width = qMax(1, this->width() - kLeftMargin - 8);
    return kLeftMargin + static_cast<int>(time / m_maxTime * width);
}

double SceneTimelineWidget::timeForX(int x) const
{
    const int width = qMax(1, this->width() - kLeftMargin - 8);
    return static_cast<double>(qBound(kLeftMargin, x, kLeftMargin + width) - kLeftMargin)
        / width * m_maxTime;
}

int SceneTimelineWidget::phaseAtX(int x) const
{
    if (!m_phases)
        return -1;
    const int top = 40;
    for (int i = 0; i < m_phases->size(); ++i)
    {
        const double start = xForTime(m_phases->at(i).startTime);
        const double end = xForTime(m_phases->at(i).endTime);
        if (x >= start - 2 && x <= end + 2)
        {
            // Only pick it up when the click is on the row band.
            if (m_clickY >= top && m_clickY <= top + kRowHeight)
                return i;
        }
    }
    return -1;
}

int SceneTimelineWidget::phaseResizeHandleAtX(int x) const
{
    if (!m_phases)
        return -1;
    const int top = 40;
    if (m_clickY < top || m_clickY > top + kRowHeight)
        return -1;
    for (int i = 0; i < m_phases->size(); ++i)
    {
        const int end = xForTime(m_phases->at(i).endTime);
        if (qAbs(x - end) <= kResizeHandle)
            return i;
    }
    return -1;
}

void SceneTimelineWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 34));

    const int top = 40;
    const int rowHeight = kRowHeight;

    // Time axis.
    p.setPen(QColor(160, 160, 170));
    p.setFont(QFont(font().family(), 8));
    for (int t = 0; t <= static_cast<int>(m_maxTime); ++t)
    {
        const int x = xForTime(t);
        p.drawLine(x, top - 6, x, top - 2);
        p.drawText(x - 12, top - 12, 24, 12, Qt::AlignHCenter, QString::number(t));
    }

    if (!m_phases)
        return;

    for (int i = 0; i < m_phases->size(); ++i)
    {
        const ScenePhase& phase = m_phases->at(i);
        const int x0 = xForTime(phase.startTime);
        const int x1 = xForTime(phase.endTime);
        const QColor color(kPhaseColors[i % (sizeof(kPhaseColors) / sizeof(int))]);

        p.fillRect(x0, top, qMax(2, x1 - x0), rowHeight, color);
        p.setPen(color.lighter(140));
        p.drawRect(x0, top, qMax(2, x1 - x0), rowHeight);

        // Resize handle.
        p.fillRect(x1 - kResizeHandle, top, kResizeHandle, rowHeight, color.darker(130));

        // Label.
        p.setPen(Qt::white);
        p.drawText(x0 + 4, top, qMax(8, x1 - x0 - 12), rowHeight,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("%1  [%2..%3]")
                       .arg(phase.name)
                       .arg(phase.startTime, 0, 'f', 2)
                       .arg(phase.endTime, 0, 'f', 2));
    }

    p.setPen(QColor(80, 80, 90));
    p.drawLine(kLeftMargin, top - 8, kLeftMargin, top + rowHeight + 4);
    p.drawLine(kLeftMargin, top + rowHeight + 4, width() - 4, top + rowHeight + 4);
}

void SceneTimelineWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_clickY = event->pos().y();
    const int handleIndex = phaseResizeHandleAtX(event->pos().x());
    if (handleIndex >= 0)
    {
        m_dragIndex = handleIndex;
        m_resizing = true;
        m_dragStartTime = m_phases->at(handleIndex).endTime;
        m_dragStartX = event->pos().x();
        setCursor(Qt::SizeHorCursor);
        return;
    }
    const int phaseIndex = phaseAtX(event->pos().x());
    if (phaseIndex >= 0)
    {
        m_dragIndex = phaseIndex;
        m_resizing = false;
        m_dragStartTime = m_phases->at(phaseIndex).startTime;
        m_dragStartX = event->pos().x();
        setCursor(Qt::ClosedHandCursor);
    }
}

void SceneTimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragIndex < 0 || !m_phases || m_dragIndex >= m_phases->size())
    {
        // Hover: show the resize cursor over a phase edge.
        m_clickY = event->pos().y();
        setCursor(phaseResizeHandleAtX(event->pos().x()) >= 0
                      ? Qt::SizeHorCursor : Qt::ArrowCursor);
        return;
    }

    const double dt = (event->pos().x() - m_dragStartX)
        * (m_maxTime / qMax(1, width() - kLeftMargin - 8));

    if (m_resizing)
    {
        const double newEnd = m_dragStartTime + dt;
        ScenePhase phase = m_phases->at(m_dragIndex);
        const double minEnd = phase.startTime + 0.05;
        const double maxEnd = (m_dragIndex + 1 < m_phases->size())
            ? m_phases->at(m_dragIndex + 1).startTime
            : m_maxTime;
        phase.endTime = qBound(minEnd, newEnd, maxEnd);
        (*m_phases)[m_dragIndex] = phase;
        emit phasesChanged();
        update();
    }
    else
    {
        ScenePhaseModel::movePhaseStart(*m_phases, m_dragIndex,
                                        m_dragStartTime + dt);
        emit phasesChanged();
        update();
    }
}

void SceneTimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_dragIndex = -1;
    m_resizing = false;
    setCursor(Qt::ArrowCursor);
}
