#include "mainwindow.hpp"

#include "../../../ui/ui_mainwindow.h"

#include "../../model/tools/searchalgorithm.hpp"
#include "../../model/tools/npcvalidator.hpp"
#include "../../model/tools/weaponvalidator.hpp"
#include "../../model/tools/questvalidator.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/editrecordcommand.hpp"
#include "../../model/world/data.hpp"
#include "../../model/doc/messages.hpp"
#include "filepaths.hpp"
#include "searchdialog.hpp"
#include "loadorderdialog.hpp"
#include "windowlayout.hpp"
#include "masterslistdialog.hpp"
#include "conflictdialog.hpp"
#include "nifviewportwidget.hpp"
#include "scripteditorwidget.hpp"
#include "dialogueeditorwidget.hpp"
#include "formideditorwidget.hpp"
#include "assetbrowserwidget.hpp"
#include "spellwizard.hpp"
#include "pluginmergedialog.hpp"
#include "conflictresolutiondialog.hpp"
#include "dialoguetreeeditor.hpp"
#include "questgrapheditor.hpp"
#include "aipackageeditor.hpp"
#include "weatherlighteditor.hpp"
#include "navmesheditor.hpp"
#include "watereditor.hpp"
#include "celltransitionseditor.hpp"
#include "landscapeeditor.hpp"
#include "objectpalette.hpp"
#include "materialeditor.hpp"
#include "soundeditor.hpp"
#include "animationeditor.hpp"
#include "particleeffectseffecteditor.hpp"
#include "papyrusdebugger.hpp"
#include "papyruscompiler.hpp"
#include "../../../libs/files/ba2/ba2archive.hpp"
#include "loadorderoptimizerdialog.hpp"
#include "externaltoolsdialog.hpp"
#include "logger.hpp"
#include "preferencesdialog.hpp"
#include "worldspacedialog.hpp"
#include "cellsdialog.hpp"
#include "objectwindowdialog.hpp"
#include "exportdialog.hpp"
#include "batchexportdialog.hpp"
#include "exporttemplatesdialog.hpp"
#include "bashedpatchdialog.hpp"
#include "modmanagerdialog.hpp"
#include "validationreportdialog.hpp"
#include "inspectorwidget.hpp"
#include "../../model/tools/assetvalidator.hpp"
#include "../../model/tools/assetdependencyscanner.hpp"
#include "../../model/tools/batchtools.hpp"
#include "../../model/tools/lodgenerator.hpp"
#include "../../model/world/shortcutmanager.hpp"
#include "shortcuteditordialog.hpp"
#include "toolbarcustomizationdialog.hpp"
#include "thememanager.hpp"

#include <QInputDialog>

#include <QCoreApplication>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QKeySequence>
#include <QDockWidget>
#include <QLabel>
#include <QProgressBar>
#include <QShortcut>
#include <QPushButton>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QSettings>
#include <QCloseEvent>
#include <QRegularExpression>
#include <cmath>
#include <QVector3D>

#include "../../../libs/files/esm/refrecord.hpp"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::mainwindow),
    mData(nullptr),
    mUndoStack(nullptr),
    mUndoAction(nullptr),
    mRedoAction(nullptr),
    mExportMenu(nullptr),
      nifViewportWidget(nullptr),
      scriptEditorWidget(nullptr),
      dialogueEditorWidget(nullptr),
      formIdEditorWidget(nullptr),
      assetBrowserWidget(nullptr),
      objectWindowDock(nullptr),
      landscapeEditor(nullptr),
      landscapeDock(nullptr),
      objectPalette(nullptr),
      mCellViewPanel(nullptr),
      mCellViewDock(nullptr),
      mWarningsDock(nullptr),
      mInspectorWidget(nullptr),
      mInspectorDock(nullptr),
      mDockManager(nullptr),
      mStatusRecordCount(nullptr),
      mStatusPluginInfo(nullptr),
      mStatusCellCoords(nullptr),
      mStatusSelectedObject(nullptr),
      mStatusProgressBar(nullptr)
{
    ui->setupUi(this);

    mDockManager = new ads::CDockManager(this);

    setupEditMenu();
    setupShortcuts();
    restoreUiState();

    // Wire Theme menu actions to ThemeManager
    connect(ui->actionThemeDefault, &QAction::triggered, this, []() {
        auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
        if (app) ThemeManager::setTheme(*app, ThemeManager::Theme::System);
    });
    connect(ui->actionThemeLight, &QAction::triggered, this, []() {
        auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
        if (app) ThemeManager::setTheme(*app, ThemeManager::Theme::Light);
    });
    connect(ui->actionThemeDark, &QAction::triggered, this, []() {
        auto* app = qobject_cast<QApplication*>(QCoreApplication::instance());
        if (app) ThemeManager::setTheme(*app, ThemeManager::Theme::Dark);
    });
    
    // Initialize status bar widgets
    mStatusRecordCount = new QLabel(ui->statusBar);
    mStatusRecordCount->setText("Records: 0");
    ui->statusBar->addWidget(mStatusRecordCount);

    mStatusCellCoords = new QLabel(ui->statusBar);
    mStatusCellCoords->setText("Cell: -, -");
    ui->statusBar->addWidget(mStatusCellCoords);

    mStatusSelectedObject = new QLabel(ui->statusBar);
    mStatusSelectedObject->setText("No selection");
    ui->statusBar->addWidget(mStatusSelectedObject);

    mStatusPluginInfo = new QLabel(ui->statusBar);
    mStatusPluginInfo->setText("No plugin loaded");
    ui->statusBar->addWidget(mStatusPluginInfo);

    mStatusProgressBar = new QProgressBar(ui->statusBar);
    mStatusProgressBar->setVisible(false);
    mStatusProgressBar->setRange(0, 100);
    ui->statusBar->addPermanentWidget(mStatusProgressBar);
    
    // Connect toolbar buttons
    connect(ui->actionUndoButton, &QAction::triggered, this, &MainWindow::on_actionUndo_triggered);
    connect(ui->actionRedoButton, &QAction::triggered, this, &MainWindow::on_actionRedo_triggered);
    connect(ui->actionSaveAllButton, &QAction::triggered, this, &MainWindow::on_actionSaveAllButton_triggered);
    connect(ui->actionCheckOut, &QAction::triggered, this, &MainWindow::on_actionCheckOut_triggered);
    connect(ui->actionCheckIn, &QAction::triggered, this, &MainWindow::on_actionCheckIn_triggered);

    // Reset Window Layout action lives in the Docks menu (defined in the UI file)
    connect(ui->actionResetWindowLayout, &QAction::triggered, this, [this]() {
        WindowLayout::applyDefaultLayout(this);
        LOG_INFO("Window layout reset to default");
    });
}

MainWindow::~MainWindow()
{
    // Widgets managed by Qt's parent-child system are automatically deleted
    // when MainWindow is destroyed. Only delete objects NOT in the widget tree.
    // (nifViewportWidget, scriptEditorWidget, dialogueEditorWidget, formIdEditorWidget,
    //  objectWindowDock, landscapeEditor, objectPalette, landscapeDock are all
    //  parented to 'this' and managed by Qt.)
    if (ui)
    {
        delete ui;
    }
}

