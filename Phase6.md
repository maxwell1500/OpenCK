# Phase 6: Advanced Editing - Particles (6-8 hours)

## Current State

| Component | Status |
|-----------|--------|
| ParticleEffectsParser | **STUB** — returns dummy data |
| ParticleEffectsEditor UI | **REAL** — fully coded dialog |
| NIF particle block types | **MISSING** — no NiParticleSystem/NiPSys* classes |
| ESM particle records (EFSH, EXPL) | **MISSING** |
| Viewport particle rendering | **MISSING** |
| BSLightingShaderProperty | **MISSING** |

---

## Step 6.1: NIF Particle Block Types (Estimated: 2-3 hours)

**Goal**: Parse actual particle system data from NIF files.

### Step 6.1.1: Add NiParticleSystem block class to nifrecord.hpp/cpp
- Add `class NifParticleSystem : public NifObject` with fields:
  - `quint32 numParticles`
  - `float numVisibleParticles`
  - `ParticleSystemSettings settings` (emitter type, renderer type, emission rate, etc.)
- Add `parse()` and `write()` methods
- Register in CMakeLists.txt

### Step 6.1.2: Add NiPSys* modifier block classes
- Create lightweight wrapper classes for common modifiers:
  - `NifPSysEmitter` (box/sphere/cylinder/mesh emitter types)
  - `NifPSysGravityModifier`
  - `NifPSysDragModifier`
  - `NifPSysColorModifier`
  - `NifPSysRotationModifier`
  - `NifPSysGrowFadeModifier`
- Each stores the modifier-specific parameters
- Register all in nifparser.cpp block type switch

### Step 6.1.3: Add BSLightingShaderProperty block class
- Parse shader properties (including particle shader flags)
- Register in nifparser.cpp

### Step 6.1.4: Connect particle blocks to NifParser
- In nifparser.cpp block type switch, add cases for all NiPSys* types
- Map particle nodes to the NifNode tree (particles are attached to nodes)
- Store particle system data on the node for later retrieval

### Step 6.1.5: Build verification
- `cmake --build`

---

## Step 6.2: Replace Stub Parser with Real Parser (Estimated: 1-2 hours)

**Goal**: Make ParticleEffectsParser actually parse NIF files.

### Step 6.2.1: Rewrite ParticleEffectsParser::parse()
- Instead of returning dummy data, use NifParser to load the NIF
- Walk the node tree looking for nodes with particle system data
- Extract ParticleSystemData from NifParticleSystem blocks
- Map emitter types, renderer types, and modifier parameters

### Step 6.2.2: Update ParticleSystemData struct
- Add fields for modifier data (gravity, drag, color over life, size over life)
- Add fields for emitter shape (box extents, sphere radius, etc.)
- Add texture path, blend mode, alpha test

### Step 6.2.3: Build verification
- `cmake --build`

---

## Step 6.3: Particle Editing UI Enhancements (Estimated: 1-2 hours)

**Goal**: Make the editor fully functional with real data.

### Step 6.3.1: Add missing editor fields
- Emission rate spinner
- Lifetime min/max spinners
- Speed min/max spinners
- Gravity strength spinner
- Texture path browser
- Color gradient editor (color over lifetime)
- Size curve editor (size over lifetime)

### Step 6.3.2: Add color gradient editor widget
- Create `ColorGradientWidget` — horizontal bar showing color gradient
- Click to add/remove color stops
- Drag to reposition stops
- Preview gradient

### Step 6.3.3: Add size curve editor widget
- Create `SizeCurveWidget` — graph showing size over lifetime
- Click to add/remove control points
- Drag to adjust curve
- Preview curve

### Step 6.3.4: Wire apply buttons
- Ensure applyEmitterChanges() and applyRendererChanges() work correctly
- Add undo support for particle edits

### Step 6.3.5: Build verification
- `cmake --build`

---

## Step 6.4: Particle Preview in Viewport (Estimated: 2-3 hours)

**Goal**: Live particle simulation in the 3D viewport.

### Step 6.4.1: Create ParticleSystem class
- `src/model/tools/particlesystem.hpp/cpp`
- Simulates particle emission, movement, color/size over lifetime
- Uses QTimer for simulation ticks
- Stores particle positions, velocities, colors, sizes, ages

### Step 6.4.2: Create ParticleRenderer class
- `src/view/window/particlerenderer.hpp/cpp`
- Renders particles as GL_POINTS or billboarded quads
- Uses particle texture from NIF
- Supports additive blending, alpha test
- Updates VBO each frame with current particle state

### Step 6.4.3: Integrate with NifViewportWidget
- Add particle system members to NifViewportWidget
- Load particle systems when NIF is loaded
- Start/stop simulation with play/stop buttons
- Render particles in paintEvent after geometry

### Step 6.4.4: Add particle controls to viewport toolbar
- Play/Pause button for particle simulation
- Speed control
- Reset button

### Step 6.4.5: Build verification
- `cmake --build`

---

## Step 6.5: Dead Code Cleanup (Estimated: 30 min)

### Step 6.5.1: Audit for stubs/TODOs
- Search all particle files for TODO/FIXME/HACK/stub
- Fix or document each finding

### Step 6.5.2: Final build verification
- `cmake --build`

---

## File Summary

| Action | File | Step |
|--------|------|------|
| **Edit** | `libs/files/esm/nifrecord.hpp` | 6.1.1-3 |
| **Edit** | `libs/files/esm/nifrecord.cpp` | 6.1.1-3 |
| **Edit** | `libs/files/nif/nifparser.cpp` | 6.1.4 |
| **Edit** | `libs/files/nif/particle/particleeffects.hpp` | 6.2.1-2 |
| **Edit** | `libs/files/nif/particle/particleeffects.cpp` | 6.2.1-2 |
| **Edit** | `src/view/window/particleeffectseffecteditor.hpp` | 6.3.1 |
| **Edit** | `src/view/window/particleeffectseffecteditor.cpp` | 6.3.1-4 |
| **Edit** | `src/view/window/nifviewportwidget.hpp` | 6.4.3 |
| **Edit** | `src/view/window/nifviewportwidget.cpp` | 6.4.3-4 |
| **New** | `src/view/window/colorgradientwidget.hpp` | 6.3.2 |
| **New** | `src/view/window/colorgradientwidget.cpp` | 6.3.2 |
| **New** | `src/view/window/sizecurvewidget.hpp` | 6.3.3 |
| **New** | `src/view/window/sizecurvewidget.cpp` | 6.3.3 |
| **New** | `src/model/tools/particlesystem.hpp` | 6.4.1 |
| **New** | `src/model/tools/particlesystem.cpp` | 6.4.1 |
| **New** | `src/view/window/particlerenderer.hpp` | 6.4.2 |
| **New** | `src/view/window/particlerenderer.cpp` | 6.4.2 |
| **Edit** | `CMakeLists.txt` | All |

---

## Iteration Schedule

| Iteration | Steps | Gate |
|-----------|-------|------|
| **Iter 1** | 6.1 + 6.2 | Build passes, NIF files load with real particle data |
| **Iter 2** | 6.3 | Build passes, editor shows real particle properties |
| **Iter 3** | 6.4 | Build passes, particles render in viewport |
| **Iter 4** | 6.5 | Final audit clean |
