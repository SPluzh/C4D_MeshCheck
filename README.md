# C4D_MeshCheck

A high-performance C++ plugin for **Maxon Cinema 4D (2025 & 2026)** providing real-time interactive viewport highlighting of problematic mesh edges and topology validation.

Inspired by Cinema 4D's native modeling mesh checker, **C4D_MeshCheck** provides instant visual feedback directly in the viewport for sharp creases, planar breaks, inverted normals, open boundaries, and non-manifold topology.

---

## Features

### 1. Dihedral Angle Analysis (Highlight from Angle)
- **Automatic Normal Calculation**: Evaluates accurate face normals using Newell's method for arbitrary triangles and non-planar 3D quads.
- **Configurable Angle Threshold (`Highlight from Angle`)**:
  - Measures the dihedral angle between adjacent polygon face normals:
    - **`0°`**: Coplanar polygons (flat surface).
    - **`90°`**: Perpendicular polygons (standard cube edges).
    - **`180°`**: Polygons folded back-to-back or adjacent faces with inverted normals.
  - Highlights all edges where the angle between adjacent faces is **greater than or equal to** this threshold.
  - Default threshold is **`89°`** — cleanly identifying perpendicular edges and sharp creases while ignoring flat or smoothly subdivided surfaces.

### 2. Smooth Angle Gradient & Color Customization
- **Dynamic Color Interpolation**:
  - Renders edges using a smooth color ramp: starts at **Color at Min Angle** (default: amber/yellow) at the threshold angle and transitions to **180° Color** (default: bright red) as the crease approaches 180°.
- **Solid Color Option**: Disable the gradient to render all highlighted edges with a single uniform color.

### 3. Boundary & Non-Manifold Topology Detection
- **Open Boundary Edges**: Optional highlighting of border edges belonging to only one polygon (default: cyan).
- **Non-Manifold Edges**: Automatically flags complex topology errors (edges shared by 3 or more polygons) in distinct magenta.

### 4. High-Quality Viewport Rendering
- **Screen-Space Anti-Aliasing**: Multi-pass screen-space line renderer with configurable thickness (1–10 px).
- **Depth Testing & Occlusion**: Hardware Z-buffer depth test (`DRAW_Z_LOWEREQUAL`) with line Z-offset (`LineZOffset`) prevents Z-fighting while properly occluding edges behind the mesh.

### 5. Instant Workflow & High Performance (60+ FPS)
- **Tag & Command Integration**:
  - Add the `MeshCheck` tag to any polygon object, generator, or deformer cache.
  - Use the `Toggle MeshCheck Tag` command to quickly apply or toggle the tag on selected objects with a single click.
- **Zero Viewport Overhead**:
  - High-speed 64-bit packed vertex key hashing via `maxon::HashMap`.
  - Smart dirty-state caching (`DIRTYFLAGS::DATA`): geometry is recalculated only when mesh topology or tag settings change.

---

## Tag Parameters (Attribute Manager)

| Parameter | Description | Default Value |
|---|---|---|
| **Enable Highlighting** (`MESHCHECK_ENABLED`) | Enable or disable viewport line display | `On` |
| **Highlight from Angle** (`MESHCHECK_ANGLE_THRESHOLD`) | Angle starting from which edges are highlighted (in degrees) | `89°` |
| **Line Width** (`MESHCHECK_EDGE_WIDTH`) | Viewport line rendering thickness (1–10 px) | `2.5 px` |
| **Depth Test** (`MESHCHECK_DEPTH_TEST`) | Occlude edges behind geometry (Z-buffer test) | `On` |
| **Use Angle Gradient** (`MESHCHECK_USE_GRADIENT`) | Smooth color transition from yellow to red | `On` |
| **Edge Color** (`MESHCHECK_EDGE_COLOR`) | Solid color (when gradient is disabled) | Orange-Red |
| **Color at Min Angle** (`MESHCHECK_COLOR_MIN`) | Edge color at the minimum angle threshold | Yellow / Amber |
| **180° Color** (`MESHCHECK_COLOR_MAX`) | Edge color at maximum angle / inverted normals | Bright Red |
| **Highlight Boundary Edges** (`MESHCHECK_SHOW_BOUNDARY`) | Enable display of open mesh boundary edges | `Off` |
| **Boundary Color** (`MESHCHECK_BOUNDARY_COLOR`) | Color of open mesh boundary edges | Cyan |
| **Status** (`MESHCHECK_INFO_COUNT`) | Count of detected problematic and boundary edges | Info string |

---

## Installation

1. Download the release package: [`C4D_MeshCheck_v1.0.0.zip`](C4D_MeshCheck_v1.0.0.zip).
2. Extract the folder corresponding to your Cinema 4D version (`2025` or `2026`) into your Cinema 4D `plugins` directory:
   - **Windows:** `C:\Program Files\Maxon Cinema 4D 2026\plugins\C4D_MeshCheck`
   - *Or your custom plugins path configured in Preferences → Plugins.*
3. Restart Cinema 4D.

---

## Assigning a Shortcut

To toggle the tag quickly via keyboard:

1. In Cinema 4D, open **Window → Customization → Customize Commands...** (or press `Shift + F12`).
2. Search for: `Toggle MeshCheck Tag` (or `Переключить тег MeshCheck`).
3. Select the command, click the **Shortcut** input field, and enter your shortcut.
4. Click **Assign**.