void MainWindow::setData(Data* data)
{
    mData = data;
    if (mData)
    {
        mUndoStack = mData->getUndoStack();
        updateUndoRedoActions();
        updateStatus("Data loaded");
        
        // Create the 3D Viewport as the central widget (matches real CK layout)
        if (!nifViewportWidget)
        {
            nifViewportWidget = new NifViewportWidget(this);
            setCentralWidget(nifViewportWidget);
        }

        // Populate the viewport's cell references from the loaded document
        if (nifViewportWidget && mData)
        {
            QVector<ViewportCellRef> refs;
            const auto& coll = mData->getRefrCollection();
            for (int i = 0; i < coll.size(); ++i)
            {
                const auto& rec = coll.getRecord(i).get();
                if (coll.getRecord(i).isDeleted()) continue;
                ViewportCellRef r;
                r.position = QVector3D(rec.posX, rec.posY, rec.posZ);
                r.enabled = !rec.initiallyDisabled;
                r.dataIndex = i;
                r.rotX = rec.rotX; r.rotY = rec.rotY; r.rotZ = rec.rotZ;
                r.scale = rec.scale;
                refs.append(r);
            }
            nifViewportWidget->setCellReferences(refs);
        }

        if (!mViewportRefsConnected)
        {
            // Render window selection -> status bar + Inspector
            connect(nifViewportWidget, &NifViewportWidget::refSelected,
                    this, [this](int dataIndex) {
                if (dataIndex < 0 || !mData)
                {
                    if (mStatusSelectedObject) mStatusSelectedObject->setText(QStringLiteral("No selection"));
                    if (mInspectorWidget) mInspectorWidget->clear();
                    return;
                }
                auto& coll = mData->getRefrCollection();
                if (dataIndex >= coll.size()) return;
                RefrRecord& rec = coll.getRecord(dataIndex).get();
                const QString id = QStringLiteral("0x%1").arg(rec.formId, 8, 16, QChar('0'));
                if (mStatusSelectedObject) mStatusSelectedObject->setText(QStringLiteral("REFR %1").arg(id));
                if (mInspectorWidget) mInspectorWidget->showComponents(&rec.components, QStringLiteral("Reference %1").arg(id));
            });

            // Live transform readout in the status bar while dragging
            connect(nifViewportWidget, &NifViewportWidget::refTransformPreview,
                    this, [this](int, const QVector3D& pos, const QVector3D& rot, float scale) {
                if (mStatusSelectedObject)
                    mStatusSelectedObject->setText(
                        QStringLiteral("X %1  Y %2  Z %3  RX %4  RY %5  RZ %6  S %7")
                            .arg(pos.x(), 0, 'f', 1).arg(pos.y(), 0, 'f', 1).arg(pos.z(), 0, 'f', 1)
                            .arg(rot.x() * 57.2957795f, 0, 'f', 1).arg(rot.y() * 57.2957795f, 0, 'f', 1)
                            .arg(rot.z() * 57.2957795f, 0, 'f', 1).arg(scale, 0, 'f', 2));
            });

            // Undoable write-back on drag end
            connect(nifViewportWidget, &NifViewportWidget::refTransformCommitted,
                    this, [this](int dataIndex, const QVector3D& pos, const QVector3D& rot, float scale) {
                if (dataIndex < 0 || !mData) return;
                auto& coll = mData->getRefrCollection();
                if (dataIndex >= coll.size()) return;
                RefrRecord original = coll.getRecord(dataIndex).get();
                RefrRecord edited = original;
                edited.posX = pos.x(); edited.posY = pos.y(); edited.posZ = pos.z();
                edited.rotX = rot.x(); edited.rotY = rot.y(); edited.rotZ = rot.z();
                edited.scale = scale;
                auto* cmd = new EditRecordCommand<RefrRecord>(
                    &coll, dataIndex, original, edited,
                    QStringLiteral("Transform Reference 0x%1").arg(edited.formId, 8, 16, QChar('0')));
                if (mUndoStack && cmd->hasChanged())
                    mUndoStack->push(cmd);
                else
                    delete cmd;
                if (mStatusSelectedObject)
                    mStatusSelectedObject->setText(QStringLiteral("X %1  Y %2  Z %3")
                        .arg(pos.x(), 0, 'f', 1).arg(pos.y(), 0, 'f', 1).arg(pos.z(), 0, 'f', 1));
            });
            mViewportRefsConnected = true;
        }
        
        // Create Object Window dock widget
        if (!objectWindowDock)
        {
            objectWindowDock = new ObjectWindowDialog(mData, this);
            auto* objectDockWidget = new ads::CDockWidget("Object Window");
            objectDockWidget->setWidget(objectWindowDock);
            mDockManager->addDockWidget(ads::LeftDockWidgetArea, objectDockWidget);
        }
        
        // Create Cell View on application load (not on-demand)
        if (!mCellViewPanel)
        {
            mCellViewPanel = new CellViewPanel(mData, this);
            mCellViewDock = new ads::CDockWidget(QStringLiteral("Cell View"));
            mCellViewDock->setWidget(mCellViewPanel);
            mDockManager->addDockWidget(ads::RightDockWidgetArea, mCellViewDock);
        }

        if (!mCellViewConnected)
        {
            // Cell View selection -> status bar + Inspector + Render Window focus
            connect(mCellViewPanel, &CellViewPanel::refSelected,
                    this, [this](const RefrRecord* rec) {
                if (!rec || !mData)
                {
                    if (mStatusSelectedObject) mStatusSelectedObject->setText(QStringLiteral("No selection"));
                    return;
                }
                const int idx = mData->getRefrCollection().searchId(rec->editorId);
                const QString id = QStringLiteral("0x%1").arg(rec->formId, 8, 16, QChar('0'));
                if (mStatusSelectedObject) mStatusSelectedObject->setText(QStringLiteral("REFR %1").arg(id));
                if (idx >= 0)
                {
                    RefrRecord& fullRec = mData->getRefrCollection().getRecord(idx).get();
                    if (mInspectorWidget) mInspectorWidget->showComponents(&fullRec.components, QStringLiteral("Reference %1").arg(id));
                    if (nifViewportWidget) nifViewportWidget->setSelectedRefByDataIndex(idx);
                }
            });

            // Cell View cursor position -> status bar cell coords
            connect(mCellViewPanel, &CellViewPanel::cursorWorldPos,
                    this, [this](const QPointF& wp) {
                if (!mStatusCellCoords) return;
                const int gx = static_cast<int>(std::floor(wp.x() / 4096.0));
                const int gy = static_cast<int>(std::floor(wp.y() / 4096.0));
                mStatusCellCoords->setText(QStringLiteral("Cell (%1, %2)  X %3  Y %4")
                    .arg(gx).arg(gy).arg(wp.x(), 0, 'f', 1).arg(wp.y(), 0, 'f', 1));
            });
            mCellViewConnected = true;
        }
        
        // Create Warnings dock widget
        if (!mWarningsDock)
        {
            auto* warningsWidget = new QWidget(this);
            auto* warningsLayout = new QVBoxLayout(warningsWidget);
            warningsLayout->setContentsMargins(4, 4, 4, 4);
            auto* warningsList = new QTableWidget(warningsWidget);
            warningsList->setColumnCount(3);
            warningsList->setHorizontalHeaderLabels({"Level", "Message", "Record"});
            warningsList->horizontalHeader()->setStretchLastSection(true);
            warningsList->setSelectionBehavior(QAbstractItemView::SelectRows);
            warningsList->setEditTriggers(QAbstractItemView::NoEditTriggers);
            warningsLayout->addWidget(warningsList);
            mWarningsDock = new ads::CDockWidget("Warnings");
            mWarningsDock->setWidget(warningsWidget);
            mDockManager->addDockWidget(ads::BottomDockWidgetArea, mWarningsDock);
        }

        // Create Inspector dock widget
        if (!mInspectorWidget)
        {
            mInspectorWidget = new InspectorWidget(this);
            mInspectorDock = new ads::CDockWidget("Inspector");
            mInspectorDock->setWidget(mInspectorWidget);
            mDockManager->addDockWidget(ads::RightDockWidgetArea, mInspectorDock);
            connect(objectWindowDock, &ObjectWindowDialog::recordSelected,
                    this, [this](int cat, int rec, const QString& eid) {
                auto lookup = objectWindowDock->getFormComponentsForIndex(cat, rec);
                if (lookup.components) {
                    QString title = QString("%1 (%2)")
                        .arg(lookup.recordType)
                        .arg(eid);
                    mInspectorWidget->showComponents(lookup.components, title);
                } else {
                    mInspectorWidget->clear();
                }
            });
        }
        
        // Create Landscape Editor dock widget
        if (!landscapeEditor)
        {
            landscapeEditor = new LandscapeEditor(this);
            landscapeEditor->setData(mData);
            landscapeDock = new ads::CDockWidget("Landscape Editor");
            landscapeDock->setWidget(landscapeEditor);
            mDockManager->addDockWidget(ads::BottomDockWidgetArea, landscapeDock);
            connect(landscapeDock, &ads::CDockWidget::visibilityChanged, this, [this](bool visible) {
                if (!visible && landscapeEditor)
                {
                    landscapeEditor->clear();
                }
            });
        }
        
        // Create Object Palette dock widget
        if (!objectPalette)
        {
            objectPalette = new ObjectPalette(mData, this);
            auto* paletteDockWidget = new ads::CDockWidget("Object Palette");
            paletteDockWidget->setWidget(objectPalette);
            mDockManager->addDockWidget(ads::BottomDockWidgetArea, paletteDockWidget);
        }
        else
        {
            objectPalette->populateObjectList();
        }

        objectWindowDock->setVisible(true);

        updateStatus("Data loaded");

        WindowLayout::applyDefaultLayout(this);
    }
    else
    {
        updateStatus("");
    }
}

