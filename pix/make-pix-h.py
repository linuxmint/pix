#!/usr/bin/env python3

import sys
import re

if len(sys.argv) <= 3:
    print("Usage: make-gthumb-h.py INPUT_FILE OUTPUT_FILE HEADER_FILE [HEADER_FILE [..]]")
    exit(1)

infile = sys.argv[1]
outfile = sys.argv[2]
headers = "\n".join(map(lambda x: "#include <{0}>".format(x), sys.argv[3:]))

with open(outfile, 'w') as to_file, open(infile) as from_file:
    for line in iter(from_file.readline, ''):
        if line == "@HEADERS@\n":
            line = headers + "\n"
        to_file.write(line)
