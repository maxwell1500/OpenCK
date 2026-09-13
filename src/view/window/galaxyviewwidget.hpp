#ifndef GALAXYVIEWWIDGET_HPP
#define GALAXYVIEWWIDGET_HPP

#include <QWidget>

#include "../../model/tools/galaxymap.hpp"

// Galaxy view (REMAINING.md §3.8): renders a GalaxyMap's star systems as
// circles positioned in galaxy space, with the selected system highlighted
// and its planet count shown. Clicking selects the nearest system.
class GalaxyViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GalaxyViewWidget(QWidget* parent = nullptr);

    void setMap(const GalaxyMap& map);
    GalaxyMap galaxyMap() const { return m_map; }

    int selectedSystem() const { return m_selected; }
    void setSelectedSystem(int index);

signals:
    void systemSelected(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QPointF toScreen(double x, double y) const;
    int pickSystem(const QPoint& pos) const;

    GalaxyMap m_map;
    int m_selected = -1;
};

#endif // GALAXYVIEWWIDGET_HPP