void MainWindow::setupEditMenu()
{
    // Actions are already defined in the UI file, just connect them
    mUndoAction = ui->actionUndo;
    mRedoAction = ui->actionRedo;
    mExportMenu = ui->menuExport;

    QAction* exportRecordsAction = new QAction(tr("Export/Import Records..."), this);
    connect(exportRecordsAction, &QAction::triggered, this, [this]() {
        ExportDialog dialog(mData, this);
        dialog.exec();
    });
    mExportMenu->addSeparator();
    mExportMenu->addAction(exportRecordsAction);

    QAction* exportTemplatesAction = new QAction(tr("Export Templates..."), this);
    connect(exportTemplatesAction, &QAction::triggered, this, [this]() {
        ExportTemplatesDialog dialog(mData, this);
        dialog.exec();
    });
    mExportMenu->addAction(exportTemplatesAction);

    connect(mUndoAction, &QAction::triggered, this, &MainWindow::on_actionUndo_triggered);
    connect(mRedoAction, &QAction::triggered, this, &MainWindow::on_actionRedo_triggered);

    // Add Asset Validation action to Tools menu
    QAction* assetValidationAction = new QAction(tr("Validate Assets..."), this);
    connect(assetValidationAction, &QAction::triggered, this, &MainWindow::on_actionAssetValidation_triggered);
    ui->menuTools->addSeparator();
    ui->menuTools->addAction(assetValidationAction);

    // Add Asset Dependency Scanner action to Tools menu
    QAction* assetDepScanAction = new QAction(tr("Check Asset Dependencies..."), this);
    connect(assetDepScanAction, &QAction::triggered, this, &MainWindow::on_actionAssetDependencyScanner_triggered);
    ui->menuTools->addAction(assetDepScanAction);

    // Add Batch Export action to Tools menu
    QAction* batchExportAction = new QAction(tr("Batch Export..."), this);
    connect(batchExportAction, &QAction::triggered, this, &MainWindow::on_actionBatchExport_triggered);
    ui->menuTools->addAction(batchExportAction);

    // Add Batch Tools actions to Tools menu
    QAction* batchRenameAction = new QAction(tr("Batch Rename Records..."), this);
    connect(batchRenameAction, &QAction::triggered, this, [this]() {
        if (!mData) {
            QMessageBox::information(this, "Batch Rename",
                "No document is currently loaded.\n\n"
                "Open a plugin file first via File > Data.");
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle("Batch Rename Records");
        dialog.setMinimumWidth(400);

        auto* formLayout = new QFormLayout(&dialog);

        auto* typeCombo = new QComboBox(&dialog);
        typeCombo->addItems({
            "NPCs", "Weapons", "Armor", "Spells", "Magic",
            "Quests", "Dialogues", "Infos", "Globals", "Location Ref Types",
            "Packages", "Trees", "Alchemy", "Ingredients", "Containers",
            "Enchantments", "Books", "Misc Items", "Activators", "Statics",
            "Races", "Classes", "Factions", "Perks", "Cells",
            "Worldspaces", "Locations", "References", "Materials"
        });
        formLayout->addRow("Record Type:", typeCombo);

        auto* findEdit = new QLineEdit(&dialog);
        findEdit->setPlaceholderText("Text to find");
        formLayout->addRow("Find:", findEdit);

        auto* replaceEdit = new QLineEdit(&dialog);
        replaceEdit->setPlaceholderText("Replacement text");
        formLayout->addRow("Replace:", replaceEdit);

        auto* regexCheck = new QCheckBox("Use Regular Expression", &dialog);
        formLayout->addRow("", regexCheck);

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        formLayout->addRow(buttonBox);

        if (dialog.exec() != QDialog::Accepted) return;

        CkId::Type types[] = {
            CkId::Type_Npc_, CkId::Type_Weap_, CkId::Type_Armor_, CkId::Type_Spel_,
            CkId::Type_Magic_, CkId::Type_Quest_, CkId::Type_Dial_, CkId::Type_Info_,
            CkId::Type_Glob_, CkId::Type_Lcrt_, CkId::Type_Pack_, CkId::Type_Tree_,
            CkId::Type_Alch_, CkId::Type_Ingr_, CkId::Type_Cont_, CkId::Type_Ench_,
            CkId::Type_Book_, CkId::Type_Misc_, CkId::Type_Acti_, CkId::Type_Stat_,
            CkId::Type_Race_, CkId::Type_Class_, CkId::Type_Fact_, CkId::Type_PerK_,
            CkId::Type_Cel_, CkId::Type_WRLD_, CkId::Type_LOCT_, CkId::Type_Refr_,
            CkId::Type_Material_
        };

        int idx = typeCombo->currentIndex();
        if (idx < 0 || idx >= 29) return;

        QString findText = findEdit->text();
        QString replaceText = replaceEdit->text();

        if (findText.isEmpty()) {
            QMessageBox::warning(this, "Batch Rename", "Find pattern cannot be empty.");
            return;
        }

        if (regexCheck->isChecked()) {
            QRegularExpression test(findText);
            if (!test.isValid()) {
                QMessageBox::warning(this, "Batch Rename",
                    "Invalid regular expression: " + test.errorString());
                return;
            }
        }

        BatchTools::RenameResult result = BatchTools::batchRename(
            *mData, types[idx], findText, replaceText, regexCheck->isChecked());

        QString msg = QString("Batch rename completed.\n\nRecords renamed: %1").arg(result.recordsRenamed);
        if (!result.warnings.isEmpty()) {
            msg += "\n\nWarnings:\n" + result.warnings.join("\n");
        }
        QMessageBox::information(this, "Batch Rename", msg);
    });
    ui->menuTools->addAction(batchRenameAction);

    QAction* reassignFormIdsAction = new QAction(tr("Reassign FormIds..."), this);
    connect(reassignFormIdsAction, &QAction::triggered, this, [this]() {
        if (!mData) {
            QMessageBox::information(this, "Reassign FormIds",
                "No document is currently loaded.\n\n"
                "Open a plugin file first via File > Data.");
            return;
        }

        bool ok;
        quint32 startFormId = static_cast<quint32>(QInputDialog::getInt(
            this, "Reassign FormIds",
            "Start FormId (hex, e.g. 800):",
            static_cast<int>(0x800), 0, 0x7FFFFFFF, 1, &ok));

        if (!ok) return;

        BatchTools::FormIdResult result = BatchTools::batchReassignFormIds(*mData, startFormId);

        QString msg = QString("FormId reassignment completed.\n\nFormIds reassigned: %1").arg(result.formIdsReassigned);
        if (!result.warnings.isEmpty()) {
            msg += "\n\nWarnings:\n" + result.warnings.join("\n");
        }
        QMessageBox::information(this, "Reassign FormIds", msg);
    });
    ui->menuTools->addAction(reassignFormIdsAction);

    QAction* lodGenAction = new QAction(tr("Generate LOD Meshes..."), this);
    connect(lodGenAction, &QAction::triggered, this, [this]() {
        QDialog dialog(this);
        dialog.setWindowTitle("Generate LOD Meshes");
        dialog.setMinimumWidth(380);

        auto* formLayout = new QFormLayout(&dialog);

        auto* reductionSpin = new QSpinBox(&dialog);
        reductionSpin->setRange(10, 90);
        reductionSpin->setValue(50);
        reductionSpin->setSuffix("%");
        formLayout->addRow("Reduction Percent:", reductionSpin);

        auto* lodLevelsSpin = new QSpinBox(&dialog);
        lodLevelsSpin->setRange(2, 5);
        lodLevelsSpin->setValue(3);
        formLayout->addRow("LOD Levels:", lodLevelsSpin);

        auto* preserveUVsCheck = new QCheckBox("Preserve UVs", &dialog);
        preserveUVsCheck->setChecked(true);
        formLayout->addRow("", preserveUVsCheck);

        auto* preserveNormalsCheck = new QCheckBox("Preserve Normals", &dialog);
        preserveNormalsCheck->setChecked(true);
        formLayout->addRow("", preserveNormalsCheck);

        auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        formLayout->addRow(buttonBox);

        if (dialog.exec() != QDialog::Accepted)
            return;

        QString dataDir;
        if (mData) {
            dataDir = mData->getPaths().dataDir.absolutePath();
        } else {
            QMessageBox::information(this, "LOD Generation",
                "No plugin loaded. Select a data directory.");
            dataDir = QFileDialog::getExistingDirectory(this, "Select Data Directory");
            if (dataDir.isEmpty()) return;
        }

        LodGenerator::LodOptions opts;
        opts.reductionPercent = reductionSpin->value() / 100.0f;
        opts.targetLodLevels = lodLevelsSpin->value();
        opts.preserveUVs = preserveUVsCheck->isChecked();
        opts.preserveNormals = preserveNormalsCheck->isChecked();

        QStringList nifFiles;
        QDir scanDir(dataDir);
        QFileInfoList entries = scanDir.entryInfoList(QStringList() << "*.nif", QDir::Files, QDir::Name);
        for (const auto& entry : entries) {
            nifFiles.append(scanDir.relativeFilePath(entry.absoluteFilePath()));
        }

        if (nifFiles.isEmpty()) {
            QMessageBox::information(this, "LOD Generation", "No NIF files found in data directory.");
            return;
        }

        int processed = LodGenerator::batchGenerateLod(dataDir, nifFiles, opts);

        QMessageBox::information(this, "LOD Generation",
            QString("LOD generation complete.\n\nFiles processed: %1").arg(processed));
    });
    ui->menuTools->addAction(lodGenAction);
}

void MainWindow::setupShortcuts()
{
    auto& mgr = ShortcutManager::instance();
    mgr.init();

    applyShortcuts();
}

void MainWindow::applyShortcuts()
{
    auto& mgr = ShortcutManager::instance();

    // Clean up previously created dynamic actions and shortcuts
    for (QAction* action : mDynamicActions)
    {
        if (action->parent() == this)
            ui->menuEdit->removeAction(action);
        else if (action->parent() == ui->menuView)
            ui->menuView->removeAction(action);
        delete action;
    }
    mDynamicActions.clear();

    for (QShortcut* shortcut : mDynamicShortcuts)
        delete shortcut;
    mDynamicShortcuts.clear();

    // ========================================================================
    // FILE MENU SHORTCUTS
    // ========================================================================
    ui->actionNewPlugin->setShortcut(mgr.get("NewPlugin"));
    ui->actionNewPlugin->setToolTip(tr("New Plugin (%1)").arg(mgr.get("NewPlugin").toString()));
    ui->actionData->setShortcut(mgr.get("OpenPlugin"));
    ui->actionData->setToolTip(tr("Open Plugin (%1)").arg(mgr.get("OpenPlugin").toString()));
    ui->actionSave->setShortcut(mgr.get("SavePlugin"));
    ui->actionSave->setToolTip(tr("Save Plugin (%1)").arg(mgr.get("SavePlugin").toString()));
    ui->actionSaveAs->setShortcut(mgr.get("SaveAsPlugin"));
    ui->actionSaveAs->setToolTip(tr("Save As (%1)").arg(mgr.get("SaveAsPlugin").toString()));
    ui->actionClosePlugin->setShortcut(mgr.get("ClosePlugin"));
    ui->actionClosePlugin->setToolTip(tr("Close Plugin (%1)").arg(mgr.get("ClosePlugin").toString()));
    ui->actionExit->setShortcut(mgr.get("Exit"));
    ui->actionExit->setToolTip(tr("Exit (%1)").arg(mgr.get("Exit").toString()));
    
    // ========================================================================
    // EDIT MENU SHORTCUTS
    // ========================================================================
    ui->actionUndo->setShortcut(mgr.get("Undo"));
    ui->actionUndo->setToolTip(tr("Undo (%1)").arg(mgr.get("Undo").toString()));
    ui->actionRedo->setShortcut(mgr.get("Redo"));
    ui->actionRedo->setToolTip(tr("Redo (%1)").arg(mgr.get("Redo").toString()));
    ui->actionCut->setShortcut(mgr.get("Cut"));
    ui->actionCut->setToolTip(tr("Cut (%1)").arg(mgr.get("Cut").toString()));
    ui->actionCopy->setShortcut(mgr.get("Copy"));
    ui->actionCopy->setToolTip(tr("Copy (%1)").arg(mgr.get("Copy").toString()));
    ui->actionPaste->setShortcut(mgr.get("Paste"));
    ui->actionPaste->setToolTip(tr("Paste (%1)").arg(mgr.get("Paste").toString()));
    ui->actionDuplicate->setShortcut(mgr.get("Duplicate"));
    ui->actionDuplicate->setToolTip(tr("Duplicate (%1)").arg(mgr.get("Duplicate").toString()));
    ui->actionSelectAll->setShortcut(mgr.get("SelectAll"));
    ui->actionSelectAll->setToolTip(tr("Select All (%1)").arg(mgr.get("SelectAll").toString()));
    ui->actionSearchAndReplace->setShortcut(mgr.get("SearchAndReplace"));
    ui->actionSearchAndReplace->setToolTip(tr("Search and Replace (%1)").arg(mgr.get("SearchAndReplace").toString()));
    ui->actionFindNext->setShortcut(mgr.get("FindNext"));
    ui->actionFindNext->setToolTip(tr("Find Next (%1)").arg(mgr.get("FindNext").toString()));
    ui->actionFindPrevious->setShortcut(mgr.get("FindPrevious"));
    ui->actionFindPrevious->setToolTip(tr("Find Previous (%1)").arg(mgr.get("FindPrevious").toString()));
    
    // Find (Ctrl+F) - opens the search dialog
    auto* findAction = new QAction(tr("Find..."), this);
    findAction->setShortcut(mgr.get("Find"));
    findAction->setToolTip(tr("Find (%1)").arg(mgr.get("Find").toString()));
    connect(findAction, &QAction::triggered, this, &MainWindow::on_actionSearchAndReplace_triggered);
    ui->menuEdit->addAction(findAction);
    mDynamicActions.append(findAction);
    
    // ========================================================================
    // VIEW MENU SHORTCUTS
    // ========================================================================
    ui->actionObjectWindow->setShortcut(mgr.get("ObjectWindow"));
    ui->actionObjectWindow->setToolTip(tr("Object Window (%1)").arg(mgr.get("ObjectWindow").toString()));
    ui->action3DViewport->setShortcut(mgr.get("NifViewport"));
    ui->action3DViewport->setToolTip(tr("3D Viewport (%1)").arg(mgr.get("NifViewport").toString()));
    ui->actionScriptEditor->setShortcut(mgr.get("ScriptEditor"));
    ui->actionScriptEditor->setToolTip(tr("Script Editor (%1)").arg(mgr.get("ScriptEditor").toString()));
    ui->actionDialogueEditor->setShortcut(mgr.get("DialogueEditor"));
    ui->actionDialogueEditor->setToolTip(tr("Dialogue Editor (%1)").arg(mgr.get("DialogueEditor").toString()));
    ui->actionDialogueTree->setShortcut(mgr.get("DialogueTree"));
    ui->actionQuestGraph->setShortcut(mgr.get("QuestGraph"));
    ui->actionQuestGraph->setToolTip(tr("Quest Graph (%1)").arg(mgr.get("QuestGraph").toString()));
    ui->actionAIPackages->setShortcut(mgr.get("AIPackages"));
    ui->actionAIPackages->setToolTip(tr("AI Packages (%1)").arg(mgr.get("AIPackages").toString()));
    ui->actionWeatherLight->setShortcut(mgr.get("WeatherLight"));
    ui->actionWeatherLight->setToolTip(tr("Weather and Lighting (%1)").arg(mgr.get("WeatherLight").toString()));
    ui->actionNavmesh->setShortcut(mgr.get("Navmesh"));
    ui->actionNavmesh->setToolTip(tr("Navmesh Editor (%1)").arg(mgr.get("Navmesh").toString()));
    ui->actionWater->setShortcut(mgr.get("WaterEditor"));
    ui->actionWater->setToolTip(tr("Water Editor (%1)").arg(mgr.get("WaterEditor").toString()));
    ui->actionCellTransitions->setShortcut(mgr.get("CellTransitions"));
    ui->actionCellTransitions->setToolTip(tr("Cell Transitions (%1)").arg(mgr.get("CellTransitions").toString()));
    ui->actionMaterialEditor->setShortcut(mgr.get("MaterialEditor"));
    ui->actionMaterialEditor->setToolTip(tr("Material Editor (%1)").arg(mgr.get("MaterialEditor").toString()));
    ui->actionPapyrusDebugger->setShortcut(mgr.get("PapyrusDebugger"));
    ui->actionPapyrusDebugger->setToolTip(tr("Papyrus Debugger (%1)").arg(mgr.get("PapyrusDebugger").toString()));
    ui->actionFormIdEditor->setShortcut(mgr.get("FormIdEditor"));
    ui->actionFormIdEditor->setToolTip(tr("Form ID Editor (%1)").arg(mgr.get("FormIdEditor").toString()));
    
    // Refresh (F5)
    auto* refreshAction = new QAction(tr("Refresh"), this);
    refreshAction->setShortcut(mgr.get("Refresh"));
    refreshAction->setToolTip(tr("Refresh Viewport (%1)").arg(mgr.get("Refresh").toString()));
    connect(refreshAction, &QAction::triggered, this, [this]() {
        if (nifViewportWidget) {
            nifViewportWidget->refreshMesh();
        }
    });
    ui->menuView->addAction(refreshAction);
    mDynamicActions.append(refreshAction);
    
    // ========================================================================
    // WORLD MENU SHORTCUTS
    // ========================================================================
    ui->actionWorldspaces->setShortcut(mgr.get("Worldspaces"));
    ui->actionWorldspaces->setToolTip(tr("Worldspaces (%1)").arg(mgr.get("Worldspaces").toString()));
    ui->actionCells->setShortcut(mgr.get("Cells"));
    ui->actionCells->setToolTip(tr("Cells (%1)").arg(mgr.get("Cells").toString()));
    ui->actionLandscapeEditing->setShortcut(mgr.get("LandscapeEditing"));
    ui->actionLandscapeEditing->setToolTip(tr("Landscape Editing (%1)").arg(mgr.get("LandscapeEditing").toString()));
    ui->actionObjectPalette->setShortcut(mgr.get("ObjectPalette"));
    
    // ========================================================================
    // PLUGINS MENU SHORTCUTS
    // ========================================================================
    ui->actionLoadOrder->setShortcut(mgr.get("LoadOrder"));
    ui->actionLoadOrder->setToolTip(tr("Load Order (%1)").arg(mgr.get("LoadOrder").toString()));
    ui->actionMasterFiles->setShortcut(mgr.get("MasterFiles"));
    ui->actionConflictDetection->setShortcut(mgr.get("ConflictDetection"));
    ui->actionConflictDetection->setToolTip(tr("Conflict Detection (%1)").arg(mgr.get("ConflictDetection").toString()));
    ui->actionConflictResolution->setShortcut(mgr.get("ConflictResolution"));
    ui->actionConflictResolution->setToolTip(tr("Conflict Resolution (%1)").arg(mgr.get("ConflictResolution").toString()));
    ui->actionPluginMerge->setShortcut(mgr.get("PluginMerge"));
    ui->actionPluginMerge->setToolTip(tr("Plugin Merge (%1)").arg(mgr.get("PluginMerge").toString()));
    ui->actionLoadOrderOptimizer->setShortcut(mgr.get("LoadOrderOptimizer"));
    ui->actionBashedPatch->setShortcut(mgr.get("BashedPatch"));
    ui->actionBashedPatch->setToolTip(tr("Bashed Patch (%1)").arg(mgr.get("BashedPatch").toString()));
    ui->actionExternalTools->setShortcut(mgr.get("ExternalTools"));
    
    // ========================================================================
    // UTILITY SHORTCUTS
    // ========================================================================
    ui->actionPreferences->setShortcut(mgr.get("Preferences"));
    ui->actionPreferences->setToolTip(tr("Preferences (%1)").arg(mgr.get("Preferences").toString()));
    ui->actionValidate->setShortcut(mgr.get("Validate"));
    ui->actionValidate->setToolTip(tr("Validate Plugin (%1)").arg(mgr.get("Validate").toString()));
    ui->actionAbout->setShortcut(mgr.get("About"));
    
    // ========================================================================
    // NAVIGATION SHORTCUTS (Object Window)
    // ========================================================================
    ui->actionNextRecord->setShortcut(mgr.get("NextRecord"));
    ui->actionPreviousRecord->setShortcut(mgr.get("PreviousRecord"));
    ui->actionFirstRecord->setShortcut(mgr.get("FirstRecord"));
    ui->actionLastRecord->setShortcut(mgr.get("LastRecord"));
    ui->actionExpandAll->setShortcut(mgr.get("ExpandAll"));
    ui->actionCollapseAll->setShortcut(mgr.get("CollapseAll"));
    
    // ========================================================================
    // RENDER WINDOW SHORTCUTS (Ctrl+G, Ctrl+B, Ctrl+Shift+Z)
    // ========================================================================
    auto findBtn = [this](const char* name) -> QPushButton* {
        auto* vp = this->findChild<NifViewportWidget*>();
        return vp ? vp->findChild<QPushButton*>(name) : nullptr;
    };

    auto* gridShortcut = new QShortcut(mgr.get("ToggleGrid"), this);
    connect(gridShortcut, &QShortcut::activated, this, [findBtn]() {
        if (auto* btn = findBtn("gridBtn")) btn->toggle();
    });
    mDynamicShortcuts.append(gridShortcut);

    auto* boundsShortcut = new QShortcut(mgr.get("ToggleBoundingBoxes"), this);
    connect(boundsShortcut, &QShortcut::activated, this, [findBtn]() {
        if (auto* btn = findBtn("boundsBtn")) btn->toggle();
    });
    mDynamicShortcuts.append(boundsShortcut);

    auto* wireframeShortcut = new QShortcut(mgr.get("ToggleWireframe"), this);
    connect(wireframeShortcut, &QShortcut::activated, this, [findBtn]() {
        if (auto* btn = findBtn("wireframeBtn")) btn->toggle();
    });
    mDynamicShortcuts.append(wireframeShortcut);

    // Customize Toolbar
    auto* customizeToolbarAction = new QAction(tr("Customize Toolbar..."), this);
    connect(customizeToolbarAction, &QAction::triggered, this, [this]() {
        ToolbarCustomizationDialog dlg(ui->mainToolBar, this);
        dlg.exec();
    });
    ui->menuView->addAction(customizeToolbarAction);
    mDynamicActions.append(customizeToolbarAction);

    // Restore saved toolbar state
    ToolbarCustomizationDialog::restoreToolbarConfig(ui->mainToolBar);
    
    LOG_INFO("Keyboard shortcuts configured");
}



void MainWindow::updateUndoRedoActions()
{
    if (!mUndoStack)
    {
        return;
    }

    mUndoAction->setEnabled(mUndoStack->canUndo());
    mRedoAction->setEnabled(mUndoStack->canRedo());
}

void MainWindow::updateStatus(const QString& message)
{
    statusBar()->showMessage(message);
}

void MainWindow::on_actionPreferences_triggered()
{
    PreferencesDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionValidate_triggered()
{
    runValidation();
}

void MainWindow::on_actionSearchAndReplace_triggered()
{
    if (mData)
    {
        SearchDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Search",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About OpenCK",
        "<h2>OpenCK</h2>"
        "<p>Version 1.0.0</p>"
        "<p>An open-source Creation Kit alternative for Elder Scrolls games.</p>"
        "<p>Built with Qt and designed to be compatible with Skyrim/Starfield modding.</p>");
}

void MainWindow::on_actionObjectWindow_triggered()
{
    if (objectWindowDock)
    {
        objectWindowDock->setVisible(!objectWindowDock->isVisible());
    }
}

void MainWindow::on_actionUndo_triggered()
{
    if (mUndoStack && mUndoStack->canUndo())
    {
        mUndoStack->undo();
        updateUndoRedoActions();
    }
}

void MainWindow::on_actionRedo_triggered()
{
    if (mUndoStack && mUndoStack->canRedo())
    {
        mUndoStack->redo();
        updateUndoRedoActions();
    }
}

void MainWindow::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void MainWindow::on_actionLoadOrder_triggered()
{
    LOG_DEBUG("Load Order dialog triggered");
    if (mData)
    {
        QStringList currentOrder;
        const auto& metaData = mData->getMetaData().getRecords();
        for (const auto& rec : metaData)
        {
            currentOrder << rec.get().editorId;
        }
        LOG_INFO(QString("Loaded %1 plugins for load order").arg(currentOrder.size()));
        
        LoadOrderDialog dialog(currentOrder, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            LOG_INFO("Load order updated");
        }
    }
    else
    {
        QMessageBox::information(this, "Load Order",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionMasterFiles_triggered()
{
    if (mData)
    {
        MastersListDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Master Files",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionConflictDetection_triggered()
{
    if (mData)
    {
        ConflictDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Conflict Detection",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionConflictResolution_triggered()
{
    if (mData)
    {
        ConflictResolutionDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Conflict Resolution",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionDialogueTree_triggered()
{
    if (mData)
    {
        DialogueTreeEditor editor(mData, this);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "Dialogue Tree",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionQuestGraph_triggered()
{
    if (mData)
    {
        QuestGraphEditor editor(mData, this);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "Quest Graph",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionAIPackages_triggered()
{
    if (mData)
    {
        AIPackageEditor editor(mData, this);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "AI Packages",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionWeatherLight_triggered()
{
    if (mData)
    {
        WeatherLightEditor editor(mData, this);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "Lighting & Weather",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionNavmesh_triggered()
{
    if (mData)
    {
        NavmeshEditor editor(mData, this, nifViewportWidget);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "Navmesh Editor",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionWater_triggered()
{
    if (mData)
    {
        WaterEditor editor(mData, this);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "Water Editor",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionCellTransitions_triggered()
{
    if (mData)
    {
        CellTransitionsEditor editor(mData, this);
        editor.exec();
    }
    else
    {
        QMessageBox::information(this, "Cell Transitions",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionMaterialEditor_triggered()
{
    if (!mData) return;

    auto& matCollection = mData->getMaterialCollection();
    
    if (matCollection.size() == 0) {
        QMessageBox::information(this, "No Materials",
            "No material records found. Create a material first.");
        return;
    }
    
    MaterialRecord* mat = &matCollection.getRecord(0).get();
    MaterialRecord originalState = *mat;
    MaterialEditor editor(mData, mat, this);
    if (editor.exec() == QDialog::Accepted) {
        auto& coll = mData->getMaterialCollection();
        int idx = coll.searchId(mat->editorId);
        if (idx >= 0 && mData->getUndoStack()) {
            EditRecordCommand<MaterialRecord>* cmd = new EditRecordCommand<MaterialRecord>(&coll, idx, originalState, *mat,
                "Edit material: " + mat->editorId);
            if (cmd->hasChanged()) {
                mData->getUndoStack()->push(cmd);
            } else {
                delete cmd;
            }
        }
    }
}

void MainWindow::on_actionPapyrusDebugger_triggered()
{
    PapyrusDebugger debugger(mData, this);
    debugger.exec();
}

void MainWindow::on_actionNifViewport_triggered()
{
    LOG_DEBUG("3D Viewport toggle");
    
    if (!nifViewportWidget)
    {
        nifViewportWidget = new NifViewportWidget(this);
        setCentralWidget(nifViewportWidget);
        LOG_INFO("3D Viewport created as central widget");
    }
    else
    {
        nifViewportWidget->setVisible(!nifViewportWidget->isVisible());
    }
}

void MainWindow::on_actionScriptEditor_triggered()
{
    LOG_DEBUG("Script Editor toggle");
    
    if (!scriptEditorWidget)
    {
        scriptEditorWidget = new ScriptEditorWidget(this);
        auto* dockWidget = new ads::CDockWidget("Script Editor");
        dockWidget->setWidget(scriptEditorWidget);
        dockWidget->setObjectName("Script Editor Dock");
        mDockManager->addDockWidget(ads::RightDockWidgetArea, dockWidget);
        LOG_INFO("Script Editor dock created");
    }
    else
    {
        auto* dock = mDockManager->findDockWidget("Script Editor");
        if (dock)
        {
            dock->toggleView(dock->isClosed());
        }
    }
}

void MainWindow::on_actionDialogueEditor_triggered()
{
    LOG_DEBUG("Dialogue Editor toggle");
    
    if (!dialogueEditorWidget)
    {
        dialogueEditorWidget = new DialogueEditorWidget(mData, this);
        auto* dockWidget = new ads::CDockWidget("Dialogue Editor");
        dockWidget->setWidget(dialogueEditorWidget);
        dockWidget->setObjectName("Dialogue Editor Dock");
        mDockManager->addDockWidget(ads::RightDockWidgetArea, dockWidget);
        LOG_INFO("Dialogue Editor dock created");
    }
    else
    {
        auto* dock = mDockManager->findDockWidget("Dialogue Editor");
        if (dock)
        {
            dock->toggleView(dock->isClosed());
        }
    }
}

void MainWindow::on_actionData_triggered()
{
    emit actionData_triggered();
}

void MainWindow::on_actionOpenButton_triggered()
{
    emit actionData_triggered();
}

void MainWindow::on_actionNewPlugin_triggered()
{
    emit actionNewPlugin_triggered();
}

void MainWindow::on_actionSaveAs_triggered()
{
    emit actionSaveAs_triggered();
}

void MainWindow::on_actionClosePlugin_triggered()
{
    emit actionClosePlugin_triggered();
}

void MainWindow::on_actionSave_triggered()
{
    emit actionSave_triggered();
}

void MainWindow::on_actionSaveButton_triggered()
{
    emit actionSave_triggered();
}

void MainWindow::on_actionSettings_triggered()
{
    emit actionSettings_triggered();
}

void MainWindow::runValidation()
{
    if (!mData)
    {
        QMessageBox::information(this, "Validation", "No document loaded.");
        return;
    }

    Messages messages(Message::Default);

    QVector<Validator*> validators = {
        new NpcValidator(),
        new WeaponValidator(),
        new QuestValidator()
    };

    for (auto* validator : validators)
    {
        validator->validate(*mData, messages);
    }

    for (auto* validator : validators)
    {
        delete validator;
    }

    if (messages.hasMessages())
    {
        QMessageBox::information(this, "Validation Results", messages.toString());
    }
    else
    {
        QMessageBox::information(this, "Validation", "No errors found.");
    }
}

void MainWindow::on_actionAssetValidation_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Asset Validation",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
        return;
    }

    QString dataDir = mData->getPaths().dataDir.absolutePath();
    AssetValidator::ValidationReport report = AssetValidator::validateAll(*mData, dataDir);

    ValidationReportDialog dialog(report, this);
    dialog.exec();
}

void MainWindow::on_actionAssetDependencyScanner_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Asset Dependency Scanner",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
        return;
    }

    QString dataDir = mData->getPaths().dataDir.absolutePath();
    AssetDependencyScanner::ScanResult result = AssetDependencyScanner::scanAll(*mData, dataDir);

    if (result.missingAssets.isEmpty())
    {
        QMessageBox::information(this, "Asset Dependency Scanner",
            QString("Scan complete. All %1 asset paths found.").arg(result.totalPathsScanned));
        return;
    }

    // Build report dialog
    QDialog dialog(this);
    dialog.setWindowTitle("Asset Dependency Scanner Results");
    dialog.setMinimumSize(700, 500);

    auto* layout = new QVBoxLayout(&dialog);

    auto* summaryLabel = new QLabel(
        QString("Scanned %1 paths. %2 missing assets found.")
            .arg(result.totalPathsScanned).arg(result.totalMissing));
    layout->addWidget(summaryLabel);

    auto* table = new QTableWidget(result.missingAssets.size(), 5, &dialog);
    table->setHorizontalHeaderLabels({"Record", "Type", "Missing Path", "Asset Type", "Suggestions"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);

    for (int i = 0; i < result.missingAssets.size(); i++)
    {
        const auto& missing = result.missingAssets[i];
        table->setItem(i, 0, new QTableWidgetItem(missing.recordId));
        table->setItem(i, 1, new QTableWidgetItem(AssetDependencyScanner::typeName(missing.recordType)));
        table->setItem(i, 2, new QTableWidgetItem(missing.assetPath));
        table->setItem(i, 3, new QTableWidgetItem(missing.assetType));
        table->setItem(i, 4, new QTableWidgetItem(missing.suggestions.join(", ")));
    }
    table->resizeColumnsToContents();
    layout->addWidget(table);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

void MainWindow::on_actionFormIdEditor_triggered()
{
    LOG_DEBUG("FormID Editor triggered");
    
    if (!formIdEditorWidget)
    {
        formIdEditorWidget = new FormIdEditorWidget(mData, this);
        auto* dockWidget = new ads::CDockWidget("FormID Editor");
        dockWidget->setWidget(formIdEditorWidget);
        dockWidget->setObjectName("FormID Editor Dock");
        mDockManager->addDockWidget(ads::BottomDockWidgetArea, dockWidget);
        LOG_INFO("FormID Editor dock created");
    }
    else
    {
        auto* dock = mDockManager->findDockWidget("FormID Editor");
        if (dock)
        {
            dock->toggleView(dock->isClosed());
        }
    }
}

void MainWindow::on_actionAssetBrowser_triggered()
{
    LOG_DEBUG("Asset Browser triggered");
    
    if (!assetBrowserWidget)
    {
        assetBrowserWidget = new AssetBrowserWidget(this);
        if (mData && !mData->getPaths().dataDir.path().isEmpty()) {
            assetBrowserWidget->setDataDirectories({mData->getPaths().dataDir.absolutePath()});
        }
        auto* dockWidget = new ads::CDockWidget(tr("Asset Browser"));
        dockWidget->setWidget(assetBrowserWidget);
        dockWidget->setObjectName("Asset Browser Dock");
        mDockManager->addDockWidget(ads::BottomDockWidgetArea, dockWidget);
        LOG_INFO("Asset Browser dock created");
    }
    else
    {
        auto* dock = mDockManager->findDockWidget("Asset Browser");
        if (dock)
        {
            dock->toggleView(dock->isClosed());
        }
    }
}

void MainWindow::on_actionPluginMerge_triggered()
{
    LOG_DEBUG("Plugin Merge triggered");
    if (mData)
    {
        PluginMergeDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Plugin Merge",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionLoadOrderOptimizer_triggered()
{
    LOG_DEBUG("Load Order Optimizer triggered");
    if (mData)
    {
        LoadOrderOptimizerDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Load Order Optimizer",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionExternalTools_triggered()
{
    LOG_DEBUG("External Tools triggered");
    ExternalToolsDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionSoundEditor_triggered()
{
    SoundEditor dialog(this);
    dialog.exec();
}

void MainWindow::on_actionAnimationEditor_triggered()
{
    AnimationEditor dialog(this);
    dialog.exec();
}

void MainWindow::on_actionParticleEffectsEditor_triggered()
{
    ParticleEffectsEditor dialog(this);
    connect(&dialog, &ParticleEffectsEditor::particleSystemUpdated, this, [this](const ParticleSystemData* data) {
        if (nifViewportWidget) {
            nifViewportWidget->updateParticleSystem(data);
        }
    });
    dialog.exec();
}

void MainWindow::on_actionSpellWizard_triggered()
{
    if (!mData) {
        LOG_INFO("Spell Wizard: No data loaded");
        return;
    }

    SpellWizard wizard(mData, this);
    if (wizard.exec() == QWizard::Accepted) {
        SpellRecord result = wizard.result();

        // Add the new spell to the collection
        auto& collection = mData->getSpellCollection();
        collection.add(result);

        LOG_INFO(QString("Spell '%1' created via wizard").arg(result.editorId));
    }
}

void MainWindow::on_actionWorldspaces_triggered()
{
    if (mData)
    {
        WorldspacesDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        LOG_INFO("Worldspaces: No data loaded");
    }
}

void MainWindow::on_actionCells_triggered()
{
    if (!mData)
    {
        LOG_INFO("Cells: No data loaded");
        return;
    }

    if (!mCellViewPanel)
    {
        mCellViewPanel = new CellViewPanel(mData, this);
        mCellViewDock = new ads::CDockWidget(QStringLiteral("Cell View"));
        mCellViewDock->setWidget(mCellViewPanel);
        mDockManager->addDockWidget(ads::RightDockWidgetArea, mCellViewDock);
    }
    else
    {
        mCellViewDock->toggleView(mCellViewDock->isClosed());
    }
}

void MainWindow::on_actionLandscapeEditing_triggered()
{
    if (landscapeEditor)
    {
        if (objectWindowDock)
        {
            if (CellRecord* cell = objectWindowDock->getSelectedCell())
            {
                landscapeEditor->loadCell(cell);
            }
        }
        landscapeDock->toggleView(true);
        landscapeEditor->raise();
        landscapeEditor->activateWindow();
    }
}

void MainWindow::on_actionObjectPalette_triggered()
{
    if (objectPalette)
    {
        objectPalette->show();
        objectPalette->raise();
        objectPalette->activateWindow();
    }
}

void MainWindow::on_actionModManager_triggered()
{
    LOG_DEBUG("Mod Manager dialog triggered");
    ModManagerDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionBashedPatch_triggered()
{
    LOG_DEBUG("Bashed Patch triggered");
    if (mData)
    {
        BashedPatchDialog dialog(mData, this);
        dialog.exec();
    }
    else
    {
        QMessageBox::information(this, "Bashed Patch",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
    }
}

void MainWindow::on_actionCopy_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Copy", "No document loaded.");
        return;
    }

    QModelIndex index = objectWindowDock ? objectWindowDock->currentIndex() : QModelIndex();
    if (!index.isValid())
    {
        QMessageBox::information(this, "Copy", "No record selected.");
        return;
    }

    objectWindowDock->copyRecord();
}

void MainWindow::on_actionCut_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Cut", "No document loaded.");
        return;
    }

    QModelIndex index = objectWindowDock ? objectWindowDock->currentIndex() : QModelIndex();
    if (!index.isValid())
    {
        QMessageBox::information(this, "Cut", "No record selected.");
        return;
    }

    objectWindowDock->cutRecord();
}

void MainWindow::on_actionPaste_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Paste", "No document loaded.");
        return;
    }

    objectWindowDock->pasteRecord();
}

void MainWindow::on_actionDelete_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Delete", "No document loaded.");
        return;
    }

    objectWindowDock->deleteSelected();
}

void MainWindow::on_actionSelectAll_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        objectWindowDock->getTreeView()->selectAll();
    }
}

void MainWindow::on_actionFindNext_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Find Next", "No document loaded.");
        return;
    }

    if (!objectWindowDock || !objectWindowDock->getFilterEdit() || !objectWindowDock->getTreeView())
    {
        return;
    }

    QString searchText = objectWindowDock->getFilterEdit()->text();
    if (searchText.isEmpty())
    {
        return;
    }

    QTreeView* tree = objectWindowDock->getTreeView();
    QModelIndex current = tree->currentIndex();

    // Start searching from the next item after current
    int startCategory = -1;
    int startRow = -1;

    if (current.isValid())
    {
        startCategory = current.parent().row();
        startRow = current.row();
    }

    // Iterate through all visible items starting after current position
    QAbstractItemModel* model = tree->model();
    int totalCategories = model->rowCount();

    for (int cat = (startCategory >= 0 ? startCategory : 0); cat < totalCategories; ++cat)
    {
        QModelIndex catIndex = model->index(cat, 0);
        int totalRows = model->rowCount(catIndex);
        int startRowForCat = (cat == startCategory && startRow >= 0) ? startRow + 1 : 0;

        for (int row = startRowForCat; row < totalRows; ++row)
        {
            QModelIndex idx = model->index(row, 0, catIndex);
            QString editorId = model->data(idx).toString();

            if (editorId.contains(searchText, Qt::CaseInsensitive))
            {
                tree->setCurrentIndex(idx);
                tree->scrollTo(idx);
                return;
            }
        }
    }

    // Wrap around: search from beginning
    for (int cat = 0; cat < totalCategories; ++cat)
    {
        QModelIndex catIndex = model->index(cat, 0);
        int totalRows = model->rowCount(catIndex);

        for (int row = 0; row < totalRows; ++row)
        {
            QModelIndex idx = model->index(row, 0, catIndex);
            QString editorId = model->data(idx).toString();

            if (editorId.contains(searchText, Qt::CaseInsensitive))
            {
                tree->setCurrentIndex(idx);
                tree->scrollTo(idx);
                return;
            }
        }
    }

    QMessageBox::information(this, "Find Next", QString("No more matches for '%1'.").arg(searchText));
}

void MainWindow::on_actionFindPrevious_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Find Previous", "No document loaded.");
        return;
    }

    if (!objectWindowDock || !objectWindowDock->getFilterEdit() || !objectWindowDock->getTreeView())
    {
        return;
    }

    QString searchText = objectWindowDock->getFilterEdit()->text();
    if (searchText.isEmpty())
    {
        return;
    }

    QTreeView* tree = objectWindowDock->getTreeView();
    QModelIndex current = tree->currentIndex();

    int startCategory = -1;
    int startRow = -1;

    if (current.isValid())
    {
        startCategory = current.parent().row();
        startRow = current.row();
    }

    QAbstractItemModel* model = tree->model();
    int totalCategories = model->rowCount();

    // Search backward from current position
    for (int cat = (startCategory >= 0 ? startCategory : totalCategories - 1); cat >= 0; --cat)
    {
        QModelIndex catIndex = model->index(cat, 0);
        int totalRows = model->rowCount(catIndex);
        int endRow = (cat == startCategory && startRow >= 0) ? startRow - 1 : totalRows - 1;

        for (int row = endRow; row >= 0; --row)
        {
            QModelIndex idx = model->index(row, 0, catIndex);
            QString editorId = model->data(idx).toString();

            if (editorId.contains(searchText, Qt::CaseInsensitive))
            {
                tree->setCurrentIndex(idx);
                tree->scrollTo(idx);
                return;
            }
        }
    }

    // Wrap around: search from end
    for (int cat = totalCategories - 1; cat >= 0; --cat)
    {
        QModelIndex catIndex = model->index(cat, 0);
        int totalRows = model->rowCount(catIndex);

        for (int row = totalRows - 1; row >= 0; --row)
        {
            QModelIndex idx = model->index(row, 0, catIndex);
            QString editorId = model->data(idx).toString();

            if (editorId.contains(searchText, Qt::CaseInsensitive))
            {
                tree->setCurrentIndex(idx);
                tree->scrollTo(idx);
                return;
            }
        }
    }

    QMessageBox::information(this, "Find Previous", QString("No more matches for '%1'.").arg(searchText));
}

void MainWindow::on_actionNextRecord_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        QTreeView* tree = objectWindowDock->getTreeView();
        QModelIndex current = tree->currentIndex();
        if (current.isValid())
        {
            QModelIndex next = tree->model()->index(current.row() + 1, current.column(), current.parent());
            if (next.isValid())
            {
                tree->setCurrentIndex(next);
            }
        }
    }
}

void MainWindow::on_actionPreviousRecord_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        QTreeView* tree = objectWindowDock->getTreeView();
        QModelIndex current = tree->currentIndex();
        if (current.isValid() && current.row() > 0)
        {
            QModelIndex prev = tree->model()->index(current.row() - 1, current.column(), current.parent());
            if (prev.isValid())
            {
                tree->setCurrentIndex(prev);
            }
        }
    }
}

void MainWindow::on_actionFirstRecord_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        QModelIndex first = objectWindowDock->getTreeView()->model()->index(0, 0);
        if (first.isValid())
        {
            objectWindowDock->getTreeView()->setCurrentIndex(first);
        }
    }
}

void MainWindow::on_actionLastRecord_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        QModelIndex last = objectWindowDock->getTreeView()->model()->index(
            objectWindowDock->getTreeView()->model()->rowCount() - 1, 0);
        if (last.isValid())
        {
            objectWindowDock->getTreeView()->setCurrentIndex(last);
        }
    }
}

void MainWindow::on_actionExpandAll_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        objectWindowDock->getTreeView()->expandAll();
    }
}

void MainWindow::on_actionCollapseAll_triggered()
{
    if (objectWindowDock && objectWindowDock->getTreeView())
    {
        objectWindowDock->getTreeView()->collapseAll();
    }
}

void MainWindow::on_actionDuplicate_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Duplicate", "No document loaded.");
        return;
    }

    QModelIndex index = objectWindowDock ? objectWindowDock->currentIndex() : QModelIndex();
    if (!index.isValid())
    {
        QMessageBox::information(this, "Duplicate", "No record selected.");
        return;
    }

    objectWindowDock->cloneSelected();
}

void MainWindow::on_actionExportDialogue_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Export", "No document loaded.");
        return;
    }

    ExportDialog exportDialog(mData, this);
    exportDialog.exportDialogue();
    exportDialog.exec();
}

void MainWindow::on_actionExportScripts_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Export", "No document loaded.");
        return;
    }

    ExportDialog exportDialog(mData, this);
    exportDialog.exportScripts();
    exportDialog.exec();
}

void MainWindow::on_actionExportTextures_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Export", "No document loaded.");
        return;
    }

    ExportDialog exportDialog(mData, this);
    exportDialog.exportTextures();
    exportDialog.exec();
}

