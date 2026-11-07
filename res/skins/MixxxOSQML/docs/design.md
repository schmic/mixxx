# TouchQML Design Brief

This document preserves the original design goals and acceptance criteria.

The original standalone TouchQML skin is now integrated at
`res/skins/MixxxOSQML/`, displayed as MixxxOSQML. The standalone repository
layout below describes the original design, not a required external checkout.

On the pinned Mixxx base, loaded-row markers use live player track URLs rather
than unavailable library load-state roles. Key text and colors use the existing
library key column rather than an unavailable `Track.numericKey` property.
The intended visible behavior and deck-color precedence remain unchanged.

## Goal

Create a new QML-based Mixxx skin optimized for landscape touchscreens in the
10–14 inch range. Its interaction model and visual direction should feel
familiar to users of Denon DJ's Engine DJ 5.x interface, while remaining an
original Mixxx design and using only Mixxx-owned or suitably licensed assets.

The skin should prioritize the controls needed during a performance, make them
easy to operate by touch, and remain readable on relatively small displays.

## Deliverable

Maintain the standalone experimental skin in this repository:

```text
mixxx-touchqml-skin/
```

It must have its own `skin.ini`, `main.qml`, components, theme, and preview
image(s). Do not turn `LateNightQML` into the touchscreen skin. Reuse suitable
Mixxx QML APIs and shared controls instead of duplicating their behavior.

The initial implementation is an MVP, but it must be coherent and usable for a
basic two-deck mixing workflow rather than being a static visual mock-up.

## Target Displays

Physical screen size alone is not available reliably to QML and varies with
display scaling, so design and test using Qt logical pixels.

The recommended effective resolution and primary design/test viewport is
**1280 × 800 logical pixels (16:10)**. This can be provided natively at 100%
scaling or by desktop scaling on a higher-resolution display. The current
hardware target is a 14-inch, 1920 × 1200, 60 Hz touchscreen; 150% scaling
provides the recommended 1280 × 800 logical workspace while retaining the
panel's native physical resolution and refresh rate.

Use the effective Qt viewport when validating layouts, not the panel's physical
pixel count. Avoid applying a second application scale on top of desktop
scaling. This recommendation does not force a window size, change display
settings automatically, or remove the other supported viewport sizes. Physical
touch usability at the selected scaling still requires hardware validation.

- Minimum supported landscape viewport: 1024 × 600 logical pixels.
- Primary design viewport: 1280 × 800 logical pixels.
- Also verify scaling at 1366 × 768 and 1920 × 1080.
- Restore the last normal window width and height across launches; let the
  platform manage window placement.
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
- Use Mixxx's bundled Ubuntu family for interface text.

## Touch Interaction Requirements

- Normal interactive targets should be at least 48 × 48 logical pixels.
- Compact DeckStatus actions at 36 pixels and waveform hotcue strips at 32
  pixels are explicit height exceptions. Waveform editing buttons are an
  explicit 36 × 36-pixel size exception.
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

In `PerformanceView`, the top three components form one fixed-height stack and
must share exactly the same horizontal deck split:

```text
┌────────────────────── NavigationBar: 48 ────────────────────────┐
│ global navigation                         status and clock      │
├───────────────────────────┬─────────────────────────────────────┤
│       DeckStatus 1        │          DeckStatus 2               │  72
├───────────────────────────┼─────────────────────────────────────┤
│      DeckOverview 1       │         DeckOverview 2              │  88
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
readonly property int deckStatusHeight: 72
readonly property int deckOverviewHeight: 88
```

This is the only layout profile: the heights are fixed in Qt logical pixels at
every supported viewport. They must not use `Layout.fillHeight`, ratios based
on window height, or implicit content height. Width remains responsive.

### Shared Center Split

`TouchMainWindow.qml` owns one split coordinate shared by the deck-aware rows:

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

Settings uses an icon-only cog button immediately left of the clock, with a
48 × 48 logical-pixel target, an accessible Settings name, and active-page
feedback.

