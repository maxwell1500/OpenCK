#ifndef LOCATIONDATAWIDGET_HPP
#define LOCATIONDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

namespace openck {
class FormComponents;
}

class LocationDataWidget : public QWidget, public openck::FormDataWidget
{
    Q_OBJECT

public:
    explicit LocationDataWidget(void* recordPtr, openck::FormComponents* components,
                                QWidget* parent = nullptr);
    ~LocationDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

#endif // LOCATIONDATAWIDGET_HPP
