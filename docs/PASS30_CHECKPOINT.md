# Pass 30 checkpoint — Apple chrome and footer cleanup

Pass 30 addresses issues captured on the first real macOS/iPad visual review after the dual-platform gate became usable.

## Page header

- The Front page is branded **Alesis ION/Micron Editor** instead of the generic `Front` title.
- Header title and metadata use separate layout regions so they cannot paint through each other.
- The metadata banner is ASCII-only (`shared JSON state / desktop + iPad touch surface`) to eliminate the observed mojibake bullet.

## Fixed global control bar

- MIDI IN, BANK, PROGRAM, MIDI OUT, CH, and PANEL headings now draw in a dedicated 12 px strip above their selectors.
- Heading rectangles no longer reuse the selector height, eliminating the label/value overlap seen on both macOS and iPadOS.

## Physical iPad full-screen presentation

- JUCE `STATUS_BAR_HIDDEN` and `REQUIRES_FULL_SCREEN` remain enabled.
- The generated plist now disables view-controller-based status-bar appearance so iPadOS honors the global hidden-status-bar policy.
- The iOS window also enters JUCE kiosk mode with menus/bars disallowed after becoming visible, which makes the native peer explicitly request a hidden status bar. The physical-device build gate verifies all three packaged plist keys before installation.

The application remains a standalone app. No simulator or plugin targets are introduced.