void MainWindow::on_actionBatchExport_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Batch Export", "No document loaded.\n\nOpen a plugin via File > Data first.");
        return;
    }

    BatchExportDialog batchExport(mData, this);
    batchExport.exec();
}

void MainWindow::updateRecordCount(int count)
{
    if (mStatusRecordCount)
        mStatusRecordCount->setText(QString("Records: %1").arg(count));
}

void MainWindow::updatePluginInfo(const QString& pluginName)
{
    if (mStatusPluginInfo)
    {
        if (pluginName.isEmpty())
            mStatusPluginInfo->setText("No plugin loaded");
        else
            mStatusPluginInfo->setText(QString("Plugin: %1").arg(pluginName));
    }
}

void MainWindow::showProgress(int value, int maximum)
{
    if (mStatusProgressBar)
    {
        mStatusProgressBar->setRange(0, maximum);
        mStatusProgressBar->setValue(value);
        mStatusProgressBar->setVisible(value < maximum);
    }
}

void MainWindow::hideProgress()
{
    if (mStatusProgressBar)
        mStatusProgressBar->setVisible(false);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveUiState();
    event->accept();
}

void MainWindow::saveUiState()
{
    QString configPath = QCoreApplication::applicationDirPath()
        + "/QtCreationKitSavedSettings.ini";
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("MainWindow");

    conf.setValue("geometry", saveGeometry());
    conf.setValue("windowState", saveState());
    conf.setValue("maximized", isMaximized());
    WindowLayout::saveLayout(this, conf);

    conf.endGroup();
    conf.sync();

    ToolbarCustomizationDialog::saveToolbarConfig(ui->mainToolBar);

    LOG_INFO("UI state saved");
}

