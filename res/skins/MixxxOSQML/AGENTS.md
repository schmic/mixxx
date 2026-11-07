# AGENTS.md

MixxxOSQML is the in-tree integration of the experimental TouchQML skin.
This directory, `res/skins/MixxxOSQML/`, is the installable skin directory:
`main.qml` and `skin.ini` must remain at its root. Mixxx source is at `../../..`.
Follow the repository-root [AGENTS.md](../../../AGENTS.md), including its
required reading of [MIXXXOS.md](../../../MIXXXOS.md) and
[MIXXXOS_TODOS.md](../../../MIXXXOS_TODOS.md), as well as these skin-specific rules.
Project definitions and hardware/scaling recommendations belong in `MIXXXOS.md`;
pending implementation work belongs in `MIXXXOS_TODOS.md`, not this file.

## Workflow

- Keep skin-only implementation changes inside this skin directory.
- Do not modify Mixxx C++ sources or tests for skin-only work.
- Do not configure, build, or run Mixxx unless the user explicitly requests it.
- Run static lint from this skin directory using Qt 6's executable with
  `qmllint -I ../../../build/qml *.qml Controls/*.qml Deck/*.qml Effects/*.qml Library/*.qml Performance/*.qml Samples/*.qml Settings/*.qml Theme/*.qml`.
  The import path uses metadata from the enclosing Mixxx developer build.
- There is no skin-local build or test suite. Use the enclosing Mixxx build
  and test instructions when explicitly requested. Its `tools/qmlformat.py`
  uses the repository-root `.qmlformat.ini`; do not replace it with an
  unconfigured `qmlformat -i` run.
- When runtime verification is explicitly requested, this configured skin uses
  `mixxx --developer`; `--new-ui` loads a different standalone QML frontend.
- Read [docs/architecture.md](docs/architecture.md) before API or behavior
  changes and [docs/design.md](docs/design.md) before layout or interaction
  changes. Keep those documents synchronized with implementation changes.
  Component ownership belongs in the architecture document; exact dimensions,
  spacing, colors, and visual styling belong in the design document.

## Behavioral invariants

- Defer core-dependent content until `Mixxx.Core.ready`.
- Preserve page state when switching views; pooled `TrackRow` delegates must
  reset swipe state when reused.
- `BrowseView` handles deck and Preview Deck load controls with its selected
  URL because QML browsing does not use the legacy selected-row model.

## Compatibility

- Prefer named `Mixxx 1.0` and `Mixxx.Controls 1.0` modules.
- Do not add relative imports that depend on this repository living under
  Mixxx's source-tree `res/skins/` directory.
- QML skin APIs remain experimental; verify API names against current Mixxx
  source before adding bindings.
- Bind existing engine/application controls with `Mixxx.ControlProxy`. Use
  `Mixxx.SkinControlCreator` only for new skin-owned `[Skin]` state; duplicate
  core-owned `[Skin]` controls are rejected.
- Follow Mixxx control names, button semantics, and controller conventions as
  closely as possible. For controller interoperability, consume existing Mixxx
  controls before introducing TouchQML-specific controls; add a skin-specific
  control only when no suitable core control exists.
- QML auto-reload destroys and recreates skin-owned objects and controls. Never
  rely on their QObject identity or external connections surviving reload.

## Touch, layout, and cue safety

- Follow the viewport and touch-target requirements in
  [docs/design.md](docs/design.md), including its documented compact-target
  exceptions. Required actions must not depend on hover, right click, or
  mouse wheel.
- Never rewrite stored cue colors while rendering. Hotcue activation must
  reset on release and component destruction.
- Map Deck 1 to EffectUnit 1 and Deck 2 to EffectUnit 2. Preserve existing
  routing and enabled state while selecting a new effect or Quick Effect preset.
- All deck-aware rows use the shared center split from `TouchMainWindow.qml`;
  do not independently round splits or give dividers their own layout column.
- Do not copy Denon/Engine DJ branding or proprietary assets; use original or
  suitably licensed assets only.
