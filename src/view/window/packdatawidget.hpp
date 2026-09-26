#ifndef PACKDATAWIDGET_HPP
#define PACKDATAWIDGET_HPP

#include "../widgets/formdatawidget.hpp"

#include <QWidget>

namespace openck {
class FormComponents;
}

class PackDataWidget : public QWidget, public openck::FormDataWidget
{
    Q_OBJECT

public:
    explicit PackDataWidget(void* recordPtr, openck::FormComponents* components,
                            QWidget* parent = nullptr);
    ~PackDataWidget() override;

    void loadSession() override;
    bool validateSession(QString* error) override;
    void applySession() override;

private:
    void* m_recordPtr;
};

#endif // PACKDATAWIDGET_HPP
