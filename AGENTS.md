# AGENTS.md — Mixxx Project Instructions

See [README.md](README.md) for a project overview, and
[CONTRIBUTING.md](CONTRIBUTING.md) for build instructions, code style,
pre-commit setup, Git workflow, and pull request guidelines.

## Key Architecture

- **ControlObject/ControlProxy**: `[Group], key_name` inter-component communication.
- **Engine thread**: Real-time audio — no allocations, no locks, may emit Qt signals but cannot receive them.
- **parented_ptr/make_parented**: Qt object-tree ownership. Object must get a parent before `parented_ptr` destructs.

## Project Layout

```text
src/          C++ source (engine/, controllers/, library/, mixer/, effects/, qml/, preferences/, util/, test/)
res/          Resources (controllers/ JS/XML, skins/, qml/)
cmake/        CMake modules
tools/        Python helper scripts
```

## QML Skins

Before changing QML skin discovery, startup, manifests, shared QML controls, or
`res/skins/LateNightQML`, read [QML_SKIN.md](QML_SKIN.md). Keep that document in
sync with behavioral or architectural changes.

Important boundaries:

- A configured QML skin is loaded with `--developer`; `--new-ui` is the separate
  standalone QML UI and must not be used as an equivalent test path.
- `res/skins/LateNightQML` is the current experimental example.
- `Mixxx` and `Mixxx.Controls` are named QML modules. Relative imports from
  `res/qml` depend on the bundled source layout and are not portable to a user
  skin directory.
- QML skin selection crosses the QWidget/QML application startup boundary and
  requires a restart. Preserve profile-safety behavior when editing that path.
