# CLAUDE.md

Цей файл містить настанови для Claude Code (claude.ai/code) під час роботи з кодом у цьому репозиторії.

## Проєкт

NoiseArt — це кросплатформний (Windows/Linux) шаровий редактор зображень, написаний на C++17. Це десктопний застосунок з графічним інтерфейсом (SDL3 + OpenGL 4.6 + Dear ImGui), а не бібліотека, і він **не має автоматизованих тестів**. Коментарі в коді написані українською; дотримуйся цієї мови, редагуючи наявні блоки коментарів, але ідентифікатори в коді — англійською.

## Збірка та запуск

Залежностями керує **vcpkg у режимі маніфесту** (`vcpkg.json`). Шлях до toolchain жорстко прописаний як `X:/vcpkg` у `CMakePresets.json` (`VCPKG_ROOT`) — зміни цей блок env, якщо vcpkg знаходиться в іншому місці.

```sh
cmake --preset default          # конфігурація (вперше / після змін у vcpkg.json)
cmake --build build --config Debug      # збірка Debug  (або: cmake --build --preset debug)
cmake --build build --config Release    # збірка Release (або: cmake --build --preset release)
build/Debug/NoiseArt.exe        # запуск
```

- Папка `resources/` (шрифти) копіюється поряд з виконуваним файлом на кроці POST_BUILD, тож exe треба запускати за наявності `resources/` поруч (збірка робить це автоматично).
- `compile_commands.json` генерується в `build/` для використання IDE/clangd.
- Конфігурації лінтера/форматування немає; дотримуйся стилю навколишнього коду.

## Домовленості

- Усе знаходиться в `namespace NoiseArt`.
- Шляхи включень (include) відраховуються від `src/` (напр. `#include "core/Image.h"`, `#include "effects/Effect.h"`), а не відносні шляхи.
- **Додавання нового файлу `.cpp` вимагає додавання його до списку `add_executable(...)` у `CMakeLists.txt`** — glob не використовується. Файли, що складаються лише із заголовка (header-only, як більшість ефектів), не потребують змін у CMake.

## Архітектура

`src/main.cpp` — це тонка точка входу; уся логіка живе в `App` (`src/App/App.cpp`), який володіє вікном SDL, контекстом GL, контекстом NanoVG, налаштуванням ImGui, `LayerStack`, `EffectRegistry` та кожною панеллю UI.

Головний цикл (`App::run`): `processEvents` → `beginFrame` → ImGui DockSpace → `renderMenuBar` → `renderCanvasToFBO` (вектори NanoVG → FBO) → `renderUI` (панелі) → `updateProcessing` → `endFrame`.

### Система ефектів (основна абстракція)

`Effect` (`src/effects/Effect.h`) — це абстрактний контракт, який реалізує кожен ефект. Існує **три різновиди**, що обираються віртуальними прапорцями:

1. **Растровий/CPU** — реалізує `apply(const Image& input, Image& output)` для перетворення пікселів на CPU. Більшість ефектів (`NoiseEffect`, `BlurEffect`, `BloomEffect`, `InvertEffect`, `BrightnessContrast`, `AdjustmentsEffect`, `BlockifyEffect`, `ThresholdEffect`, `OverlayEffect`, …) — це header-only файли в `src/effects/`.
2. **Векторний** — перевизначає `isVector()→true` та `renderVector(NVGcontext*)` (`VectorLayerEffect`, `TextLayerEffect`).
3. **Шейдерний** — перевизначає `isShader()→true` та `renderShader(w,h,time)` для GLSL (`ShaderLayerEffect`: Plasma/Voronoi/Perlin).

Кожен ефект також реалізує `getName()`, `getCategory()` (групує його в меню панелі Effects), `renderUI()` (малює елементи керування ImGui, повертає `true`, коли параметр змінився → запускає переобробку) та `clone()` (глибока копія, для дублювання шару). Ефекти, які можна переміщувати/масштабувати у viewport, також реалізують `ITransformable` і повертають його з `getTransformable()`.

