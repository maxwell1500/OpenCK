MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::mainwindow),
    mData(nullptr),
    mUndoStack(nullptr),
    mUndoAction(nullptr),
    mRedoAction(nullptr)
{
    ui->setupUi(this);
}
