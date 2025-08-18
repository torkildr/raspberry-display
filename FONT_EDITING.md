# Font Editing Guide

The raspberry-display project uses a user-friendly font editing system that allows you to modify character definitions using ASCII art instead of raw hex values.

## Font Architecture

- **Source**: `tools/font_definitions.txt` - Human-readable ASCII art font definitions (version controlled)
- **Generated**: `src/font_generated.hpp` - C++ header file generated from definitions (ignored by git)
- **Build**: Font is automatically generated during build process

## Quick Start

### Edit Font Definitions
Open `tools/font_definitions.txt` and modify the ASCII art patterns:
- Use `#` for pixels that should be lit
- Use `.` for pixels that should be dark
- Each character has 8 rows and an arbitrary number of columns

### Build with New Font
```bash
make clean && make
```
The font is automatically generated from `tools/font_definitions.txt` during the build process.

### Manual Font Generation (Optional)
```bash
make font-generate
```
This manually creates `src/font_generated.hpp` from the font definitions.

## Font Definition Format

Each character in `tools/font_definitions.txt` follows this format:

```
CHAR: A
.
.####
#....#
#....#
######
#....#
#....#
.
```

- **CHAR:** The character being defined (special names: SPACE, TAB, NEWLINE, or hex like 0xe6)
- **WIDTH:** Number of columns used (1-7)
- **ASCII Art:** 8 rows of 8 characters each, using `#` for lit pixels and `.` for dark pixels

## Special Characters

The font system is encoded in iso8859-1. Only characters in this range can be used.

- **SPACE**: Regular space character
- **TAB**: Tab character (rarely used in display)
- **NEWLINE**: Newline character (rarely used in display)
- **0xe6, 0xf8, etc.**: Extended ASCII characters (æ, ø, å, Æ, Ø, Å)

## Build System Integration

The font system integrates with the build system:

- **Automatic regeneration**: If you modify `tools/font_definitions.txt`, the build system will automatically regenerate `src/font_generated.hpp` when needed
- **Dependency tracking**: Changes to font definitions or the generator script trigger automatic regeneration
