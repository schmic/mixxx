# MixxxOS definitions and ground rules

Read this file alongside [AGENTS.md](AGENTS.md). Upstream instructions remain
in force. Implementation procedures, future work, and acceptance checklists
live in [MIXXXOS_TODOS.md](MIXXXOS_TODOS.md); that roadmap does not authorize
unrequested implementation.

## Product and architecture

- MixxxOS is a **performance-focused distribution/fork of Mixxx**, not a rewrite
  of its audio engine. Reuse Mixxx playback, mixer, effects, audio I/O, and
  controller stack.
- **Linux first**, including eventual ARM64/Raspberry Pi-class touchscreen
  hardware; macOS is secondary. Windows is not a current priority.
- **C++20 and Qt 6/QML**, tracking Mixxx's current `main` QML work. The interface
  targets landscape 10–14-inch touchscreens, with physical controllers
  performing most actions. TouchQML is the starting implementation.
- Recommended effective viewport: **1280 x 800 logical pixels (16:10)**,
  provided natively or by desktop scaling. Current hardware target: a 14-inch,
  1920 x 1200, 60 Hz panel at 150% scaling. Keep the native physical resolution;
  do not change user display settings automatically. Retain responsive layouts
  and other supported viewport sizes.
- This is the user's existing **direct Mixxx fork**, not `vendor/mixxx` or a
  parent repository wrapping Mixxx. All C++ changes and product resources
  belong in this Git repository.
- Do not create another repository, change remotes, add Mixxx as a submodule,
  or introduce an extra product submodule or build-time patch queue unless
  the maintainer explicitly changes this architecture.
- Prefer directly tracked source and small, focused upstream changes.

## Skin and product resources

- Skin folder and displayed name: **MixxxOSQML**, under
  `res/skins/MixxxOSQML/`, not `mixxxos/ui/`.
- Keep `main.qml` and `skin.ini` at the skin root. Preserve source licence,
  attribution, assets, documentation, and nested agent guidance.
- Vendor ordinary source files: no nested Git repository, submodule, runtime
  symlink, or mandatory external checkout/download at build or runtime.
- Preserve the experimental named `Mixxx 1.0` and `Mixxx.Controls 1.0` module
  imports. Verify APIs against the checked-out Mixxx revision.
- Configured-QML-skin runs use `--developer`, not the separate `--new-ui`
  frontend. Verify CLI switches and CMake options instead of assuming names.
- The skin is an experimental preview; do not claim complete transport,
  mixer, or UI functionality. Follow its nested `AGENTS.md` for skin work.
- Product-owned configuration, scripts, documentation, and packaging may live
  under `mixxxos/`; introduce directories only as needed.

## Library and analysis design constraints

- No mandatory import into a proprietary MixxxOS library format.
- Do not develop independent BPM, beatgrid, key, or waveform-analysis
  algorithms. Potential reuse and library-provider work are deferred tasks
  in `MIXXXOS_TODOS.md`, not first-task prerequisites.
- Preserve vendor/manual analysis and cues; never silently replace them with
  Mixxx analysis. This is a design requirement, not a claim that future
  vendor-library integrations or disabled-analysis defaults are implemented.
- Library information and analysis must be independent of device settings.
  XDG local configuration is separate from portable/vendor libraries as a
  design goal; document any existing settings/database coupling rather than
  claiming storage separation has already been implemented.

## Upstream compatibility and development safety

- Read the current `CONTRIBUTING.md` and compare root `AGENTS.md` with
  `git show upstream/main:AGENTS.md` before editing Mixxx code. Inspect policy
  changes when rebasing and preserve applicable upstream instructions plus
  the required-reading links to these project files.
- Respect Mixxx's style, formatting, architecture, Qt ownership conventions,
  and real-time audio requirements. No blocking I/O, locks, or allocations
  on real-time audio paths.
- Do not reformat unrelated upstream source or change the Mixxx licence.
- Preserve unrelated working-tree changes. Avoid destructive commands,
  overwriting real profiles, modifying real music collections, or changing
  attached DJ-device libraries during development.
- Use disposable profiles/libraries for tests. Verify `--settings-path` via
  the built executable's help before relying on it.
- Do not globally install packages or rewrite system configuration without
  the maintainer's approval.
- Work only on the explicitly requested slice; do not treat the roadmap as
  permission for library-schema changes, DSP rewrites, or UI restructuring.

## Development and Git workflow

- Default to short-lived `feature/*` branches for coherent changes unless
  the user explicitly requests work on the current branch.
- Keep the official upstream remote; do not recreate or rename it. Inspect
  `git remote -v` before updates to confirm the actual names.
- During early solo development, rebasing private MixxxOS work onto a selected
  upstream `main` revision is acceptable. Do not rebase shared work or rewrite
  published history without explicit authorization. The explicitly invoked
  sync workflow records that authorization and its safety checks.
- For each integration, record upstream base SHA, inspect incoming changes,
  run relevant builds/tests, and check the skin before proposing an update.
- Keep commits small and topical: skin import, defaults, paths, library
  integration, and similar independent changes. Commits remain human actions.
- Local `git rerere` may help with conflict reuse; the sync workflow can
  disable it for deliberate resolution. Never run automatic force-pushes.
- Shared sync workflow: `.opencode/commands/sync-upstream.md`, available as
  OpenCode `/sync-upstream` and Codex `$sync-upstream`. It owns backup creation,
  conflict handling, validation, and guarded cleanup; do not duplicate it here.

## Agent handoff

After each task, report:

1. Files added or changed and upstream/source revisions used.
2. What works versus what remains planned.
3. Exact build, test, and run commands used and actual results.
4. Compatibility assumptions, risks, and unresolved issues.
5. Human-only next steps: visual QA, DJ hardware testing, commit, and push.
