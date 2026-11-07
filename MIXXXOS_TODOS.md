# MixxxOS tasks and acceptance criteria

Read [MIXXXOS.md](MIXXXOS.md) for project definitions and ground rules, and
[AGENTS.md](AGENTS.md) for upstream instructions. This file holds the ordered
roadmap, implementation procedures, and acceptance checklists. It is planning
context, not permission to implement tasks without an explicit user request.

Phase 1 procedures below are preserved for reference and revalidation, not an
instruction to re-import or overwrite the existing skin. Unchecked items below
must not be treated as a fresh audit of the current checkout.

## Phase 1 scope

Integrate
[TouchQML](https://github.com/schmic/mixxx-touchqml-skin) into
`res/skins/MixxxOSQML/` in the user's existing Mixxx fork. Later phases are
listed for planning, not permission to make unrelated changes in one task.

## Phase 1 — integrate TouchQML into the existing fork

Work in the already-cloned **fork repository root**, not in a new parent repository.

### 1. Inspect the existing checkout

Before modifying anything:

```bash
git status --short --branch
git remote -v
git branch -vv
git rev-parse HEAD
ls res/skins
```

Confirm that `origin` refers to the user's fork and `upstream` refers to the official
`mixxxdj/mixxx` repository (regardless of the user's remote naming). If the checkout
has unrelated uncommitted changes, **preserve them** and do not run a destructive
reset, clean, checkout or rebase.

Check `README.md`, `CONTRIBUTING.md`, the existing root `AGENTS.md`, Mixxx's current
QML build options and skin discovery/install rules. Do not assume a CMake option name
without inspecting the checked-out revision.

### 2. Bring the skin into the fork as normal files

Required path (relative to the **repository root**):

```text
res/skins/MixxxOSQML/
├── main.qml
├── skin.ini
├── TouchMainWindow.qml
├── NavigationBar.qml
├── Controls/
├── Deck/
├── Effects/
├── Icons/
├── Library/
├── Performance/
├── Samples/
├── Settings/
├── Theme/
├── docs/
├── AGENTS.md
├── LICENSE
└── README.md
```

The source repository is `https://github.com/schmic/mixxx-touchqml-skin` on `main`.
Use a **pinned source commit** for the initial copy. An acceptable one-time workflow
(the agent may choose an equivalent non-destructive procedure) is:

```bash
SOURCE_DIR="$(mktemp -d)"
git clone --depth 1 https://github.com/schmic/mixxx-touchqml-skin.git "$SOURCE_DIR/touchqml"
git -C "$SOURCE_DIR/touchqml" rev-parse HEAD
mkdir -p res/skins/MixxxOSQML
rsync -a --exclude='/.git/' "$SOURCE_DIR/touchqml/" res/skins/MixxxOSQML/
```

**Do not** use a nested `.git/` directory, a submodule, or a runtime symlink as the
final integration. Preserve the source skin's GPL licence and attribution, including
its `LICENSE` and documentation. Do not discard its existing nested `AGENTS.md`;
read it before changes and adjust obsolete standalone-repository instructions (e.g.
old relative paths to a neighboring Mixxx checkout) for the in-tree location.

Record the source URL and exact initial source revision in `MIXXXOS.md`.
Document how to refresh the vendored skin from a later upstream skin revision, while
preserving MixxxOS-local changes. No automatic dependency on the external skin repo
at build or runtime is permitted.

Keep `main.qml` and `skin.ini` **at the root of `res/skins/MixxxOSQML/`**. Preserve
QML module/import semantics; the skin currently depends on the experimental compiled
`Mixxx 1.0` and `Mixxx.Controls 1.0` modules.

### 3. Make it identifiable as MixxxOS

Set the displayed skin name in `res/skins/MixxxOSQML/skin.ini` to `MixxxOSQML`,
while retaining its minimum display dimensions unless
validated otherwise. **Folder name and displayed skin name are distinct.** Avoid
unnecessary QML reorganizations in the first change.

Inspect Mixxx's current skin discovery mechanism and packaging/install rules.
Verify that this in-tree resource is found in **both a developer build and installed
packages**. Add the narrowest necessary build/install changes if it is not. Do not
assume that being in `res/skins` automatically covers every package format.

### 4. Build, run and smoke-test against the pinned Mixxx revision

Use the instructions in `CONTRIBUTING.md` and the checked-out branch. On Debian/Ubuntu,
Mixxx currently documents its environment setup with:

```bash
# Only if the required dependencies are not already installed:
# tools/debian_buildenv.sh setup

cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

If CMake, dependencies or test setup differ in the user's checkout, adapt and report
exact commands. Avoid globally installing packages or rewriting system configuration
without the maintainer's approval. Verify that Mixxx was built with QML support.

For the current experimental **configured QML skin**, the skin's README specifies
`--developer`: choose `MixxxOSQML` in Preferences > Interface, restart, and keep
`--developer` enabled. **Do not use `--new-ui`** for this task: that starts the
separate standalone QML frontend rather than the configured QML skin.

Use a fresh, disposable settings/profile directory for smoke tests, preferably with
Mixxx's `--settings-path` command-line option *after verifying its current spelling*
via `mixxx --help`. Never point test runs at the user's real Mixxx profile.

Run the skin's static lint according to its nested `AGENTS.md`, and test touch
interaction, track browsing, deck loading, waveforms and basic controls. The skin
is an experimental preview; **do not claim complete transport/mixer/UI functionality**.

Validate at least these landscape logical resolutions (or document untested ones):
`1024x600`, `1280x800`, `1366x768`, `1920x1080`.

### 5. First-task acceptance criteria

The first assignment is done only when:

- [ ] `res/skins/MixxxOSQML/main.qml` and `skin.ini` exist and are Git-tracked.
- [ ] All required QML files, assets, licence and documentation are included.
- [ ] No nested Git repository, submodule, hard dependency on external checkout,
      or runtime symlink is needed.
- [ ] Source revision/provenance and future skin-sync process are documented.
- [ ] Skin is discoverable in Mixxx's interface selector as `MixxxOSQML`.
- [ ] Developer build starts the configured QML skin (not standalone `--new-ui`).
- [ ] Static checks and relevant build/tests are run; any failures are documented.
- [ ] Skin works at the tested touchscreen sizes, or concrete blockers are reported.
- [ ] Existing Mixxx skins continue to work; no real library/profile was modified.
- [ ] Agent provides a concise change list, commands run, test results, issues, and
      **proposed** commit boundaries; commits remain a human action.

Stop after this vertical slice and report; do not begin large refactors or external
analysis/library integrations without a follow-up task.

## NEXT TASKS — ordered roadmap (do not implement all at once)

### Phase 2 — Product startup and defaults

1. Introduce minimal `mixxxos/` product-owned directories **only as needed**, such as
   `mixxxos/config/`, `mixxxos/scripts/`, `mixxxos/docs/` and `mixxxos/packaging/`.
   The skin itself stays in **`res/skins/MixxxOSQML/`**, not `mixxxos/ui/`.
2. Add a reproducible MixxxOS development launcher: QML skin selection, developer
   flags where still needed, fullscreen option, and diagnostic output. Use stable
   Mixxx CLI switches rather than patching source when possible.
3. Make the MixxxOS skin the **first-run default** without overwriting an existing
   user's selected skin. Document when a minimal C++ startup hook is genuinely needed.
4. Configure safe UI defaults for controller-first touch use; preserve subsequent
   user preferences and device mappings across upgrades.
5. Keep standard Mixxx behavior available as a fallback/development mode.

### Phase 3 — Linux configuration directory (`~/.config/mixxxos`)

**Goal:** On Linux, MixxxOS uses `$XDG_CONFIG_HOME/mixxxos` as its default
configuration directory, falling back to `$HOME/.config/mixxxos` when
`XDG_CONFIG_HOME` is unset, empty or not an absolute path. This is a
**MixxxOS default**, not a change to the standard Mixxx profile or a requirement
for callers to pass a command-line switch on every launch.

1. Inspect Mixxx's existing settings-directory resolution, its
   `--settings-path` option, startup order and any platform-specific logic.
   Implement the smallest maintainable MixxxOS-specific change: prefer
   configuring a default startup path or a dedicated launcher if it covers
   **all supported MixxxOS launch paths**; otherwise make a narrowly scoped
   C++ change. Preserve explicit user-selected settings paths.
2. Resolve paths using the XDG Base Directory rules on Linux; never hardcode
   `/home/<user>` or assume the configured path is `$HOME/.config`. Honor an
   absolute, non-empty `XDG_CONFIG_HOME`. If absent/invalid, resolve
   `$HOME/.config/mixxxos`. Do not expand a literal `~` in an environment
   variable as a substitute for a valid absolute path.
3. Apply this only to **MixxxOS**. A regular upstream Mixxx build must retain
   its own default configuration behavior. Keep macOS and other platforms'
   existing native settings paths unless explicitly scoped in later work.
4. On first run, create the resolved directory as needed and initialize
   MixxxOS settings without overwriting existing files or silently migrating
   `~/.mixxx`, `~/.config/Mixxx`, or another profile. Never read/write a real
   user's existing Mixxx profile as part of tests.
5. Keep library/analysis storage independent of configuration. If Mixxx's
   current `--settings-path` also relocates `mixxxdb.sqlite` or other
   application data, document that existing coupling and its consequences;
   **do not** claim that full XDG config/data/state/cache separation is
   implemented. Do not relocate or migrate a music library in this phase.
6. Document the effective path, its precedence rules and how to override it,
   and provide a diagnostic/smoke-test path suitable for CLI agents.

**Phase 3 acceptance tests (use disposable temporary `$HOME` directories):**

- [ ] With `XDG_CONFIG_HOME` unset, a new MixxxOS profile is created at
      `$HOME/.config/mixxxos`.
- [ ] With `XDG_CONFIG_HOME=/tmp/test-xdg-config` (or another temporary absolute
      directory), a new profile is created under `$XDG_CONFIG_HOME/mixxxos`.
- [ ] Empty or relative `XDG_CONFIG_HOME` values fall back to
      `$HOME/.config/mixxxos`.
- [ ] An explicit `--settings-path` (when supported by this build) overrides
      the default and is never silently rewritten.
- [ ] Existing settings persist across launches; no existing upstream Mixxx
      profile or music library is modified.
- [ ] The standard Mixxx behavior remains unchanged when MixxxOS-specific
      defaults are not enabled.
- [ ] Relevant CMake builds and tests pass, or blockers are documented; the
      agent reports the final path behavior and the specific code/configuration
      changes made.

### Phase 4 — Disable Mixxx-generated analysis by default

**Goal:** A fresh MixxxOS profile must **not automatically analyze music**.
MixxxOS is a performance-oriented application that should consume existing BPM,
beatgrids, musical keys, waveforms, and other supplied track information without
silently recalculating them. This phase changes **defaults and initialization**;
it does **not** integrate rbxport, create new analyzers, or alter vendor libraries.

1. Inspect the checked-out Mixxx revision and identify **every analysis
   producer and trigger**, rather than assuming there is one global switch.
   Start with `src/analyzer/`, the analysis scheduler, settings/preferences,
   library scan/import paths, the Analyze view, and deck-load handling. Check
   at least BPM/beat detection, key detection, waveform/overview generation,
   ReplayGain/loudness calculation, silence/intro/outro detection, and any
   other automatic or background analysis present in this revision.
2. **Default all Mixxx-generated analysis to off** for a **new MixxxOS
   profile**: no automatic full-library jobs, on-import or on-scan jobs,
   background/reanalysis jobs, or on-deck-load analysis of unanalysed tracks.
   Do not schedule a job solely because saved analysis is missing, marked
   outdated, or originated from another DJ application. Disable settings
   that would reanalyze or replace imported BPM/beatgrids or keys. Audit
   code paths that explicitly force an analyzer despite a disabled preference.
3. Do **not** remove the analyzer implementation. A user may explicitly
   enable analysis later via the existing preferences or another deliberate
   opt-in mechanism. The Analyze view or manual analysis action must not
   bypass the disabled-by-default policy silently; document whether manual
   analysis requires first enabling individual analyzers. Any opt-in must
   persist across restarts.
4. Preserve imported and manually corrected analysis **byte-for-byte where
   feasible**. Merely scanning, browsing, loading, playing, or restarting
   MixxxOS must not overwrite stored BPM, beatgrids, keys, cues, waveform
   records or ReplayGain values. **Normal metadata reading, file indexing,
   audio decoding for playback, and rendering already available waveforms
   must continue working.** Distinguish those from calculating new analysis.
   If disabling waveform generation means an unanalysed track lacks a
   waveform, document the limitation; do not silently re-enable analysis.
5. Use the **smallest maintainable mechanism**: inspect the real Mixxx
   configuration keys and their defaults in source. Prefer MixxxOS-scoped
   first-run defaults; use narrowly scoped C++ changes only if configuration
   alone cannot prevent an automatic analysis path. Keep non-MixxxOS builds'
   defaults unchanged. **Do not invent `.cfg` key names or its file syntax.**
6. Integrate with Phase 3's settings directory. If necessary, generate a
   minimal, valid **`$XDG_CONFIG_HOME/mixxxos/mixxx.cfg`** (falling back to
   **`$HOME/.config/mixxxos/mixxx.cfg`**) on first run using Mixxx's actual
   settings-writing mechanism or a verified native-format template. Initialize
   only settings needed for these defaults; never overwrite an existing
   `.cfg`, overwrite individual user choices, or recreate a disabled setting
   on every startup. Honor `--settings-path` and seed the **effective** new
   profile location, not a hardcoded directory. Preserve migrations.
7. Document the actual configuration entries/defaults and the distinction
   between disabled analysis, previously imported results and user opt-in
   in `mixxxos/docs/ANALYSIS_DEFAULTS.md`. Include any known waveform,
   normalization or UI behavior when no analysis data is available.

**Phase 4 acceptance tests (use disposable libraries and profiles only):**

- [ ] A fresh MixxxOS profile has BPM/beat, key, waveform, ReplayGain,
      silence and any other discovered automatic analyzers disabled by
      default, to the extent their settings are separately exposed.
- [ ] Starting MixxxOS, scanning a test music folder, viewing the library,
      and loading/playing an unanalysed track do **not** trigger any
      automatic analysis jobs or persist generated analysis results.
- [ ] A test track with existing BPM, beatgrid, key, cues and waveform data
      preserves those values after scan, deck load and restart. Missing or
      outdated analysis never causes automatic reanalysis.
- [ ] If `mixxx.cfg` seeding is needed, a first launch creates a valid config
      under the Phase 3 XDG default, and settings survive a second launch.
- [ ] `XDG_CONFIG_HOME` overrides and explicit `--settings-path` work; an
      existing user config (including an explicitly re-enabled analyzer) is
      not reset on startup or upgrade.
- [ ] A deliberate user opt-in allows the intended analysis operation and
      that choice persists; other analyzers remain disabled unless enabled.
- [ ] Playback, track metadata reading and display of already available
      analysis still work. Missing-waveform behavior is verified/documented.
- [ ] Ordinary upstream Mixxx behavior remains unchanged, and relevant
      build/tests pass or specific blockers are reported.

## Deferred library integrations

- [ ] Explore a common library-provider interface for supported Engine DJ and
      rekordbox libraries, with capability checks and safe read/write behavior.
- [ ] Explore reuse of rbxport's existing analysis and rekordbox-format code.
- [ ] Explore libdjinterop for Engine DJ.
- [ ] Verify preservation of vendor/manual analysis and cues across supported
      scan, browse, load, playback, and restart workflows. This remains a design
      requirement, not a claim that vendor-library preservation is implemented.

These are later integrations, not prerequisites for the TouchQML vertical slice.
Prioritize a working isolated development run and completing the relevant skin
validation before large refactors, library-schema changes, or external integrations.
