# NoiseArt — Settings Reference

All configurable constants are in [`src/Config.h`](src/Config.h).  
Change a value → rebuild → see the change.

---

## Window

| Constant | Default | Description |
|----------|---------|-------------|
| `WINDOW_WIDTH` | `1280` | Initial window width (px) |
| `WINDOW_HEIGHT` | `720` | Initial window height (px) |
| `WINDOW_TITLE` | `"NoiseArt — Редактор зображень"` | Window title bar text |

## Grid

| Constant | Default | Description |
|----------|---------|-------------|
| `GRID_DEFAULT_SIZE` | `100.0` | Base grid cell size in world pixels |
| `GRID_SHOW_DEFAULT` | `true` | Show grid on startup |
| `GRID_AUTO_SCALE_DEFAULT` | `true` | Auto-adjust grid subdivisions on zoom |
| `GRID_SNAP_DEFAULT` | `false` | Snap objects to grid by default |
| `GRID_MIN_PIXEL_SIZE` | `4.0` | Minimum visible grid line spacing (screen px) |
| `GRID_LINE_COLOR` | `(200,200,200,40)` | Grid line color (RGBA 0-255) |
| `AXIS_X_COLOR` | `(255,100,100,80)` | X/Y axis color |

## Zoom

| Constant | Default | Description |
|----------|---------|-------------|
| `ZOOM_MIN` | `0.05` | Minimum zoom level (5%) |
| `ZOOM_MAX` | `20.0` | Maximum zoom level (2000%) |
| `ZOOM_STEP` | `1.15` | Zoom multiplier per scroll step |

## Transform Handles

| Constant | Default | Description |
|----------|---------|-------------|
| `HANDLE_RADIUS` | `6.0` | Visual handle circle radius |
| `HANDLE_HIT_RADIUS` | `12.0` | Click detection radius (larger = easier to grab) |
| `HANDLE_COLOR` | `(0,150,255,255)` | Handle color (single selection) |
| `HANDLE_COLOR_MULTI` | `(255,200,0,200)` | Handle color (multi selection) |
| `HANDLE_BORDER_WIDTH` | `2.0` | Selection border thickness |

## Selection

| Constant | Default | Description |
|----------|---------|-------------|
| `SELECTION_COLOR` | `(0,150,255,255)` | Single selection frame color |
| `MULTI_SELECTION_COLOR` | `(255,200,0,180)` | Multi-selection frame color |

## Panels

| Constant | Default | Description |
|----------|---------|-------------|
| `PANEL_VIEWPORT_OPEN` | `true` | Viewport panel open on start |
| `PANEL_LAYERS_OPEN` | `true` | Layers panel open on start |
| `PANEL_EFFECTS_OPEN` | `true` | Effects panel open on start |
| `PANEL_PROPERTIES_OPEN` | `true` | Properties panel open on start |
| `PANEL_SETTINGS_OPEN` | `false` | Settings panel open on start |

## Layers

| Constant | Default | Description |
|----------|---------|-------------|
| `LAYER_DEFAULT_OPACITY` | `1.0` | Default opacity for new layers |
| `LAYER_TREE_INDENT` | `20` | Pixels of indentation per tree depth level |
| `LAYER_DRAG_THRESHOLD` | `5.0` | Mouse movement threshold to start drag |

## New Layer Defaults

| Constant | Default | Description |
|----------|---------|-------------|
| `NEW_RECT_WIDTH` | `200.0` | Default rectangle width |
| `NEW_RECT_HEIGHT` | `200.0` | Default rectangle height |
| `NEW_CIRCLE_RADIUS` | `100.0` | Default circle radius |
| `NEW_TEXT_FONT_SIZE` | `48.0` | Default text font size |
| `NEW_TEXT_X` / `NEW_TEXT_Y` | `200.0` | Default text position |

## Fonts

| Constant | Default | Description |
|----------|---------|-------------|
| `IMGUI_FONT_SIZE` | `18.0` | UI font size |
| `DEFAULT_FONT_PATH` | `"resources/fonts/Inter.ttf"` | Path to default font file |

---

## How to customize

1. Open `src/Config.h`
2. Change the value you need
3. Rebuild: `cmake --build build --config Debug`
4. Run: `build/Debug/NoiseArt.exe`
