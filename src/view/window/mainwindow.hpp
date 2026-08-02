#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QLabel>
#include <QProgressBar>

#include <DockManager.h>
#include <DockWidget.h>

class Data;
class UndoStack;
class NifViewportWidget;
class ScriptEditorWidget;
class DialogueEditorWidget;
class FormIdEditorWidget;
class AssetBrowserWidget;
class ObjectWindowDialog;
class ExportDialog;
class BatchExportDialog;
class ConflictResolutionDialog;
class DialogueTreeEditor;
class QuestGraphEditor;
class AIPackageEditor;
class WeatherLightEditor;
class NavmeshEditor;
class WaterEditor;
class CellTransitionsEditor;
class LandscapeEditor;
class ObjectPalette;
class BashedPatchDialog;
class CellViewPanel;
class InspectorWidget;
class WarningsDockWidget;
struct CellRecord;

namespace Ui {
class mainwindow;
}

/// Main application window hosting dockable editor panels and menus.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /// Sets the active Data model backing the editor windows.
    void setData(Data* data);
    /// Updates the status bar with a transient message.
    void updateStatus(const QString& message);
    void updateRecordCount(int count);
    void updatePluginInfo(const QString& pluginName);
    void showProgress(int value, int maximum);
    void hideProgress();

signals:
    void actionData_triggered();
    void actionSave_triggered();
    void actionNewPlugin_triggered();
    void actionSaveAs_triggered();
    void actionClosePlugin_triggered();
    void actionSettings_triggered();
    void actionObjectWindow_triggered();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionExit_triggered();
    void on_actionSaveLayout_triggered();
    void on_actionLoadLayout_triggered();
    void on_actionData_triggered();
    void on_actionNewPlugin_triggered();
    void on_actionSaveAs_triggered();
    void on_actionClosePlugin_triggered();
    void on_actionOpenButton_triggered();
    void on_actionSave_triggered();
    void on_actionSaveButton_triggered();
    void on_actionSettings_triggered();
    void on_actionObjectWindow_triggered();
    void on_actionPreferences_triggered();
    void on_actionValidate_triggered();
    void on_actionSearchAndReplace_triggered();
    void on_actionAbout_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionLoadOrder_triggered();
    void on_actionMasterFiles_triggered();
    void on_actionConflictDetection_triggered();
    void on_actionNifViewport_triggered();
    void on_actionScriptEditor_triggered();
    void on_actionDialogueEditor_triggered();
    void on_actionFormIdEditor_triggered();
    void on_actionAssetBrowser_triggered();
    void on_actionPluginMerge_triggered();
    void on_actionConflictResolution_triggered();
    void on_actionDialogueTree_triggered();
    void on_actionQuestGraph_triggered();
    void on_actionAIPackages_triggered();
    void on_actionWeatherLight_triggered();
    void on_actionNavmesh_triggered();
    void on_actionWater_triggered();
    void on_actionCellTransitions_triggered();
    void on_actionMaterialEditor_triggered();
    void on_actionPapyrusDebugger_triggered();
    void on_actionLoadOrderOptimizer_triggered();
    void on_actionExternalTools_triggered();
    void on_actionBnetLogin_triggered();
    void on_actionBnetLogout_triggered();
    void on_actionBnetUpload_triggered();
    void on_actionSoundEditor_triggered();
    void on_actionAnimationEditor_triggered();
    void on_actionParticleEffectsEditor_triggered();
    void on_actionSpellWizard_triggered();
    void on_actionWorldspaces_triggered();
    void on_actionCells_triggered();
    void on_actionLandscapeEditing_triggered();
    void on_actionObjectPalette_triggered();
    void on_actionModManager_triggered();
    void on_actionBashedPatch_triggered();
    void on_actionAssetValidation_triggered();
    void on_actionAssetDependencyScanner_triggered();
    void on_actionCopy_triggered();
    void on_actionCut_triggered();
    void on_actionPaste_triggered();
    void on_actionDelete_triggered();
    void on_actionSelectAll_triggered();
    void on_actionDuplicate_triggered();
    void on_actionFindNext_triggered();
    void on_actionFindPrevious_triggered();
    void on_actionNextRecord_triggered();
    void on_actionPreviousRecord_triggered();
    void on_actionFirstRecord_triggered();
    void on_actionLastRecord_triggered();
    void on_actionExpandAll_triggered();
    void on_actionCollapseAll_triggered();
    void on_actionExportDialogue_triggered();
    void on_actionExportScripts_triggered();
    void on_actionExportTextures_triggered();
    void on_actionBatchExport_triggered();
    void on_actionShortcuts_triggered();
    void on_actionCreateArchive_triggered();
    void on_actionCompilePapyrusScripts_triggered();
    void on_actionCompactSmallMaster_triggered();
    void on_actionSaveAllButton_triggered();
    void on_actionCheckOut_triggered();
    void on_actionCheckIn_triggered();
    void runValidation();

private:
void setupEditMenu();
void setupTerrainMenu();
void setupPrimitivePreviewMenu();
    void setupShortcuts();
    void applyShortcuts();
    void updateUndoRedoActions();
    void saveUiState();
    void restoreUiState();

    Ui::mainwindow *ui;
    Data* mData;
    UndoStack* mUndoStack;
    QAction* mUndoAction;
    QAction* mRedoAction;
    QMenu* mExportMenu;
    QList<QAction*> mDynamicActions;
    QList<QShortcut*> mDynamicShortcuts;
    NifViewportWidget* nifViewportWidget;
    ScriptEditorWidget* scriptEditorWidget;
    DialogueEditorWidget* dialogueEditorWidget;
    FormIdEditorWidget* formIdEditorWidget;
    AssetBrowserWidget* assetBrowserWidget;
    ObjectWindowDialog* objectWindowDock;
    LandscapeEditor* landscapeEditor;
    ads::CDockWidget* landscapeDock;
    ObjectPalette* objectPalette;
    CellViewPanel* mCellViewPanel;
    ads::CDockWidget* mCellViewDock;
    ads::CDockWidget* mWarningsDock;
    WarningsDockWidget* mWarningsWidget;
    InspectorWidget* mInspectorWidget;
    ads::CDockWidget* mInspectorDock;
    ads::CDockManager* mDockManager;
    QLabel* mStatusRecordCount;
    QLabel* mStatusPluginInfo;
    QLabel* mStatusCellCoords;
    QLabel* mStatusSelectedObject;
    QProgressBar* mStatusProgressBar;
    QString mBnetToken;
    bool mViewportRefsConnected = false;
    bool mCellViewConnected = false;
};

#endif //MAINWINDOW_H
