#ifndef OPALPLACEMENTDIALOG_HPP
#define OPALPLACEMENTDIALOG_HPP

#include <QDialog>

#include "../../model/tools/opallist.hpp"

class QTableWidget;

// Starfield OPAL procedural-placement list editor (REMAINING.md §3.8). Shows
// the header/rows of a .opl list in a table, lets the user add/remove rows and
// edit cells, and loads/saves the file via OpalList.
class OpalPlacementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpalPlacementDialog(QWidget* parent = nullptr);

    void setList(const OpalList& list);
    OpalList list() const;

private slots:
    void addRow();
    void removeRow();
    void loadFromFile();
    void saveToFile();

private:
    void rebuildTable();

    OpalList m_list;
    QTableWidget* m_table = nullptr;
};

#endif // OPALPLACEMENTDIALOG_HPP
