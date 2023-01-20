#!/usr/bin/env python3

import sys
import re

if len(sys.argv) != 3:
    print("Usage: make-authors-tab.py INPUT_FILE OUTPUT_FILE")
    exit(1)

infile = sys.argv[1]
outfile = sys.argv[2]

with open(outfile, 'w') as to_file, open(infile) as from_file:
    for line in iter(from_file.readline, ''):
        # wrap the line inside double quotes, and adds a trailing comma
        new_line = re.sub(r'^(.*)$', r'"\1",', line)
        to_file.write(new_line) 