**Щоб додати ефект:** напиши клас, що успадковує `Effect` (header-only у `src/effects/` — це норма), потім у `App::init` (`src/App/App.cpp`) додай `#include` для нього та один рядок: `m_registry.registerEffect<YourEffect>();`. `EffectRegistry` зчитує name/category з тимчасового екземпляра і зберігає фабричну лямбду; панель Effects автоматично наповнюється з нього — код UI чіпати не треба.

### Модель шарів

- `Layer` (`src/core/Layer.h`) = один `Effect` + метадані (назва, непрозорість, увімкнено, `BlendMode`) + дерево дочірніх шарів.
- `LayerStack` (`src/core/LayerStack.h`) тримає кореневі шари як дерево: множинний вибір (`std::set<int>`), перевпорядкування drag-and-drop, вкладення/розвкладення, `flattenTree()` для лінійної ітерації UI/рендерингу та прапорець dirty.
- Скасування/повтор (undo/redo): `History` (`src/core/History.h`) — це стек команд (`AddLayerCommand`, `RemoveLayerCommand`, `MoveLayerCommand`, `ChangeLayerCommand`); команди виконуються *до* того, як їх додають у стек.

### Рендеринг — важливий нюанс

Існує два шляхи коду, і **активний лише один**:

- **Активний (те, що ти бачиш):** `ViewportPanel` (`src/ui/panels/ViewportPanel.cpp`) рендерить кожен шар напряму кожного кадру — шари-зображення через `drawList->AddImage` з використанням GPU proxy-текстури (`OverlayEffect::getProxyTexture()`), вектор/текст через примітиви ImGui `ImDrawList`, а шейдерний/NanoVG-вміст через FBO, який заповнює `App::renderCanvasToFBO`. `Camera2D` (`src/core/Camera2D.h`) забезпечує панорамування/масштабування та відображення world↔screen; viewport малює маркери виділення й обробляє перетягування переміщення/масштабування з опційним прив'язуванням до сітки.
- **Рудиментарний (CPU-композитинг):** `Effect::apply` → `Layer::process` (змішування за непрозорістю/режимом змішування) → `LayerStack::processAll` створює єдине скомпоноване `Image`. **`processAll` наразі не має викликів, `m_resultImage`/`m_resultTexture` ніколи не заповнюються, а `App::updateProcessing` лише скидає прапорець dirty.** Як наслідок, `File > Save As` (`App::saveImage`) нічого не записує, бо `m_resultImage` лишається порожнім — щоб налаштувати експорт, треба пропустити стек шарів через цей шлях (або захопити viewport/FBO). Не очікуй, що редагування `apply()` змінить вивід на екрані для шарів зображень/векторів/шейдерів.

### Панелі UI (`src/ui/panels/`)

`ViewportPanel` (полотно), `LayersPanel` (дерево шарів), `EffectsPanel` (каталог ефектів → додає шари), `PropertiesPanel` (`renderUI` вибраного шару), `SettingsPanel` (сітка/прив'язка/налаштування застосунку). `App::renderUI` керує ними; панель, що повертає `true`, позначає стек шарів як dirty.

### Конфігурація

Налаштовувані константи (вікно, сітка, масштаб, маркери, виділення, панелі, типові значення шарів/нових об'єктів, шрифти) живуть у `src/Config.h` — зміни значення, перезбери, побач результат. Дивись `README_SETTINGS.md` для анотованої таблиці.

## Примітки

- `scratch/` містить одноразові Python-скрипти, що використовувалися під час розробки, і не є частиною збірки. `tools/download_font.py` завантажує вбудований шрифт.
- Ввід/вивід зображень використовує stb (`src/core/stb_impl.cpp` — це єдина одиниця трансляції, що визначає реалізації stb). Зображення завжди приводяться до 4-канального RGBA при завантаженні.