- Fixed height: 48 logical pixels, including its bottom accent rule.
- Left-aligned primary destinations: Browse, Touch FX, and Samples using their
  existing Mixxx view controls.
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

- Fixed row height: 72 logical pixels, split into two 36-pixel rows.
- Render two instances with equal responsive widths, one for each active deck.
- Center 64 × 64 cover art or a neutral placeholder in a 72-pixel-wide slot
  spanning both rows.
- Use a two-level hierarchy inspired by the reference:
  - Upper line: a large elided title followed by unlabeled, right-aligned
    remaining-time, key, and BPM values.
  - Lower line: an elided artist followed by a combined
    `pitch icon / current pitch / ± / range` cell, then Loop, Beat Jump, and
    Sync/Lead cells; use muted text for inactive values.
- Text-only metadata is not a touch target. Compact Loop, Beat Jump, and Sync
  controls use the full 36-pixel row height.
- Tapping Sync triggers the momentary `beatsync` control and must not latch
  Sync. Holding it continuously for 2 seconds toggles `sync_leader` if the
  partner deck has no leader. If the partner is already Lead, the same hold
  enables `sync_enabled` on the held deck, preserving the partner as Lead and
  making the held deck a follower. Releasing after either hold action must not
  also trigger Beat Sync.
- Scale the combined Pitch/Range cell from 88 × 36 to 96 × 36, and Loop, Beat
  Jump, and Sync/Lead from 56 × 36 to 64 × 36. Use 18-pixel icons and open
  spacing instead of cell borders or full-height dividers.
- Present Loop and Beat Jump as flat SVG-icon/value cells. Do not distinguish
  them with boxed or contrasting backgrounds; use opacity for momentary press
  feedback.
- At 1024 pixels wide, preserve time, BPM, pitch, key, sync state, and an elided
  title before showing secondary metadata.

### `DeckOverview`

- Fixed row height: 88 logical pixels.
- Render one waveform overview in each deck half, aligned to the same center
  split as `DeckStatus`.
- Show only the upper/left-channel waveform half, scaled to the overview height.
- Show the complete track, current position/playhead, played-versus-upcoming
  position, hotcue markers, and active loop range where the existing waveform
  API supports them.
- Add a small downward-pointing triangle at the top of each active hotcue marker,
  using the cue's stored color and aligned with its vertical marker line.
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
- In `PerformanceView`, the three top components occupy exactly 208 logical
  pixels vertically at every target resolution.
- Their shared deck boundary remains at exactly half the available width while
  resizing through 1024 × 600, 1280 × 800, 1366 × 768, and 1920 × 1080.
- No title, waveform, marker, background, hit area, or divider crosses the deck
  boundary.
- Performance-pad feedback does not consume any space inside the 208-pixel top
  stack.
- Navigation actions retain 48-pixel-high targets; compact deck-status controls
  use 36-pixel-high targets and work without hover.
- Long track metadata elides without changing row height or moving the center
  split.
- Both overviews remain synchronized with their respective loaded tracks and
  update without QML binding or runtime warnings.

## Second Implementation Slice: Browse And Load

The Browse navigation button toggles the core-owned
`[Skin],show_maximized_library` control. While active, the content area below
the persistent 120-pixel navigation/status header replaces `PerformanceView`
with a touch-native all-tracks list backed by Mixxx's QML library model. Pages
remain instantiated in a `StackLayout`, so reopening Browse restores its
query, sort, selected row, and scroll position without rebuilding the model.

- Track rows are 56 logical pixels high and support touch flicking.
- The browser aligns Title/Artist, Genre, Comment, BPM, Rating, Key, Time, Last
  under a persistent 48-pixel sortable column header. Tapping a header sorts
  ascending; tapping it again reverses order. Initial order is Genre ascending.
  Holding Genre for 500 milliseconds opens a single-select popup of unique
  non-empty genre values plus All Genres. Its exact case-insensitive filter
  combines with text search. The Genre header uses the Sync Lead color while a
  filter is active without replacing its label. Long text elides within its
  column.
