# Phase 7: Workflow Improvements (6-8 hours)

## Current State

| Component | Status |
|-----------|--------|
| PluginMergeDialog | **REAL** — 17 record types, editor ID renaming, FormID reassignment |
| BashedPatchGenerator | **REAL** — 17 merge flags, dedup by editor ID, ESM output |
| ConflictDialog | **REAL** — 18 record types, cross-plugin ID conflict detection |
| ConflictResolutionDialog | **REAL** — Split-pane, Keep A/B/Remove, ESM export |
| LoadOrderDialog | **REAL** — Simple reorder with add/remove |
| LoadOrderOptimizerDialog | **REAL** — Dependency graph, LOOT integration, auto-order |
| LootWrapper | **REAL** — LOOT.exe detection, QProcess sorting |
| ModManagerDetection | **REAL** — MO2/Vortex detection, profiles |
| SearchDialog | **REAL** — 26 types, field filter, edit/clone/delete/batch |
| SearchAlgorithm | **REAL** — Regex support exists but NOT wired to UI |
| SavedSearch | **STUB** — struct defined, no persistence |
| Search History | **MISSING** |
| Find/Replace | **MISSING** — Ctrl+H opens search, no replace |

---

## Step 7.1: Enhanced Plugin Merge (Estimated: 1-2 hours)

**Goal**: Add conflict resolution dialog, auto-resolve, merge preview.

### Step 7.1.1: Add FACT_ merge support to PluginMergeDialog
- In `mergeType()`, FACT_ currently logs "not yet supported"
- Implement FACT_ merge similar to other record types

### Step 7.1.2: Add merge preview panel
- Before executing merge, show a preview of what will be merged
- Display record counts per type, conflicts detected, editor IDs that will be renamed
- Allow user to review before committing

### Step 7.1.3: Add auto-resolve for common conflicts
- When merging, if both plugins have the same editor ID, auto-resolve by:
  - Keeping the version with higher FormID (more recent plugin)
  - Renaming the older version with `_merged` suffix
- Log all auto-resolutions for user review

### Step 7.1.4: Build verification

---

## Step 7.2: Load Order Optimization Enhancements (Estimated: 1-2 hours)

**Goal**: Better dependency analysis and validation.

### Step 7.2.1: Enhance dependency graph analysis
- Parse TES4 header MAST records from all loaded plugins
- Build a complete dependency graph (not just ESM-before-ESP)
- Detect circular dependencies with DFS, not just pairwise

### Step 7.2.2: Add load order validation
- Check for missing master files
- Check for version conflicts
- Check for overlapping FormID ranges
- Display validation results in a report

### Step 7.2.3: Add "Auto-Fix" button
- Automatically resolve ordering issues detected by validation
- Move missing masters to correct positions
- Warn about unresolvable issues

### Step 7.2.4: Build verification

---

## Step 7.3: Enhanced Search (Estimated: 2-3 hours)

**Goal**: Wire regex to UI, add saved searches, search history, multi-criteria.

### Step 7.3.1: Wire regex toggle to SearchDialog UI
- Add a "Regex" checkbox next to the search input
- When checked, use SearchAlgorithm::MatchMode::Regex
- Show regex syntax errors in status bar

### Step 7.3.2: Add search history
- Store last 20 searches in QSettings
- Display in a QComboBox dropdown
- Clear history button

### Step 7.3.3: Add saved searches
- "Save Search" button → dialog to name the search
- "Load Search" dropdown → list of saved searches
- Persist to QSettings as JSON
- Include: search text, type filter, field filter, regex flag

### Step 7.3.4: Add multi-criteria filter
- "Add Criterion" button → adds another search row
- Each row has: field selector, match mode, search text
- AND/OR logic toggle between criteria
- Results must match ALL criteria (AND) or ANY criterion (OR)

### Step 7.3.5: Build verification

---

## Step 7.4: Shortcuts and Integration (Estimated: 30 min)

### Step 7.4.1: Assign shortcuts to all workflow tools
- PluginMerge: Ctrl+Shift+M
- ConflictDetection: Ctrl+Shift+D
- ConflictResolution: Ctrl+Shift+R
- LoadOrder: Ctrl+Shift+L

### Step 7.4.2: Build verification

---

## Step 7.5: Dead Code Cleanup (Estimated: 30 min)

### Step 7.5.1: Audit for stubs/TODOs
### Step 7.5.2: Final build verification

---

## Iteration Schedule

| Iteration | Steps | Gate |
|-----------|-------|------|
| **Iter 1** | 7.1 + 7.2 | Build passes, merge/optimization enhanced |
| **Iter 2** | 7.3 | Build passes, search fully functional |
| **Iter 3** | 7.4 + 7.5 | Final audit clean |