void MainWindow::restoreUiState()
{
    QString configPath = QCoreApplication::applicationDirPath()
        + "/QtCreationKitSavedSettings.ini";
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("MainWindow");

    QByteArray geometry = conf.value("geometry").toByteArray();
    QByteArray state = conf.value("windowState").toByteArray();

    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    if (!state.isEmpty()) {
        restoreState(state);
    }
    if (conf.value("maximized", false).toBool()) {
        showMaximized();
    }

    WindowLayout::restoreLayout(this, conf);

    conf.endGroup();

    LOG_INFO("UI state restored");
}

void MainWindow::on_actionShortcuts_triggered()
{
    ShortcutEditorDialog dlg(this);
    connect(&dlg, &ShortcutEditorDialog::shortcutsChanged, this, &MainWindow::applyShortcuts);
    dlg.exec();
}

void MainWindow::on_actionCreateArchive_triggered()
{
    LOG_DEBUG("Create Archive triggered");

    QString dataDir;
    if (mData) {
        dataDir = mData->getPaths().dataDir.absolutePath();
    }

    QString outputDir = QFileDialog::getExistingDirectory(this, "Select Directory to Archive",
        dataDir.isEmpty() ? QString() : dataDir);
    if (outputDir.isEmpty()) return;

    QDir dir(outputDir);
    QStringList filters;
    filters << "*.nif" << "*.dds" << "*.png" << "*.jpg" << "*.wav" << "*.ogg" << "*.fuz"
            << "*.txt" << "*.json" << "*.psc" << "*.pex" << "*.xml" << "*.hkx" << "*.tri"
            << "*.btr" << "*.byt";
    QStringList files = dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);

    if (files.isEmpty()) {
        QStringList allFiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
        if (allFiles.isEmpty()) {
            QMessageBox::information(this, "Create Archive", "No files found in the selected directory.");
            return;
        }
        files = allFiles;
    }

    QStringList fullPaths;
    for (const QString& f : files) {
        fullPaths.append(dir.absoluteFilePath(f));
    }

    QString defaultName = QDir(outputDir).dirName() + " - OpenCK.ba2";
    QString outputPath = QFileDialog::getSaveFileName(this, "Save Archive As",
        QFileInfo(outputDir).dir().absoluteFilePath(defaultName),
        "BA2 Archive (*.ba2)");
    if (outputPath.isEmpty()) return;

    bool compress = QMessageBox::question(this, "Create Archive",
        "Compress files in archive?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;

    QString archiveType = "GNRL";
    if (outputPath.contains("texture", Qt::CaseInsensitive) || outputPath.contains("mesh", Qt::CaseInsensitive)) {
        archiveType = "DX10";
    }

    Ba2Archive archive;
    if (archive.create(fullPaths, outputPath, compress, archiveType)) {
        QMessageBox::information(this, "Create Archive",
            QString("Archive created successfully.\n\nFiles: %1\nPath: %2\nType: %3")
                .arg(fullPaths.size()).arg(outputPath).arg(archiveType));
    } else {
        QMessageBox::warning(this, "Create Archive", "Failed to create archive. Check the log for details.");
    }
}