- Holding Comment for 500 milliseconds opens a single-select popup of unique
  whitespace-separated comment values plus All Comments. Split on spaces,
  tabs, and line breaks; ignore empty values, retain punctuation, deduplicate
  case-insensitively, and sort choices alphabetically. Show values from the
  entire library, independent of active filters. Match whole values
  case-insensitively and combine with Genre and text search. For example,
  `warm` matches `warm vocal` but not `warmup`. Highlight the Comment header
  using the Sync Lead color while active without changing its label. All
  Comments clears only the comment filter. The popup shares Genre's touch
  sizing, modal input handling, and dismissal behavior. Preserve the filter
  while Browse is hidden and during track metadata refreshes; changing it
  resets selection and scroll like Genre.
- BPM follows Mixxx's library tempo precision and locale. Last displays the
  localized last-played date, with `--` for a track that has never been played.
  Genre, Comment, Rating, and Last widths grow within bounded ranges from
  1024 to 1920 pixels; all columns remain visible and Title/Artist fills the
  remaining space. Headers and rows use identical widths and gaps.
- Honor Mixxx's Grey out played tracks preference using its library-model
  foreground color for title, artist, and all metadata, including Key. Use
  the actual played flag, independent of Last Played or loaded-deck indicators.
  Restore normal theme and key colors when the preference is disabled or the
  played flag is cleared. Selection, artwork, and load indicators retain
  their usual appearance. Applied preference changes appear within one second
  while Browse is visible and immediately when it is reopened.
- A tap selects a track. Starting a left drag selects that track as well, before
  the row begins revealing its actions.
- Reserve the three-pixel left strip's blue, green, and orange accents for
  tracks loaded in Deck 1, Deck 2, and Preview Deck 1 respectively. Other rows
  use the selected-row background color for their strip; selection alone has
  no blue accent and its strip blends into the selected background.
  Markers follow loads, track replacements, and unloads while Browse is open
  or hidden, including on reused delegates. Played-track greying affects text
  only and must not remove or dim the loaded-track strip. Use the model's
  `loaded_deck_mask` bits for Deck 1/2 and `preview_deck_loaded` for preview
  decks. Load-state role updates must not reset filtering, selection, or scroll.
- Dragging a row left reveals a 192-pixel action pane containing 96-pixel-wide
  `Load 1` and `Load 2` targets. The row snaps open after crossing its threshold,
  only one row remains open, and recycled rows reset to the closed position.
- Loading uses `Player.loadTrackFromLocationUrl()` and does not rely on drag and
  drop, hover, right click, or a double-click gesture.
- Standard controller and touch load actions only load the selected track; view
  transitions remain the caller's responsibility. Library movement controls
  traverse filtered rows, choosing the first or last row when no row is selected
  and wrapping at list boundaries thereafter. Up/Down keys use the same movement
  behavior; Enter/Return loads into the next available deck. URL, list index,
  current item, and visible position remain synchronized. Loading and sorting
  honor the all-tracks model's advertised capabilities. Browse state remains
  available for the next opening.
- Loading a track into a deck or preview must preserve Browse selection and
  scroll position during metadata and model refreshes. Reconcile selection by
  URL after the filtered list settles, including temporary empty/rebuilt library
  lists and incremental row insertions, removals, or moves. A metadata-driven
  row move preserves the scroll position without automatically revealing the
  selected row. If the library removes the selected track, select the nearest
  remaining row and clamp the previous scroll position to the new list bounds.
  Manual flicking, wheel scrolling, and scrollbar dragging take precedence over
  pending refresh restoration. Scrolling does not clear selection or pull an
  offscreen selected row back into view. Keyboard/controller selection movement
  and explicit sorting still reveal the selected row.
  Clear selection only when the final result is empty. Explicit
  search/genre/comment changes may reset to the beginning; sorting keeps the selected
  track visible at its new position.
- Double-tap is retained only as an optional shortcut for loading the selected
  track into Mixxx's next available deck.
