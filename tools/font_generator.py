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
    """Convert ASCII art to hex bytes and calculate dynamic width"""
    # Find the rightmost column with any '#' character
    max_width = 0
    for line in ascii_lines:
        for col in range(len(line) - 1, -1, -1):  # Search from right to left
            if col < len(line) and line[col] == '#':
                max_width = max(max_width, col + 1)
                break
    
    # Ensure minimum width of 1 for space character
    actual_width = max(1, max_width)
    
    hex_bytes = [0] * 7  # 7 bytes for font data (max possible width)
    
    for col in range(min(7, actual_width)):
        byte_val = 0
        for row in range(8):
            if row < len(ascii_lines) and col < len(ascii_lines[row]):
                if ascii_lines[row][col] == '#':
                    # Bit 0 is top row (row 0), bit 7 is bottom row (row 7)
                    # This matches the display logic: (1 << row)
                    byte_val |= (1 << row)
        hex_bytes[col] = byte_val
    
    return hex_bytes, actual_width

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
                char_names = line.split(':', 1)[1].strip()
                # Handle special case where the character itself is a comma
                if char_names == ',':
                    char_info['chars'] = [',']
                else:
                    # Parse comma-separated character names
                    char_list = [name.strip() for name in char_names.split(',')]
                    char_info['chars'] = [parse_char_name(name) for name in char_list]
                    # Filter out None values
                    char_info['chars'] = [c for c in char_info['chars'] if c is not None]
            elif line.startswith('WIDTH:'):
                char_info['width'] = int(line.split(':', 1)[1].strip())
            elif line and not line.startswith('CHAR:') and not line.startswith('WIDTH:'):
                ascii_lines.append(line)
        
        if len(ascii_lines) == 8 and 'chars' in char_info and char_info['chars']:
            hex_bytes, dynamic_width = ascii_to_hex(ascii_lines)
            char_info['hex_bytes'] = hex_bytes
            char_info['width'] = dynamic_width  # Override manual width with calculated width
            char_info['ascii_art'] = ascii_lines
            characters.append(char_info)
        else:
            raise ValueError(f"Invalid font definition: {block}")
    
    return characters

def parse_char_name(char_name):
    """Convert display names back to actual characters"""
    if char_name == 'SPACE':
        return ' '
    elif char_name == 'NEWLINE':
        return '\n'
    elif char_name == 'TAB':
        return '\t'
    elif char_name.startswith('0x'):
        # Handle hex display names like '0xe6'
        hex_val = char_name[2:]
        try:
            return chr(int(hex_val, 16))
        except ValueError:
            return None
    else:
        return char_name

def generate_header(characters, output_file):
    """Generate modern C++ header file with std::unordered_map"""
    # Create expanded character list from multi-character definitions
    expanded_chars = []
    
    for char_def in characters:
        # Create separate entries for each character in the definition
        for i, char in enumerate(char_def['chars']):
            char_info = {
                'char': char,
                'width': char_def['width'],
                'hex_bytes': char_def['hex_bytes'],
                'ascii_art': char_def['ascii_art']
            }
            
            # Mark additional characters as mapped for comments
            if i > 0:
                char_info['mapped_from'] = char_def['chars'][0]
            
            expanded_chars.append(char_info)
    
    with open(output_file, 'w') as f:
        f.write('#ifndef FONT_GENERATED_HPP\n')
        f.write('#define FONT_GENERATED_HPP\n\n')
        f.write('#include <unordered_map>\n')
        f.write('#include <vector>\n')
        f.write('#include <cstdint>\n\n')
        
        f.write('namespace font {\n\n')
        
        # Generate the font data type - just a vector of columns
        f.write('using GlyphData = std::vector<uint8_t>;\n\n')
        
        # Generate the font map
        f.write('const std::unordered_map<char, GlyphData> fontMap = {\n')
        
        valid_chars = []
        for char in expanded_chars:
            # Skip empty or invalid characters
            if not char['char'] or len(char['char']) == 0:
                continue
            valid_chars.append(char)
        
        for i, char in enumerate(valid_chars):
            # Only include actual glyph data columns (no width encoding, no padding)
            actual_columns = char['hex_bytes'][:char['width']]
            columns_str = ', '.join(f'0x{b:02x}' for b in actual_columns)
            
            # Generate character key with proper escaping
            if char['char'] == ' ':
                char_key = "' '"
                comment = ' // space'
            elif char['char'] == '\'':
                char_key = "'\\''"
                comment = ' // single quote'
            elif char['char'] == '\\':
                char_key = "'\\\\'"
                comment = ' // backslash'
            elif char['char'] == '\n':
                char_key = "'\\n'"
                comment = ' // newline'
            elif char['char'] == '\t':
                char_key = "'\\t'"
                comment = ' // tab'
            elif len(char['char']) == 1 and ord(char['char']) > 127:
                char_key = f"'\\x{ord(char['char']):02x}'"
                if 'mapped_from' in char:
                    mapped_from_hex = f'0x{ord(char["mapped_from"]):02x}'
                    comment = f' // 0x{ord(char["char"]):02x} -> {mapped_from_hex}'
                else:
                    comment = f' // 0x{ord(char["char"]):02x}'
            else:
                char_key = f"'{char['char']}'"
                if 'mapped_from' in char:
                    comment = f' // {char["char"]} -> {char["mapped_from"]}'
                else:
                    comment = f' // {char["char"]}'
            
            if i < len(valid_chars) - 1:
                f.write(f'    {{{char_key}, {{{columns_str}}}}},' + comment + '\n')
            else:
                f.write(f'    {{{char_key}, {{{columns_str}}}}}' + comment + '\n')
        
        f.write('};\n\n')
        
        # Generate helper function to get glyph with fallback
        f.write('inline const GlyphData* getGlyph(char c) {\n')
        f.write('    auto it = fontMap.find(c);\n')
        f.write('    if (it != fontMap.end()) {\n')
        f.write('        return &it->second;\n')
        f.write('    }\n')
        f.write('    // Fallback to space character\n')
        f.write('    auto fallback = fontMap.find(\' \');\n')
        f.write('    return (fallback != fontMap.end()) ? &fallback->second : nullptr;\n')
        f.write('}\n\n')
        
        f.write('} // namespace font\n\n')
        f.write('#endif // FONT_GENERATED_HPP\n')

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 font_generator.py input.txt output.h")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    try:
        characters = parse_font_file(input_file)
        generate_header(characters, output_file)
        
        # Count total characters including mapped ones
        total_chars = sum(len(char_def['chars']) for char_def in characters)
        mapped_chars = total_chars - len(characters)
        
        print(f"Generated {output_file} with {len(characters)} character definitions and {mapped_chars} mappings ({total_chars} total)")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
