#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Convert files to C header arrays for embedding in bootloader MSC
"""

import sys
import os
import re

def minify_html(html_content):
    """Minify HTML by removing unnecessary whitespace"""
    # Remove comments
    html_content = re.sub(r'<!--.*?-->', '', html_content, flags=re.DOTALL)
    
    # Remove whitespace between tags
    html_content = re.sub(r'>\s+<', '><', html_content)
    
    # Remove leading/trailing whitespace on each line
    html_content = re.sub(r'\n\s+', '', html_content)
    html_content = re.sub(r'\s+\n', '', html_content)
    
    # Collapse multiple spaces into one
    html_content = re.sub(r'\s{2,}', ' ', html_content)
    
    # Remove newlines
    html_content = html_content.replace('\n', '')
    
    return html_content.strip()

def file_to_c_header(input_file, output_file, var_name):
    """Convert a file to a C header with byte array"""
    
    with open(input_file, 'rb') as f:
        data = f.read()
    
    # Minify HTML files
    if input_file.endswith('.html') or input_file.endswith('.htm'):
        try:
            html_text = data.decode('utf-8')
            minified = minify_html(html_text)
            data = minified.encode('utf-8')
            print(f"HTML minified: {len(html_text)} -> {len(data)} bytes")
        except Exception as e:
            print(f"Warning: Failed to minify HTML: {e}")
    
    with open(output_file, 'w') as f:
        f.write(f"/* Auto-generated from {os.path.basename(input_file)} */\n\n")
        f.write(f"static const unsigned char {var_name}[] = {{\n")
        
        # Write bytes in rows of 12
        for i in range(0, len(data), 12):
            chunk = data[i:i+12]
            hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
            f.write(f"    {hex_str},\n")
        
        f.write("};\n\n")
        f.write(f"static const unsigned int {var_name}_size = {len(data)};\n")

def convert_filename_to_fat16(filename):
    """Convert filename to FAT16 8.3 format"""
    base, ext = os.path.splitext(filename)
    
    # Remove extension dot and convert to uppercase
    base = base.upper()[:8]
    ext = ext.lstrip('.').upper()[:3]
    
    # Pad with spaces
    base = base.ljust(8)
    ext = ext.ljust(3)
    
    return f"{base}{ext}"

if __name__ == '__main__':
    if len(sys.argv) < 2:
        # Default: convert webdfu_ultra.html from scripts directory
        script_dir = os.path.dirname(os.path.abspath(__file__))
        input_file = os.path.join(script_dir, 'webdfu_ultra.html')
        output_file = os.path.join(os.path.dirname(script_dir), 'boot', 'dfu_webusb.h')
        var_name = 'file_DFU_HTM'
    else:
        input_file = sys.argv[1]
        
        if len(sys.argv) > 2:
            output_file = sys.argv[2]
        else:
            base = os.path.splitext(input_file)[0]
            output_file = base + '.h'
        
        if len(sys.argv) > 3:
            var_name = sys.argv[3]
        else:
            var_name = 'file_' + os.path.basename(os.path.splitext(input_file)[0]).replace('-', '_')
    
    print(f"Converting {input_file} to {output_file} (variable: {var_name})")
    print(f"FAT16 name: {convert_filename_to_fat16(os.path.basename(input_file))}")
    
    file_to_c_header(input_file, output_file, var_name)
    print("Done!")