- A 48-pixel-high text input filters title, artist, genre, comment, and key with
  a short debounce. Filtering and genre/comment collection use cached metadata, so
  scanning the library does not create full Track objects for all its rows. It
  replaces both the global Search navigation button and the selected-track/load
  toolbar above the browser.
- Key labels follow the current Key Notation preference immediately, including
  while Browse is hidden. Key searches use that same notation and refresh when
  it changes, retaining a still-matching selection without resetting scrolling
  to the beginning.
- A 320-pixel compact preview control sits left of search when a preview deck is
  available. It contains a 48-pixel play/pause target and a seekable full-track
  RGB waveform for Preview Deck 1. Holding a track row for 500 milliseconds
  selects that row, loads it into Preview Deck 1, and starts playback when the
  active model advertises preview-deck loading support.
- Browse shows only All Tracks, using the upstream `LibrarySourceTree.allTracks()`
  API with the browser's column definitions. There is no source picker or
  dependency on custom crate, playlist, history, or live-sidebar APIs.
  Deck/preview loading must not reset the browser's filters, sort, or position.
- Genre and comment popups block clicks, taps, and scroll input from reaching the
  UI underneath, including their headers and empty areas. Outside clicks/taps
  dismiss on release; the dismissing gesture cannot activate a lower control.
  Popup lists remain scrollable, and Escape still closes them.

## Third Implementation Slice: Performance Waveforms

`PerformanceView` begins below the persistent DeckStatus row. It owns the
88-pixel `DeckOverviewRow`, two stacked scrolling waveform rows, and the
remaining space reserved for later transport, pad, and mixer slices.

- Deck 1 is the upper scrolling waveform with a 3-pixel blue left accent.
- Deck 2 is the lower scrolling waveform with a 3-pixel green left accent.
- The two waveforms divide all vertical space remaining after the overview,
  hotcue strips, strip gaps, and effect row equally. No maximum waveform height
  leaves unused page space. At target viewports each waveform is 134, 234, 218,
  and 374 pixels high respectively.
- Use a portable local composition of `Mixxx.Controls.WaveformDisplay` with RGB
  signal, beat grid, playhead at one third of the width, cue/hotcue marks,
  loop/intro/outro ranges, preroll, and end-of-track warning renderers. Render
  beatgrid lines in the dark border grey so they blend into the background.
- Use native default hotcue marks so cue numbers, saved labels, indexed stored
  colors, and saved-loop ranges/endpoints stay synchronized with Mixxx. Show the
  main cue and active-loop boundaries explicitly. Show both intro and outro
  endpoints and ranges only while `[Skin],show_intro_outro_cues` is active.
  Keep native hotcues and explicit CUE/loop/intro/outro markers in separate
  renderer sets so a hotcue at the same position cannot remove a fixed marker.
  Draw fixed markers underneath hotcues so hotcue colors and labels stay readable.
  Render the playhead and next-hotcue readout only once.
- Markers outside the current scrolling-waveform time window are not represented
  by edge indicators.
- Bind zoom to each deck's `waveform_zoom` control and honor Mixxx's synchronized
  waveform-zoom preference. Initialize the owning zoom control from Mixxx's
  default waveform zoom preference and apply subsequent preference changes;
  secondary synchronized waveforms share Deck 1's control without resetting it.
- Main waveforms are display-only in this slice. Do not copy the upstream mouse
  scratching, right-button bending, or wheel-zoom handlers into the touch UI.
- Keep pages instantiated to preserve Browse state and avoid model reloads.
  Hidden pages do not contribute visible scene-graph nodes, though their QML
  objects remain allocated.

### Waveform Editing

Reserve 74 logical pixels at the right of each waveform row for that deck's
editing panel. Keep the waveform-row heights and full-width hotcue strips
unchanged. Match the panel background to an empty hotcue button. Both panels
can be shown or hidden together; hiding them restores full-width scrolling
waveforms. The sidebar starts hidden on startup and QML reload; visibility
is not persisted across restarts.
A 36 × 36-pixel icon toggle stays at the right edge, vertically centered
between the two waveforms. Reserve 18 pixels at the adjacent end of each
panel's button area so the toggle cannot cover editing controls. The toggle
remains visible with the panels hidden and shows their active state.
It becomes enabled after the skin-owned visibility control is ready, so
starting hidden still allows the first tap to open the panels.
Overlay a 2-pixel grey border on the panel's left edge using the
same border color and thickness as an empty hotcue's bottom stripe.
Center three two-column rows of icon-only buttons without a heading, with
2-pixel gaps, original 24-pixel SVG icons, accessible action names, and
fixed 36 × 36-pixel buttons, a requested compact-target exception:

