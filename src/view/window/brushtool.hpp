#ifndef BRUSHTOOL_HPP
#define BRUSHTOOL_HPP

#include <QObject>

class BrushTool : public QObject
{
    Q_OBJECT

public:
    explicit BrushTool(QObject* parent = nullptr);

    void beginStroke();
    void endStroke();
    void notifyStrokeApplied();

signals:
    void strokeApplied();
    void strokeFinished();

private:
    bool mActive;
};

#endif // BRUSHTOOL_HPP