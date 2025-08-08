# Font Editing Guide

The raspberry-display project uses a user-friendly font editing system that allows you to modify character definitions using ASCII art instead of raw hex values.

## Font Architecture

- **Source**: `tools/font_definitions.txt` - Human-readable ASCII art font definitions (version controlled)
- **Generated**: `src/font_generated.h` - C header file generated from definitions (ignored by git)
- **Build**: Font is automatically generated during build process

## Quick Start

### Edit Font Definitions
Open `tools/font_definitions.txt` and modify the ASCII art patterns:
- Use `#` for pixels that should be lit
- Use `.` for pixels that should be dark
- Each character has 8 rows and up to 7 columns
- The `WIDTH:` field specifies how many columns are actually used

### Build with New Font
```bash
make clean && make
```
The font is automatically generated from `tools/font_definitions.txt` during the build process.

### Manual Font Generation (Optional)
```bash
make font-generate
```
This manually creates `src/font_generated.h` from the font definitions.

## Font Definition Format

Each character in `tools/font_definitions.txt` follows this format:

```
CHAR: A
WIDTH: 5
........
.####...
#....#..
#....#..
######..
#....#..
#....#..
........
```

- **CHAR:** The character being defined (special names: SPACE, TAB, NEWLINE, or hex like 0xe6)
- **WIDTH:** Number of columns used (1-7)
- **ASCII Art:** 8 rows of 8 characters each, using `#` for lit pixels and `.` for dark pixels

## Example: Editing the Letter 'A'

**Original:**
```
CHAR: A
WIDTH: 5
........
.####...
#....#..
#....#..
######..
#....#..
#....#..
........
```

**Modified (bolder):**
```
CHAR: A
WIDTH: 5
........
######..
##..##..
##..##..
######..
##..##..
##..##..
........
```

## Special Characters

- **SPACE**: Regular space character
- **TAB**: Tab character (rarely used in display)
- **NEWLINE**: Newline character (rarely used in display)
- **0xe6, 0xf8, etc.**: Extended ASCII characters (æ, ø, å, Æ, Ø, Å)

## Build System Integration

The font system integrates with the build system:

- **Manual workflow**: `make font-extract` → edit → `make font-generate`
- **Automatic regeneration**: If you modify `tools/font_definitions.txt`, the build system will automatically regenerate `src/font_generated.h` when needed
- **Dependency tracking**: Changes to font definitions or the generator script trigger automatic regeneration

## Tips for Font Editing

1. **Test frequently**: Generate and test your changes often to see how they look on the display
2. **Consistent width**: Keep character widths consistent within character groups (letters, numbers, symbols)
3. **Spacing**: Remember that characters are rendered adjacent to each other, so consider spacing
4. **Readability**: Ensure characters remain distinguishable at the display resolution
5. **Backup**: Keep a backup of your original font definitions before making major changes

## Technical Details

- **Font array**: 101 characters × 8 bytes each
- **Character lookup**: String mapping character codes to font array indices
- **Bit encoding**: Each byte represents one row, with bits 0-6 representing columns (bit 7 unused)
- **Width encoding**: First byte of each character entry specifies the display width (1-7 pixels)

## Troubleshooting

**Font not updating after changes:**
```bash
make clean
make font-generate
cp src/font_generated.h src/font.h
make
```

**Characters appear corrupted:**
- Check that ASCII art uses only `#` and `.` characters
- Ensure each row has exactly 8 characters
- Verify WIDTH matches the actual character width needed

**Build errors:**
- Ensure Python 3 is installed
- Check that `tools/font_definitions.txt` exists and is properly formatted
- Verify no syntax errors in the ASCII art patterns