All editing buttons and the visibility toggle have no border.
Idle backgrounds match the sidebar, regardless of loaded-track or BPM-lock
state. Only a press changes a button's background; disabled icons still dim.

- Grid/left-arrow and grid/right-arrow icons shift the beatgrid earlier/later using Mixxx's standard
  small-step controls, matching LateNightQML.
- A wider grid with a dashed midpoint and right arrow shifts the beatgrid later by half a beat, matching LateNightQML's
  secondary action. The engine supports this action for constant-BPM tracks.
- A flag/plus icon sets or replaces the intro start at the current playhead position,
  honoring Mixxx quantization and intro/outro ordering constraints.
- A magnet icon toggles the owning deck's standard `quantize` control. It uses
  the deck's blue or green accent while enabled and follows controller changes.
  Quantize remains available when BPM is locked.
- An open/closed padlock toggles the track's standard BPM lock control. Its
  button has no border; the open lock is neutral and the closed lock uses
  Deck 1's blue or Deck 2's green accent. Small shifts occupy
  the first row; quantize and half-beat shift occupy
  the second; BPM lock and Set Intro occupy the third.

The grid needs 112 pixels of height and fits at every target viewport,
including 1024 × 600. It scrolls vertically if the waveform row is shorter.

Disable grid shifts and show the closed padlock when the track's BPM is locked.
Set Intro remains available for locked tracks. Disable every action while the
deck is empty. Actions use a single tap; grid shifts and Set Intro immediately
reset their core trigger controls, while BPM lock and quantize retain their states.
Keyboard activation and visible focus feedback are available.
The deck-coloured icon indicates the control's active state only. Keyboard
focus uses a small neutral marker, so switching quantize off shows a neutral
icon even while its button retains focus.

## Fourth Implementation Slice: Hotcue Strips

Give each waveform row its own full-width 32-pixel-high hotcue strip. Deck 1's strip
sits 2 pixels above its waveform, while Deck 2's strip sits 2 pixels below its
waveform.

- Eight equal responsive columns span the full viewport width with 2-pixel gaps,
  no outer padding, and no outlines. Button width is `(viewportWidth - 14) / 8`.
- Buttons use the dark-gray control background and pressed background so the
  strips remain distinct from the performance-page background.
- Each button shows one centered label: `LOOP N` for a saved loop or `CUE N` for
  any other or empty slot. Custom Mixxx cue labels are not shown.
- Show each set cue's stored Mixxx color as a 2-pixel bottom stripe. This keeps
  the strip, overview pointers, and scrolling-waveform marks on the same Mixxx
  palette. Empty slots use a neutral gray stripe. Rendering must never write
  `hotcue_N_color`.
- Hold `hotcue_N_activate` at 1 while pressed and restore it to 0 on release,
  disable, page hiding, or destruction. This preserves Mixxx's set/activate
  behavior and prevents a stuck control if the active page changes during a
  touch.
- Do not expose a clear gesture in this slice. Clearing remains available from
  keyboard or controller mappings without adding an accidental destructive
  touch action.
- Disable all hotcue buttons while their deck has no loaded track.

## Fifth Implementation Slice: Deck Effects

Add an 8-pixel gap below Deck 2's hotcue strip, then a row split at the shared
deck center. Its height is fixed at 48 pixels at every viewport, matching the
hotcue row. Each half has four equal contiguous buttons without gaps or outer
padding. Vertical edges are shared neutral gray one-pixel seams; active buttons
highlight their top/bottom edges with the deck accent.

