# Touchscreen QML Skin

## Goal

Create a new QML-based Mixxx skin optimized for landscape touchscreens in the
10–14 inch range. Its interaction model and visual direction should feel
familiar to users of Denon DJ's Engine DJ 5.x interface, while remaining an
original Mixxx design and using only Mixxx-owned or suitably licensed assets.

The skin should prioritize the controls needed during a performance, make them
easy to operate by touch, and remain readable on relatively small displays.

## Deliverable

Add a new bundled experimental skin under:

```text
res/skins/TouchQML/
```

It must have its own `skin.ini`, `main.qml`, components, theme, and preview
image(s). Do not turn `LateNightQML` into the touchscreen skin. Reuse suitable
Mixxx QML APIs and shared controls instead of duplicating their behavior.

The initial implementation is an MVP, but it must be coherent and usable for a
basic two-deck mixing workflow rather than being a static visual mock-up.

## Target Displays

Physical screen size alone is not available reliably to QML and varies with
display scaling, so design and test using Qt logical pixels.

- Minimum supported landscape viewport: 1024 × 600 logical pixels.
- Primary design viewport: 1280 × 800 logical pixels.
- Also verify scaling at 1366 × 768 and 1920 × 1080.
- Treat the supplied Engine DJ visual reference as a 1920 × 1080 composition at
  100% display scaling. Use it for hierarchy and proportions, not for literal
  pixel sampling from the photograph.
- Portrait layout is out of scope for the MVP.
- Essential controls must not overlap, clip, or require precise pointer input at
  the minimum viewport.

## Experience And Visual Direction

Use Engine DJ 5.x as a high-level reference for information hierarchy,
touch-first interaction, dense but legible deck presentation, and a dark
performance-oriented appearance. Do not make a pixel-for-pixel clone and do not
copy Denon/Engine DJ branding, icons, screenshots, fonts, or other proprietary
assets.

The original Mixxx design should have:

- A dark, high-contrast theme suitable for a booth.
- Clear visual separation and color identity for the left and right decks.
- Large, glanceable track, time, BPM, pitch, loop, and sync state.
- Strong pressed, active, disabled, and focus states.
- A restrained hierarchy: performance-critical state is prominent; secondary
  configuration is behind panels or menus.
- Consistent spacing, typography, radii, colors, and icon sizing through
  centralized theme tokens.

## Touch Interaction Requirements

- Normal interactive targets should be at least 48 × 48 logical pixels.
- No required action may depend on hover, a right click, or a mouse wheel.
- Provide immediate visual feedback on press and a persistent indication for
  toggled state.
- Avoid adjacent destructive or mutually disruptive actions without adequate
  spacing or a deliberate gesture.
- Use drag gestures only where they are natural and provide a tap alternative
  where practical.
- Tooltips may help mouse users but must not contain information required to
  operate the skin.
- Keyboard and controller operation must continue to work through Mixxx's
  existing controls; the skin must not replace engine behavior with local-only
  state.

## MVP Layout And Features

Build a responsive landscape layout with these areas:

1. **Global/status bar**
   - Current time/status information available from existing Mixxx APIs.
   - Buttons for library access, preferences, recording, and a compact overflow
     menu where the APIs already support those actions.

2. **Two deck headers**
   - Track title and artist.
   - Cover art when available.
   - Elapsed/remaining time, BPM, pitch/rate, key, and loop size/state.
   - Clear loaded, playing, sync, and master state.

3. **Waveform area**
   - Scrolling waveforms for both active decks with play position and useful
     beat/cue markers.
   - Waveform overview or track-position indication where space permits.
   - At the minimum viewport, preserve the waveform and core transport controls
     before optional metadata or decoration.

4. **Deck controls**
   - Play/pause, cue, sync, and rate controls.
   - Loop enable and loop-size adjustment.
   - Beat jump controls.
   - At least eight hotcues per deck, reachable through a performance-pad area
     or banked layout.
   - Controls must bind to existing Mixxx `ControlObject`s through
     `Mixxx.ControlProxy` or an existing shared component.

5. **Mixer**
   - Per-deck gain, three-band EQ, channel volume, PFL/headphone cue, and VU
     meters.
   - Crossfader and main/headphone controls.
   - At small sizes, a deliberate compact or overlay presentation is acceptable
     as long as the active state remains obvious.

6. **Library**
   - A touch-accessible library panel or full-screen mode.
   - Browse/search, select a track, and load it into either deck without relying
     on drag and drop.
   - Reuse the best currently available QML/library integration. If the legacy
     QWidget bridge prevents an interaction from being genuinely touch usable,
     document the limitation rather than hiding it.

Sampler pads, four-deck operation, effects editing, custom waveform settings,
and portrait mode are follow-up work unless they are inexpensive consequences
of reused components.

## First Implementation Slice: Fixed Top Stack

