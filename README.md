# C4D_MeshCheck

A C++ plugin for **Cinema 4D (SDK 2026)** providing real-time interactive viewport highlighting of problematic mesh edges.

Designed for topology validation and detecting critical creases/creasing angles (angles between adjacent polygons exceeding 89° or a configurable threshold), similar to Cinema 4D's native Mesh Checking system.

---

## Features

- **Interactive Viewport Highlighting (Tag & Command)**:
  - The `MeshCheck` tag can be applied to any polygon object (or generator / deformer objects).
  - A menu / palette command `Toggle MeshCheck Tag` toggles or applies the tag to the selected object with a single click.
- **Polygon Angle Analysis (Edge Angle)**:
  - Automatic normal calculation (Newell's method for triangles and non-planar 3D quads).
  - Compares adjacent face normal angles against a threshold (default: **89°**).
  - Highlights edges whose dihedral angle between face normals exceeds the threshold.
- **Color Gradient**:
  - Smooth color interpolation from yellow/amber (at the 89° threshold) to bright red (approaching 180° / flipped normals).
  - Option to set a solid color instead of a gradient.
- **Line Width Control & Depth Testing**:
  - Multi-pass screen-space line rendering with subpixel antialiasing and configurable line thickness (1–10 px).
  - Depth testing (`DRAW_Z_LOWEREQUAL`) with a line Z-offset (`LineZOffset`) to eliminate Z-fighting and occlude back-facing edges behind the mesh.
- **Boundary Edge Highlighting**:
  - Optional highlighting of open mesh borders (edges belonging to only one polygon).
- **Non-Manifold Topology Support**:
  - Proper handling of non-manifold edges (more than two polygons sharing an edge).
- **High Performance (60+ FPS)**:
  - Edge indexing via packed 64-bit keys in `maxon::HashMap`.
  - Smart caching: geometry recalculation occurs only when dirty checksums change (`DIRTYFLAGS::DATA`), vertex/polygon counts change, or tag parameters are modified.

---

## Tag Parameters (Attribute Manager)

| Parameter | Description | Default Value |
|---|---|---|
| **Enable Highlighting** (`MESHCHECK_ENABLED`) | Enable or disable viewport line display | `On` |
| **Angle Threshold** (`MESHCHECK_ANGLE_THRESHOLD`) | Angle between polygons in degrees | `89°` |
| **Line Width** (`MESHCHECK_EDGE_WIDTH`) | Viewport line rendering thickness | `2.5 px` |
| **Depth Test** (`MESHCHECK_DEPTH_TEST`) | Occlude edges behind geometry (Z-buffer test) | `On` |
| **Angle Gradient** (`MESHCHECK_USE_GRADIENT`) | Smooth color transition from yellow to red | `On` |
| **Edge Color** (`MESHCHECK_EDGE_COLOR`) | Solid color (when gradient is disabled) | Orange-Red |
| **Threshold Color** (`MESHCHECK_COLOR_MIN`) | Edge color at the minimum angle threshold | Yellow / Amber |
| **180° Color** (`MESHCHECK_COLOR_MAX`) | Edge color at maximum angle / inverted normals | Bright Red |
| **Show Boundary Edges** (`MESHCHECK_SHOW_BOUNDARY`) | Enable display of open mesh boundary edges | `Off` |
| **Boundary Color** (`MESHCHECK_BOUNDARY_COLOR`) | Color of boundary edges | Cyan |
| **Status** (`MESHCHECK_INFO_COUNT`) | Number of detected problematic and boundary edges | Info string |
