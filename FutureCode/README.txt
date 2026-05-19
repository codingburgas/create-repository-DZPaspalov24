Optional fonts go in this folder.

The app's UI is in BULGARIAN by default, so the font MUST contain Cyrillic
glyphs. DejaVu Sans is the recommended choice and is freely redistributable.

The app will look for, in order:

  1. resources/DejaVuSans.ttf       (regular weight)
  2. resources/DejaVuSans-Bold.ttf  (headings, emphasis)

If those files are not present, theme.cpp will fall back to common system
locations:

  /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf       (Debian/Ubuntu)
  /usr/share/fonts/dejavu/DejaVuSans.ttf                (Fedora/RHEL/openSUSE)
  /usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf     (some RPM-based distros)
  /Library/Fonts/DejaVuSans.ttf                         (macOS, if installed)
  /Library/Fonts/Arial Unicode.ttf                      (macOS fallback)
  C:/Windows/Fonts/segoeui.ttf                          (Windows - Segoe UI has Cyrillic)
  C:/Windows/Fonts/arial.ttf                            (Windows - Arial has Cyrillic)

If even those fail, raylib's built-in default font is used. WARNING: The
default font does NOT contain Cyrillic glyphs, so Bulgarian text will appear
as boxes/question marks. The app remains functional but you should drop a
Unicode-capable font in here for the proper experience.

To get DejaVu Sans:

  Linux:    apt install fonts-dejavu       (or your distro's equivalent)
  macOS:    https://dejavu-fonts.github.io/  -> Download -> drag .ttf in here
  Windows:  https://dejavu-fonts.github.io/  -> Download -> drag .ttf in here

The font files are licensed under the Bitstream Vera + Public Domain
permissive licence.
