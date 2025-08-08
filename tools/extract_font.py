#!/usr/bin/env python3
"""
Extract existing font from font.h and convert to ASCII art format
"""

import re
import sys

def hex_to_ascii(width, hex_bytes):
    """Convert hex bytes to ASCII art representation"""
    ascii_lines = [''] * 8
    
    # Each byte represents a column, each bit represents a row
    for col in range(width):
        if col < len(hex_bytes):
            byte_val = hex_bytes[col]
            for row in range(8):
                # Bit 0 is top row (row 0), bit 7 is bottom row (row 7)
                # This matches the display logic: (1 << row)
                if byte_val & (1 << row):
                    ascii_lines[row] += '#'
                else:
                    ascii_lines[row] += '.'
        else:
            for row in range(8):
                ascii_lines[row] += '.'
    
    # Pad each line to 8 characters for consistency
    for i in range(8):
        ascii_lines[i] = ascii_lines[i].ljust(8, '.')
    
    return ascii_lines

def parse_font_h(filename):
    """Parse the existing font.h file"""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Extract character lookup string - handle escape sequences properly
    char_match = re.search(r'char charLookup\[\] = "((?:[^"\\]|\\.)*)"', content)
    if not char_match:
        raise ValueError("Could not find charLookup array")
    
    char_lookup_raw = char_match.group(1)
    print(f"Raw character lookup: {repr(char_lookup_raw)}")
    
    # Decode escape sequences properly
    char_lookup = ""
    i = 0
    while i < len(char_lookup_raw):
        # Get character name and actual character
        if i < len(char_lookup):
            char = char_lookup[i]
            if char == ' ':
                char_name = 'SPACE'
                actual_char = ' '
            elif char == '\n':
                char_name = 'NEWLINE'
                actual_char = '\n'
            elif char == '\t':
                char_name = 'TAB'
                actual_char = '\t'
            else:
                char_name = char
                actual_char = char
        else:
            char_name = f'CHAR_{i}'
            actual_char = f'CHAR_{i}'
        if char_lookup_raw[i] == '\\' and i + 1 < len(char_lookup_raw):
            next_char = char_lookup_raw[i + 1]
            if next_char == '"':
                char_lookup += '"'
                i += 2
            elif next_char == '\\':
                char_lookup += '\\'
                i += 2
            elif next_char == 'x' and i + 3 < len(char_lookup_raw):
                # Handle hex escape sequences like \xe6
                hex_val = char_lookup_raw[i+2:i+4]
                try:
                    # Store the actual character for the lookup string
                    actual_char = chr(int(hex_val, 16))
                    char_lookup += actual_char
                    i += 4
                except ValueError:
                    char_lookup += char_lookup_raw[i]
                    i += 1
            else:
                char_lookup += char_lookup_raw[i]
                i += 1
        else:
            char_lookup += char_lookup_raw[i]
            i += 1
    
    print(f"Character lookup string length: {len(char_lookup)}")
    
    # Extract font array - handle multiline array definition
    font_match = re.search(r'unsigned char font_variable\[.*?\]\[.*?\] = \{(.*?)\};', content, re.DOTALL)
    if not font_match:
        raise ValueError("Could not find font_variable array")
    
    font_data = font_match.group(1)
    
    # Parse individual character entries - handle hex values with 0x prefix
    # Look for entries like {1,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
    char_entries = re.findall(r'\{([0-9,x\sA-Fa-f]+)\}', font_data)
    print(f"Found {len(char_entries)} character entries")
    
    characters = []
    for i, entry in enumerate(char_entries):
        if i >= len(char_lookup):
            break
            
        # Parse hex values, handling both 0x and plain hex formats
        hex_strs = [x.strip() for x in entry.split(',')]
        hex_values = []
        for hex_str in hex_strs:
            if not hex_str:  # Skip empty strings
                continue
            try:
                if hex_str.startswith('0x'):
                    hex_values.append(int(hex_str, 16))
                else:
                    hex_values.append(int(hex_str, 16))
            except ValueError as e:
                print(f"Error parsing hex value '{hex_str}': {e}")
                raise
        width = hex_values[0]
        hex_bytes = hex_values[1:8]  # Take up to 7 bytes
        
        char = char_lookup[i]
        ascii_art = hex_to_ascii(width, hex_bytes)
        
        characters.append({
            'char': char,
            'width': width,
            'ascii_art': ascii_art
        })
    
    return characters

def generate_font_txt(characters, output_file):
    """Generate font definition text file"""
    with open(output_file, 'w') as f:
        for char in characters:
            # Handle special characters for display
            if char['char'] == ' ':
                display_char = 'SPACE'
            elif char['char'] == '\t':
                display_char = 'TAB'
            elif char['char'] == '\n':
                display_char = 'NEWLINE'
            elif ord(char['char']) < 32 or ord(char['char']) > 126:
                display_char = f'0x{ord(char["char"]):02x}'
            else:
                display_char = char['char']
            
            f.write(f'CHAR: {display_char}\n')
            f.write(f'WIDTH: {char["width"]}\n')
            for line in char['ascii_art']:
                f.write(f'{line}\n')
            f.write('\n')

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 extract_font.py font.h output.txt")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    try:
        characters = parse_font_h(input_file)
        generate_font_txt(characters, output_file)
        print(f"Extracted {len(characters)} characters to {output_file}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
