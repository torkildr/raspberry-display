#!/usr/bin/env python3
"""
Font Generator for Raspberry Display
Converts ASCII art font definitions to C header file

Usage: python3 font_generator.py input.txt output.h

Input format:
CHAR: A
WIDTH: 4
........
.####...
#....#..
#....#..
######..
#....#..
#....#..
........

CHAR: B  
WIDTH: 4
........
#####...
#....#..
#####...
#####...
#....#..
#####...
........
"""

import sys
import re

def ascii_to_hex(ascii_lines):
    """Convert ASCII art to hex bytes"""
    hex_bytes = [0] * 7  # 7 bytes for font data
    
    for col in range(min(7, len(ascii_lines[0]))):
        byte_val = 0
        for row in range(8):
            if row < len(ascii_lines) and col < len(ascii_lines[row]):
                if ascii_lines[row][col] == '#':
                    # Bit 0 is top row (row 0), bit 7 is bottom row (row 7)
                    # This matches the display logic: (1 << row)
                    byte_val |= (1 << row)
        hex_bytes[col] = byte_val
    
    return hex_bytes

def parse_font_file(filename):
    """Parse font definition file"""
    with open(filename, 'r') as f:
        content = f.read()
    
    characters = []
    char_blocks = re.split(r'\n(?=CHAR:)', content.strip())
    
    for block in char_blocks:
        if not block.strip():
            continue
            
        lines = block.strip().split('\n')
        char_info = {}
        ascii_lines = []
        
        for line in lines:
            if line.startswith('CHAR:'):
                char_name = line.split(':', 1)[1].strip()
                # Convert display names back to actual characters
                if char_name == 'SPACE':
                    char_info['char'] = ' '
                elif char_name == 'NEWLINE':
                    char_info['char'] = '\n'
                elif char_name == 'TAB':
                    char_info['char'] = '\t'
                elif char_name.startswith('0x'):
                    # Handle hex display names like '0xe6'
                    hex_val = char_name[2:]
                    char_info['char'] = chr(int(hex_val, 16))
                else:
                    char_info['char'] = char_name
            elif line.startswith('WIDTH:'):
                char_info['width'] = int(line.split(':', 1)[1].strip())
            elif line and not line.startswith('CHAR:') and not line.startswith('WIDTH:'):
                ascii_lines.append(line)
        
        if len(ascii_lines) == 8:
            char_info['hex_bytes'] = ascii_to_hex(ascii_lines)
            char_info['ascii_art'] = ascii_lines
            characters.append(char_info)
    
    return characters

def generate_header(characters, output_file):
    """Generate C header file"""
    with open(output_file, 'w') as f:
        f.write('#ifndef font_h\n')
        f.write('#define font_h\n\n')
        f.write(f'#define FONT_COUNT {len(characters)}\n\n')
        
        # Generate character lookup string with proper escaping
        f.write('char charLookup[] = "')
        for char in characters:
            if char['char'] == ' ':
                f.write(' ')
            elif char['char'] == '"':
                f.write('\\"')
            elif char['char'] == '\\':
                f.write('\\\\')
            elif len(char['char']) == 1 and ord(char['char']) > 127:
                # Handle extended ASCII characters with hex escape sequences
                f.write(f'\\x{ord(char["char"]):02x}')
            else:
                f.write(char['char'])
        f.write('";\n\n')
        
        # Font array with minimal comments
        f.write(f'unsigned char font_variable[{len(characters)}][8] = {{\n')
        
        for i, char in enumerate(characters):
            hex_bytes = ascii_to_hex(char['ascii_art'])
            hex_str = ', '.join(f'0x{b:02x}' for b in [char['width']] + hex_bytes)
            
            # Add minimal comment for readability
            if char['char'] == ' ':
                comment = ' // space'
            elif char['char'] == '\\':
                comment = ' // backslash'
            elif len(char['char']) == 1 and ord(char['char']) > 127:
                comment = f' // 0x{ord(char["char"]):02x}'
            else:
                comment = f' // {char["char"]}'
            
            if i < len(characters) - 1:
                f.write(f'    {{{hex_str}}},{comment}\n')
            else:
                f.write(f'    {{{hex_str}}}{comment}\n')
        
        f.write('};\n\n')
        f.write('#endif\n')

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 font_generator.py input.txt output.h")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    try:
        characters = parse_font_file(input_file)
        generate_header(characters, output_file)
        print(f"Generated {output_file} with {len(characters)} characters")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
