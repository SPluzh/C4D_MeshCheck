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

### 3. Hard Edges Highlighting
- **Explicit Phong Breaks**: Automatically detects and highlights edges broken via Cinema 4D's *Break Phong Shading* (`GetPhongBreak()`).
- **Normal Tag Split Detection**: Accurately detects split vertex normals on imported meshes (CAD, FBX, weighted normals).
- **Phong Tag Angle Shading Breaks**: Optional toggle (`Include Phong Tag Angle`) to highlight edges where dihedral angle exceeds the object's active Phong tag angle limit.
- **Dedicated Color**: Customizable hard edge color (default: Dodger Blue).

### 4. UV Seams Highlighting
- **Geometric UV Discontinuity Analysis**: Inspects the object's active UV map (`UVWTag`) and flags all internal edges where UV islands/charts are split.
- **Native BodyPaint Seam Synchronization**: Seamlessly checks both native UV seams (`GetUVSeams2`) and direct UVW coordinates for 100% reliability across polygon objects, generators, and deformer caches.
- **Dedicated Color**: Customizable UV seam color (default: Lime Green).

### 5. Combined Hard & UV Seam Workflow (Game-Ready Asset Checking)
- **Distinct Color for Hard + Seams**: Enables immediate identification of the 3 essential asset states:
  - *Hard Edge Only* (unseamed hard edge — potential normal map bake artifact).
  - *UV Seam Only* (soft edge along UV chart border).
  - *Hard Edge + UV Seam* (correctly seamed hard edge, default: Purple).

### 6. Boundary & Non-Manifold Topology Detection
- **Open Boundary Edges**: Optional highlighting of border edges belonging to only one polygon (default: cyan).
- **Non-Manifold Edges**: Automatically flags complex topology errors (edges shared by 3 or more polygons) in distinct magenta.

### 7. High-Quality Viewport Rendering
- **Screen-Space Anti-Aliasing**: Multi-pass screen-space line renderer with configurable thickness (1–10 px).
- **Depth Testing & Occlusion**: Hardware Z-buffer depth test (`DRAW_Z_LOWEREQUAL`) with line Z-offset (`LineZOffset`) prevents Z-fighting while properly occluding edges behind the mesh.

### 8. Instant Workflow & High Performance (60+ FPS)
- **Tag & Command Integration**:
  - Add the `MeshCheck` tag to any polygon object, generator, or deformer cache.
  - Use the `Toggle MeshCheck Tag` command to quickly apply or toggle the tag on selected objects with a single click.
- **Zero Viewport Overhead**:
  - High-speed 64-bit packed vertex key hashing via `maxon::HashMap`.
  - Smart dirty-state caching (`DIRTYFLAGS::DATA | DIRTYFLAGS::SELECT` and tag dirty state): geometry is recalculated only when mesh topology, UVs, normals, or tag settings change.

---

## Tag Parameters (Attribute Manager)

| Parameter | Description | Default Value |
|---|---|---|
| **Enable Highlighting** (`MESHCHECK_ENABLED`) | Master toggle for viewport line display | `On` |
| **Line Width** (`MESHCHECK_EDGE_WIDTH`) | Viewport line rendering thickness (1–10 px) | `2.5 px` |
| **Depth Test** (`MESHCHECK_DEPTH_TEST`) | Occlude edges behind geometry (Z-buffer test) | `On` |
| **Highlight by Angle** (`MESHCHECK_SHOW_ANGLE`) | Toggle dihedral angle threshold analysis | `On` |
| **Highlight from Angle** (`MESHCHECK_ANGLE_THRESHOLD`) | Angle starting from which edges are highlighted (in degrees) | `89°` |
| **Ignore Hard Edges** (`MESHCHECK_ANGLE_IGNORE_HARD`) | Do not highlight angle if a hard edge is set on the edge | `Off` |
| **Use Angle Gradient** (`MESHCHECK_USE_GRADIENT`) | Smooth color transition from yellow to red | `On` |
| **Edge Color** (`MESHCHECK_EDGE_COLOR`) | Solid color (when gradient is disabled) | Orange-Red |
| **Color at Min Angle** (`MESHCHECK_COLOR_MIN`) | Edge color at the minimum angle threshold | Yellow / Amber |
| **180° Color** (`MESHCHECK_COLOR_MAX`) | Edge color at maximum angle / inverted normals | Bright Red |
| **Highlight Hard Edges** (`MESHCHECK_SHOW_HARD_EDGES`) | Highlight Phong breaks and split normals | `On` |
| **Hard Edge Color** (`MESHCHECK_HARD_EDGE_COLOR`) | Viewport color for hard edges | Dodger Blue |
| **Highlight UV Seams** (`MESHCHECK_SHOW_UV_SEAMS`) | Highlight UV island seams / chart borders | `On` |
| **UV Seam Color** (`MESHCHECK_UV_SEAM_COLOR`) | Viewport color for UV seams | Lime Green |
| **Distinct Color for Hard + Seam** (`MESHCHECK_SHOW_HARD_SEAM_DIFF`) | Use distinct color when edge is both Hard and UV Seam | `On` |
| **Hard & UV Seam Color** (`MESHCHECK_HARD_SEAM_COLOR`) | Color when an edge is both a Hard Edge and UV Seam | Purple |
| **Highlight Boundary Edges** (`MESHCHECK_SHOW_BOUNDARY`) | Enable display of open mesh boundary edges | `Off` |
| **Boundary Color** (`MESHCHECK_BOUNDARY_COLOR`) | Color of open mesh boundary edges | Cyan |
| **Status** (`MESHCHECK_INFO_COUNT`) | Real-time counts of angle, hard, UV seam, and boundary edges | Info string |

---

## Installation

1. Download the release package: [`C4D_MeshCheck_v1.0.2.zip`](C4D_MeshCheck_v1.0.2.zip).
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