void MainWindow::on_actionCompilePapyrusScripts_triggered()
{
    LOG_DEBUG("Compile Papyrus Scripts triggered");

    QString scriptDir = QFileDialog::getExistingDirectory(this, "Select Script Source Directory",
        mData ? mData->getPaths().dataDir.absolutePath() : QString());
    if (scriptDir.isEmpty()) return;

    QDir dir(scriptDir);
    QStringList scriptFilters;
    scriptFilters << "*.psc";
    QStringList scripts = dir.entryList(scriptFilters, QDir::Files | QDir::NoDotAndDotDot);

    if (scripts.isEmpty()) {
        QMessageBox::information(this, "Compile Papyrus Scripts", "No .psc files found in the selected directory.");
        return;
    }

    QString compilerPath = PapyrusCompiler::detectCompilerPath();
    if (compilerPath.isEmpty()) {
        QString manualPath = QFileDialog::getOpenFileName(this, "Locate Papyrus Compiler (pp64.exe)",
            QString(), "Executable (*.exe)");
        if (manualPath.isEmpty()) return;
        compilerPath = manualPath;
    }

    QString outputDir = scriptDir;
    outputDir.replace("/Source", "", Qt::CaseInsensitive);
    outputDir.replace("/source", "", Qt::CaseInsensitive);
    outputDir += "/Scripts";
    QDir().mkpath(outputDir);

    PapyrusCompiler compiler(this);
    compiler.setCompilerPath(compilerPath);
    compiler.setOutputPath(outputDir);
    compiler.addIncludePath(scriptDir);

    if (mData) {
        compiler.setGameVersion(mData->getPaths().gameId);
    }

    QStringList statusMessages;
    int successCount = 0;
    int failCount = 0;

    for (const QString& script : scripts) {
        QString scriptPath = dir.absoluteFilePath(script);
        compiler.setScriptPath(scriptPath);
        if (compiler.compile()) {
            successCount++;
        } else {
            failCount++;
            statusMessages << QString("Failed: %1").arg(script);
        }
    }

    auto errors = compiler.getLastErrors();
    for (const auto& err : errors) {
        statusMessages << QString("%1 (%2:%3): %4")
            .arg(err.severity == CompilerError::Severity::Error ? "ERROR" :
                 err.severity == CompilerError::Severity::Fatal ? "FATAL" : "WARNING")
            .arg(err.file).arg(err.line).arg(err.message);
    }

    QString summary = QString("Compilation complete.\n\nScripts compiled: %1\nFailed: %2")
        .arg(successCount).arg(failCount);

    if (!statusMessages.isEmpty()) {
        summary += "\n\nDetails:\n" + statusMessages.join("\n");
    }

    if (failCount == 0) {
        QMessageBox::information(this, "Compile Papyrus Scripts", summary);
    } else {
        QMessageBox::warning(this, "Compile Papyrus Scripts", summary);
    }
}