Begin with the top three components from the performance view. They form one
fixed-height stack and must share exactly the same horizontal deck split:

```text
┌────────────────────── NavigationBar: 48 ────────────────────────┐
│ global navigation                         status and clock      │
├───────────────────────────┬─────────────────────────────────────┤
│       DeckStatus 1        │          DeckStatus 2               │  96
├───────────────────────────┼─────────────────────────────────────┤
│      DeckOverview 1       │         DeckOverview 2              │  64
└───────────────────────────┴─────────────────────────────────────┘
                              fixed total: 208 logical pixels
```

The performance-pad feedback visible below the track overviews in the visual
reference is not part of this slice and is not included in the 208-pixel total.
It will be designed as a separate component below this fixed top stack.

Define these heights as centralized theme/layout metrics rather than repeating
numeric literals:

```qml
readonly property int navigationBarHeight: 48
readonly property int deckStatusHeight: 96
readonly property int deckOverviewHeight: 64
```

This is the only layout profile: the heights are fixed in Qt logical pixels at
every supported viewport. They must not use `Layout.fillHeight`, ratios based
on window height, or implicit content height. Width remains responsive.

### Shared Center Split

The performance-view root owns one split coordinate:

```qml
readonly property real deckSplitX: width / 2
```

Pass that value to all three components. Each deck-aware row lays out its left
side from `0` to `deckSplitX` and its right side from `deckSplitX` to `width`.
Draw any center divider on top of that coordinate; do not give the divider its
own layout column or independently round the split in each component. Internal
padding must be symmetric so the visual boundary remains centered at odd as
well as even window widths.

The `NavigationBar` spans the full width rather than becoming two independent
toolbars, but its bottom deck-accent rule must change from the left-deck color
to the right-deck color at the same `deckSplitX` coordinate.

### `NavigationBar`

- Fixed height: 48 logical pixels, including its bottom accent rule.
- Left-aligned primary destinations: Browse and Touch FX or the closest
  functionality supported by current Mixxx APIs.
- Right-aligned global status/actions: recording state, other available status
  indicators, and clock. Unsupported status indicators should be omitted rather
  than mocked.
- Interactive targets must remain at least 48 logical pixels high and must show
  pressed and active state without relying on hover.
- Use self-contained original monochrome SVG icons in fixed 24 × 24 containers
  and short labels aligned on the same vertical centerline.
- Keep the center visually quiet; left and right clusters must not cross or
  displace the shared center split.

### `DeckStatus`

- Fixed row height: 96 logical pixels, split into two 48-pixel touch rows.
- Render two instances with equal responsive widths, one for each active deck.
- Use a two-level hierarchy inspired by the reference:
  - Upper line: deck number, track title/artist with elision, and compact loop
    state/size controls.
  - Lower line: sync/leader state, pitch/rate, pitch range, key, BPM, and
    elapsed or remaining time.
- Deck 1 and Deck 2 use distinct accent colors, continued from the navigation
  accent rule.
- Text-only metadata is not a touch target. Interactive items such as loop or
  sync controls must provide a 48 × 48 logical-pixel hit area even if their
  visible glyph is smaller.
- Tapping Sync triggers the momentary `beatsync` control and must not latch
  Sync. Holding it continuously for 2 seconds sets the explicit-leader
  `sync_mode` if the partner deck has no leader. If the partner is already
  Lead, the same hold enables `sync_enabled` on the held deck, preserving the
  partner as Lead and making the held deck a follower. Do not use
  `sync_leader` for the Lead hold because it currently requests only a soft
  leader that may be re-elected when a stopped follower joins Sync. Releasing
  after either hold action must not also trigger Beat Sync.
- Present Loop and Beat Jump as flat SVG-icon/value cells separated by vertical
  dividers. Do not distinguish these cells with boxed or contrasting
  backgrounds; use opacity for momentary press feedback.
- At 1024 pixels wide, preserve time, BPM, pitch, key, sync state, and an elided
  title before showing secondary metadata.

### `DeckOverview`

- Fixed row height: 64 logical pixels.
- Render one waveform overview in each deck half, aligned to the same center
  split as `DeckStatus`.
- Show the complete track, current position/playhead, played-versus-upcoming
  position, hotcue markers, and active loop range where the existing waveform
  API supports them.
- Keep cue and loop markers within the waveform overview itself. Do not add the
  two rows of performance-pad names, colors, or empty-slot indicators shown
  beneath the overview in the reference.
- Clip each deck overview to its own half so waveforms, labels, and gestures
  cannot paint across the center boundary.
- If seeking or another gesture is enabled, keep its handler inside that deck's
  half and provide clear pressed/drag feedback.

### First-Slice Acceptance Criteria

- `Ctrl+P` opens Preferences and `Ctrl+Q` quits the application, regardless of
  which TouchQML view currently has focus.
- The three components occupy exactly 208 logical pixels vertically at every
  target resolution.
