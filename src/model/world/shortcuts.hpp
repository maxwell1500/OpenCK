#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QKeySequence>

// Creation Kit Keyboard Shortcuts - Exact Parity (UESP Wiki Reference)
namespace CKShortcuts {

// ============================================================================
// RENDER WINDOW NAVIGATION (Active when Render Window is focused)
// ============================================================================

// Camera Controls
// NOTE: WASD camera movement is handled in NifViewportWidget::keyPressEvent().
//       There is intentionally no key-release handler — movement is step-only,
//       matching real Creation Kit behavior (each keypress moves one step).
const QKeySequence ZoomIn = QKeySequence("WheelUp");      // Mouse wheel forward
const QKeySequence ZoomOut = QKeySequence("WheelDown");   // Mouse wheel backward
const QKeySequence Pan = QKeySequence("MiddleButton");    // Middle mouse button + drag
const QKeySequence PanAlt = QKeySequence("Space");        // Spacebar + mouse drag
const QKeySequence Orbit = QKeySequence("Shift+MouseLeft"); // Shift + mouse move (orbit)
const QKeySequence TopDown = QKeySequence("T");           // Top-down view
const QKeySequence CycleView = QKeySequence("Y");         // Cycle preset camera angles
const QKeySequence ToggleMarkers = QKeySequence("M");     // Toggle markers on/off
const QKeySequence ResetVisibility = QKeySequence("Shift+R"); // Reset all objects to normal
// NOTE: "F" frames the selected object. FocusCamera was an alias for this — removed.
const QKeySequence FrameSelected = QKeySequence("F");      // Frame selected object in view (real CK: F)
// NOTE: "Home" is context-specific: ResetCamera in the render window,
//       FirstRecord in the object window. No runtime conflict.
const QKeySequence ResetCamera = QKeySequence("Home");     // Reset camera to default position
const QKeySequence ToggleWireframe = QKeySequence("Ctrl+Shift+Z"); // Toggle wireframe (Ctrl+Shift+Z to avoid conflict with Undo)

// Object Manipulation
const QKeySequence Translate = QKeySequence("LeftButton");  // Left mouse button + drag (move)
const QKeySequence Rotate = QKeySequence("RightButton");    // Right mouse button + drag (rotate)
const QKeySequence GridSnap = QKeySequence("Q");            // Toggle grid snapping
const QKeySequence AxisX = QKeySequence("X");               // Constrain to X axis
const QKeySequence AxisY = QKeySequence("C");               // Constrain to Y axis (next to Z/X)
const QKeySequence AxisZ = QKeySequence("Z");               // Move up/down while dragging

// Object Visibility States (Press 1 repeatedly)
const QKeySequence Ghost = QKeySequence("1");               // First press: ghost/transparent
const QKeySequence Hidden = QKeySequence("1");              // Second press: fully hidden
const QKeySequence Visible = QKeySequence("1");             // Third press: fully visible

// ============================================================================
// FILE MENU
// ============================================================================
const QKeySequence NewPlugin = QKeySequence("Ctrl+N");
const QKeySequence OpenPlugin = QKeySequence("Ctrl+O");
const QKeySequence SavePlugin = QKeySequence("Ctrl+S");
const QKeySequence SaveAsPlugin = QKeySequence("Ctrl+Shift+S");
const QKeySequence ClosePlugin = QKeySequence("Ctrl+W");
const QKeySequence Exit = QKeySequence("Alt+F4");

// ============================================================================
// EDIT MENU
// ============================================================================
const QKeySequence Undo = QKeySequence("Ctrl+Z");           // Hold for rapid undo
const QKeySequence Redo = QKeySequence("Ctrl+Y");
const QKeySequence Cut = QKeySequence("Ctrl+X");
const QKeySequence Copy = QKeySequence("Ctrl+C");
const QKeySequence Paste = QKeySequence("Ctrl+V");
const QKeySequence Delete = QKeySequence("Delete");
const QKeySequence SelectAll = QKeySequence("Ctrl+A");
const QKeySequence Find = QKeySequence("Ctrl+F");
const QKeySequence FindNext = QKeySequence("F3");
const QKeySequence FindPrevious = QKeySequence("Shift+F3");

// ============================================================================
// VIEW MENU
// ============================================================================
const QKeySequence ObjectWindow = QKeySequence("F6");       // Note: F6 is Object Window in CK
const QKeySequence NifViewport = QKeySequence("F7");
const QKeySequence ScriptEditor = QKeySequence("F8");
const QKeySequence DialogueEditor = QKeySequence("F9");
const QKeySequence DialogueTree = QKeySequence("");
const QKeySequence QuestGraph = QKeySequence("F10");
const QKeySequence AIPackages = QKeySequence("Shift+F12");
const QKeySequence WeatherLight = QKeySequence("Shift+F11");
const QKeySequence Navmesh = QKeySequence("F12");
const QKeySequence WaterEditor = QKeySequence("Ctrl+F11");
const QKeySequence CellTransitions = QKeySequence("Ctrl+F12");
const QKeySequence MaterialEditor = QKeySequence("F13");
const QKeySequence PapyrusDebugger = QKeySequence("F14");
const QKeySequence FormIdEditor = QKeySequence("F15");
const QKeySequence Refresh = QKeySequence("F5");           // Refresh viewport/data

// ============================================================================
// WORLD MENU
// ============================================================================
const QKeySequence Worldspaces = QKeySequence("Ctrl+Shift+W");
const QKeySequence Cells = QKeySequence("Ctrl+Shift+C");
const QKeySequence LandscapeEditing = QKeySequence("Ctrl+L");
const QKeySequence ObjectPalette = QKeySequence("");

// ============================================================================
// PLUGINS MENU
// ============================================================================
const QKeySequence LoadOrder = QKeySequence("Ctrl+Shift+L");
const QKeySequence MasterFiles = QKeySequence("");
const QKeySequence ConflictDetection = QKeySequence("Ctrl+Shift+D");
const QKeySequence ConflictResolution = QKeySequence("Ctrl+Shift+R");
const QKeySequence PluginMerge = QKeySequence("Ctrl+Shift+M");
const QKeySequence LoadOrderOptimizer = QKeySequence("");
const QKeySequence BashedPatch = QKeySequence("Ctrl+Shift+B");
const QKeySequence ExternalTools = QKeySequence("");

// ============================================================================
// HELP MENU
// ============================================================================
const QKeySequence About = QKeySequence("");

// ============================================================================
// NAVIGATION (Object Window / Cell View)
// ============================================================================
const QKeySequence NextRecord = QKeySequence("Down");
const QKeySequence PreviousRecord = QKeySequence("Up");
const QKeySequence FirstRecord = QKeySequence("Home");
const QKeySequence LastRecord = QKeySequence("End");
const QKeySequence ExpandAll = QKeySequence("Ctrl+Plus");
const QKeySequence CollapseAll = QKeySequence("Ctrl+Minus");

// ============================================================================
// UTILITY
// ============================================================================
const QKeySequence Preferences = QKeySequence("Ctrl+P");
const QKeySequence Validate = QKeySequence("Ctrl+Shift+V");
const QKeySequence Duplicate = QKeySequence("Ctrl+D");
const QKeySequence SearchAndReplace = QKeySequence("Ctrl+H");
const QKeySequence ToggleGrid = QKeySequence("Ctrl+G");
const QKeySequence ToggleBoundingBoxes = QKeySequence("Ctrl+B");

} // namespace CKShortcuts

#endif // SHORTCUTS_H