- Show standard `FX1`, `FX2`, and `FX3` buttons followed by `Quick FX`.
- Bind Deck 1 standard buttons to EffectUnit 1 and Deck 2 standard buttons to
  EffectUnit 2. Use slots 1 through 3 of each unit.
- Preserve Mixxx's existing effect routing. Read each unit's
  `group_[ChannelN]_enable` assignment and `enabled` state, but do not alter
  either from this row. Disable standard buttons and show `Unrouted` or
  `Unit Off` when their configured unit cannot process that deck.
- Tapping a routed standard button toggles its slot `enabled` control. Tapping
  `Quick FX` toggles the deck's `[QuickEffectRack1_[ChannelN]],enabled` chain.
- Holding a button for 250 milliseconds opens a centered modal selector
  containing only 48-pixel effect rows. Standard selectors use
  `Mixxx.EffectsManager.visibleEffectsModel` and set
  `EffectSlotProxy.effectId`; Quick FX uses `quickChainPresetModel` and sets
  `loaded_chain_preset`.
- Selecting an effect or Quick Effect preset must preserve the slot or chain
  enabled state. Selection changes only `effectId` or `loaded_chain_preset`.
- Keep current effect names visible on the buttons and show deck-colored active
  feedback. Selection and toggling must require no hover, right click, or mouse
  wheel.

## Settings

Settings opens below the persistent header through core-owned
`[Skin],show_settings`. The top row has right-aligned Preferences and Quit
buttons with 160 × 48-pixel targets and a 16-pixel gap. Preferences opens the
standard Mixxx preferences dialog; Quit requests normal application shutdown,
matching the existing keyboard shortcuts.
A row below exposes the shared Mixxx "Start in fullscreen"
preference with a 48-pixel On/Off target and a note that it applies the next
time Mixxx starts. Startup follows this preference once core initialization
is complete; changing it does not change the current window state.
The page scrolls vertically at smaller heights to keep every control reachable.
Waveform zoom sliders apply immediately through the standard deck controls and
follow changes from the controller. Moving right zooms in; moving left zooms
out. Synchronized zoom presents one slider for both decks; independent zoom
presents separate Deck 1 and Deck 2 sliders. Each slider has a 48-pixel-high
touch target and a square handle with its deck accent.
It also presents one touch panel per deck with 48-pixel
targets for `vinylcontrol_enabled`, `vinylcontrol_mode` (`ABS`, `REL`, `CONST`),
and `vinylcontrol_cueing` (`OFF`, `ONE`, `HOT`). These controls bind directly to
existing deck ControlObjects, keeping controller and engine behavior shared.

## Architecture Constraints

- Read and follow `docs/architecture.md` before implementation.
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
- Keep `docs/architecture.md` synchronized if this work changes documented behavior,
  architecture, limitations, or the list/state of bundled examples.

## Suggested Structure

```text
mixxx-touchqml-skin/
├── Controls/
├── Deck/
├── Library/
├── Mixer/
├── Performance/
├── Theme/
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
- Core targets meet the 48 × 48 logical-pixel goal except the documented compact
  DeckStatus, hotcue-strip, and waveform-editing exceptions. All required actions work
  without hover, right click, or a mouse wheel.
- QML files load without runtime errors or unresolved imports. Auto-reloading
  the skin does not leave duplicate skin controls or invalid bindings.
- The theme is centralized, referenced assets exist, and no Denon/Engine DJ
  proprietary assets or branding are included.
- No tests, C++ sources, or application code outside this repository are
  created or modified for skin-only work.
- `docs/architecture.md` accurately reflects the resulting state.

## Verification Boundary

Do not configure, build, or run Mixxx to verify TouchQML. Do not create or
modify tests, C++ sources, or other application code. Keep implementation
changes inside this repository and documentation changes inside `docs/`.

Use static inspection, `tools/qmlformat.py`, and `qmllint` for QML-only checks.
Manual runtime and touchscreen verification is left to the user. Record any
unavailable Mixxx QML API or library limitation discovered during static
implementation as a follow-up item in the final handoff.