- Their shared deck boundary remains at exactly half the available width while
  resizing through 1024 × 600, 1280 × 800, 1366 × 768, and 1920 × 1080.
- No title, waveform, marker, background, hit area, or divider crosses the deck
  boundary.
- Performance-pad feedback does not consume any space inside the 208-pixel top
  stack.
- Navigation actions and interactive deck-status controls have at least a
  48-pixel-high touch area and work without hover.
- Long track metadata elides without changing row height or moving the center
  split.
- Both overviews remain synchronized with their respective loaded tracks and
  update without QML binding or runtime warnings.

## Second Implementation Slice: Browse And Load

The Browse navigation button toggles the core-owned
`[Skin],show_maximized_library` control. While active, the content area below
the fixed 208-pixel top stack shows a touch-native all-tracks list backed by
Mixxx's QML library model.

- Track rows are 56 logical pixels high and support touch flicking.
- The browser aligns track/artist, rating, genre, comment, key, and duration
  under a persistent column header. Long text elides within its column.
- A tap selects a track. Starting a left drag selects that track as well, before
  the row begins revealing its actions.
- Dragging a row left reveals a 192-pixel action pane containing 96-pixel-wide
  `Load 1` and `Load 2` targets. The row snaps open after crossing its threshold,
  only one row remains open, and recycled rows reset to the closed position.
- Loading uses `Player.loadTrackFromLocationUrl()` and does not rely on drag and
  drop, hover, right click, or a double-click gesture.
- Double-tap is retained only as an optional shortcut for loading the selected
  track into Mixxx's next available deck.
- A 48-pixel-high text input filters title, artist, genre, comment, and key with
  a short debounce. It replaces both the global Search navigation button and
  the selected-track/load toolbar above the browser.
- This initial browser searches all library tracks. Source navigation,
  playlists, and crates remain follow-up work.

## Architecture Constraints

- Read and follow `QML_SKIN.md` before implementation.
- This is a configured QML skin. Test it with `mixxx --developer`; do not use
  `--new-ui` as an equivalent launch path.
- Preserve the restart boundary and profile-safety behavior for QML skin
  selection.
- Prefer the named `Mixxx` and `Mixxx.Controls` modules. Relative imports from
  `res/qml` are acceptable only when the skin is intentionally kept as an
  in-tree bundled skin; do not imply that they form a portable third-party API.
- Use `Mixxx.SkinControlCreator` only for persistent `[Skin]` presentation
  preferences. Use existing application/engine controls for Mixxx state.
- Account for QML engine destruction and recreation during auto-reload.
- Do not allocate, block, or add UI ownership to the real-time engine thread.
- Keep reusable visuals and behavior in focused components; avoid a monolithic
  `main.qml`.
- Keep `QML_SKIN.md` synchronized if this work changes documented behavior,
  architecture, limitations, or the list/state of bundled examples.

## Suggested Structure

```text
res/skins/TouchQML/
├── Controls/
├── Deck/
├── Library/
├── Mixer/
├── Theme/
├── Waveforms/
├── main.qml
├── skin.ini
└── skin_preview_<Scheme>.png
```

The exact component split may change as implementation reveals better
boundaries.

## Acceptance Criteria

- The skin is discovered in Preferences > Interface when Mixxx is run with
  `--developer` and QML support enabled.
- Selecting it and restarting with `--developer` opens its `main.qml`; the
  implementation does not rely on `--new-ui`.
- A user can browse for tracks, load both decks, play/cue/sync them, adjust the
  mix, set/trigger hotcues, loop, beat jump, and monitor useful deck state using
  touch input.
- The complete core workflow is usable at 1024 × 600, 1280 × 800,
  1366 × 768, and 1920 × 1080 logical pixels without essential controls being
  clipped or overlapping.
- Core targets meet the 48 × 48 logical-pixel goal and all required actions work
  without hover, right click, or a mouse wheel.
- QML files load without runtime errors or unresolved imports. Auto-reloading
  the skin does not leave duplicate skin controls or invalid bindings.
- The theme is centralized, referenced assets exist, and no Denon/Engine DJ
  proprietary assets or branding are included.
- No tests, C++ sources, or application code outside `res/skins/TouchQML` are
  created or modified for skin-only work.
- `QML_SKIN.md` accurately reflects the resulting state.

## Verification Boundary

Do not configure, build, or run Mixxx to verify TouchQML. Do not create or
modify tests, C++ sources, or other application code. Keep implementation
changes inside `res/skins/TouchQML` and documentation changes inside `TASK.md`
and `QML_SKIN.md`.

Use static inspection, `tools/qmlformat.py`, and `qmllint` for QML-only checks.
Manual runtime and touchscreen verification is left to the user. Record any
unavailable Mixxx QML API or library limitation discovered during static
implementation as a follow-up item in the final handoff.