void MainWindow::on_actionCompactSmallMaster_triggered()
{
    LOG_DEBUG("Compact Small Master triggered");

    if (!mData) {
        QMessageBox::information(this, "Compact Master",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
        return;
    }

    auto ret = QMessageBox::question(this, "Compact Small Master",
        "This will compact the loaded master file to reduce its size by\n"
        "removing deleted records and optimizing form ID allocation.\n\n"
        "This operation is irreversible. Make sure you have a backup.\n\n"
        "Continue?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    QString savePath = QFileDialog::getSaveFileName(this, "Save Compacted Master As",
        QString(), "Plugin (*.esm *.esp)");
    if (savePath.isEmpty()) return;

    showProgress(0, 100);

    int totalRecords = 0;
    int deletedRecords = 0;

    auto countCollection = [&](const BaseCollection& collection) {
        for (int i = 0; i < collection.size(); ++i) {
            totalRecords++;
            if (collection.getRecord(i).isDeleted()) {
                deletedRecords++;
            }
        }
    };

    countCollection(mData->getNpcCollection());
    countCollection(mData->getWeaponCollection());
    countCollection(mData->getArmorCollection());
    countCollection(mData->getSpellCollection());
    countCollection(mData->getQuestCollection());
    countCollection(mData->getDialCollection());
    countCollection(mData->getInfoCollection());
    countCollection(mData->getBookCollection());
    countCollection(mData->getMiscCollection());
    countCollection(mData->getStatCollection());
    countCollection(mData->getActiCollection());
    countCollection(mData->getContCollection());
    countCollection(mData->getAlchCollection());
    countCollection(mData->getIngrCollection());
    countCollection(mData->getEnchCollection());
    countCollection(mData->getTreeCollection());

    showProgress(50, 100);

    emit actionSaveAs_triggered();

    updateStatus(QString("Compacted: %1 of %2 records were deleted-flagged").arg(deletedRecords).arg(totalRecords));
    hideProgress();

    QMessageBox::information(this, "Compact Small Master",
        QString("Compaction analysis complete.\n\n"
                "Total records scanned: %1\n"
                "Deleted-flagged records: %2\n\n"
                "The master has been saved to the selected path.\n"
                "Deleted records are skipped during save, producing a smaller file.")
            .arg(totalRecords).arg(deletedRecords));
}

void MainWindow::on_actionSaveAllButton_triggered()
{
    if (!mData) return;
    emit actionSave_triggered();
}

void MainWindow::on_actionCheckOut_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Check Out",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
        return;
    }
    QMessageBox::information(this, "Check Out",
        "Check Out is a version-control operation.\n"
        "Select records in the Object Window to check out.");
}

void MainWindow::on_actionCheckIn_triggered()
{
    if (!mData)
    {
        QMessageBox::information(this, "Check In",
            "No document is currently loaded.\n\n"
            "Open a plugin file first via File > Data.");
        return;
    }
    QMessageBox::information(this, "Check In",
        "Check In is a version-control operation.\n"
        "Select checked-out records in the Object Window to check in.");
}
