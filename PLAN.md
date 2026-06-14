# NoiseArt — План розробки

> Перенесено в репозиторій з `…/.gemini/antigravity-ide/brain/7007cc44-…/implementation_plan.md`
> та доповнено реальним станом коду станом на 2026-06-14.
> Контекст розробки: `Building NoiseArt Graphics Engine conversation.md`, довідник: `ENCYCLOPEDIA.md`.

---

## 🎯 Візія

NoiseArt еволюціонує з фоторедактора у **модульний крос-платформний (Windows/Linux) рушій для дизайну сайтів/UI на безмежному полотні** (C++ конкурент Figma/Webflow) з прототипуванням, темами та генерацією коду. Ключові принципи: **модульність і моддинг** (community може додавати режими/інструменти через реєстри), безмежне полотно з глобальними координатами, режими програми (Design / Prototype / Dev / VFX), бінди клавіш у `keybinds.json`.

---

## 🧱 Поточна архітектура

```
main → App (вікно, GL, NanoVG, ImGui, головний цикл)
        ├── LayerStack ──> дерево Layer (кожен Layer = Effect + opacity/blend/enabled + діти)
        ├── EffectRegistry (registerEffect<T>() → авто в UI)
        ├── Camera2D (zoom/pan, глобальні координати)
        ├── History (Command pattern, undo/redo)
        └── UI: Viewport / Layers / Effects / Properties / Settings
```

**Типи Effect:** растрові (`apply(in,out)` на CPU), векторні (`isVector`, NanoVG/ImDrawList), шейдерні (`isShader`, GLSL). `ITransformable` — позиція/розмір на полотні.

**Як рендериться зараз:** `ViewportPanel` обходить `flattenTree()` і малює кожен шар напряму — фото через proxy-текстуру (`AddImage`), вектори/текст через `ImDrawList`. Абсолютна позиція/opacity рахується підйомом по батьках.

---

## 🐞 Реальний стан (знайдені баги)

| # | Проблема | Причина |
|---|----------|---------|
| 1 | Дочірні шари не виділяються/не редагуються | Вибір у `LayerStack` лише по `rootIndex`; немає вибору довільного вузла дерева |
| 2 | Шейдери невидимі у viewport | Немає гілки `ShaderLayerEffect` у циклі рендеру; `renderShader()` не викликається; ефект не `ITransformable` |
| 3 | Растрові ефекти не діють на фото/вектор | `Layer::process()`/`apply()`/`processAll()` не викликаються — малюється сира текстура/примітиви |
| 4 | Немає clipping дітей по межах батька | У циклі рендеру не виставляється `PushClipRect` по батьку |
| 5 | «Circle» у меню «+» створює прямокутник | Тип фігури `VectorLayerEffect` не задається при створенні |
| — | (наслідок) `Save As` нічого не пише | `m_resultImage` ніколи не заповнюється |

---

## 🔧 План ремонту (поточний пріоритет)

### Phase A — Child layers працюють по-справжньому
- [x] **A1.** Вибір довільного вузла дерева: `LayerStack::m_selectedLayer` + `getSelectedLayer()` (самоочищення від dangling-вказівників), `rootIndexOf`, `containsLayer`.
- [x] **A2.** `PropertiesPanel` редагує `getSelectedLayer()` (працює для дітей).
- [x] **A3a.** `LayersPanel`: діти клікабельні/виділяються; фікс Circle (`setShapeType`).
- [ ] **A3b.** Меню «+» додає **у вибрану групу** як дитину; підменю **Effects** (растрові ефекти як шари-діти).
- [x] **A4.** `ViewportPanel`: клік виділяє будь-який вузол; ручки трансформації в абсолютних координатах; **clipping дітей по межах батька** (`PushClipRect`).
- [x] **A5.** Шейдери видимі: `ShaderLayerEffect` → `ITransformable` + власний FBO + `AddImage` (V-flip); працює як дитина (з clipping).

### Phase B — Ефекти на елементи (модель «контент своєї групи», підтверджено)
- [x] **B3 (фото, CPU).** Дочірні растрові фільтри фото застосовуються до його пікселів (`apply()` по черзі) у `App::updateProcessing`; результат кешується в текстуру (`OverlayEffect::applyFilters` / `getDisplayTexture`).
- [ ] **B1/B2 (загальний композитор).** Вектор/текст/шейдер-контент + сусіди в групі: рендер піддерева в FBO → readback → `apply()` фільтрів → малювання. Сайзинг по bbox піддерева.

### Phase C — Шейдери та полірування
- [ ] **C1.** Налаштування шейдера в **окремому вікні** (Shader Editor), а не лише inline у Properties.
- [ ] **C2.** Мульти-вибір і переміщення дітей разом; undo для трансформацій дітей.
- [ ] **C3.** `Save As`/експорт через композицію (Artboard як область експорту).

> **Модель (обрана користувачем):** ефект застосовується до **вмісту свого батьківського шару** (групи/артборда/фото). Щоб накласти фільтр на фото — зробити фільтр **дитиною фото**. Clipping: дитина обрізається по межах батька.

---

## 🗺️ Глобальний роадмеп (31-пунктовий CORE → фази)

> Повний опис — у `Building NoiseArt Graphics Engine conversation.md`. Коротко по етапах:

1. **CORE:** полотно (zoom/pan/grid/rulers), примітиви+шари+Z-order, живі фони (шейдери Perlin/Voronoi/Plasma), текст+шрифти. *(переважно є, доробляється)*
2. **Структура:** сторінки (Pages), Component Editor + бібліотека + instances, Asset Manager.
3. **Інтерактивність:** кнопки зі станами, меню/navbar/dropdown, форми+валідація, advanced (scroll/parallax/bezier), Prototype + Presentation режими.
4. **Токени і теми:** Design Tokens, теми (Light/Dark/Custom), тема самого редактора.
5. **Превю:** in-app превю, responsive (Desktop/Tablet/Mobile), animation превю.
6. **Генерація коду:** Code Panel (HTML/CSS/JS), генератор структури папок, портування шейдерів у WebGL, framework export (React/Vue), інспектор (елемент↔код).
7. **Імпорт/Експорт:** SVG імпорт, Figma імпорт (REST API), Export project/PNG/tokens.
8. **Розширення:** версіонування, **plugin-система (Lua/sol2)**, мініфікація/Prettier.

**App Modes:** 🎨 Design / 🔗 Prototype / 💻 Dev / 🌌 VFX — кожен ізолює свої панелі та інструменти, реєструється через `IAppMode`/`ITool` (модульно).

**Крос-платформність:** SDL3 + OpenGL + CMake + ImGui + `std::filesystem` + `portable-file-dialogs`; плагіни `.dll`/`.so`.
