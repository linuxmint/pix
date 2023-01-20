#!/usr/bin/env python3

import sys
import os
import subprocess

if len(sys.argv) <= 1:
    print("Usage: make-enums.py DIR [DIR [..]]")
    exit(1)

# Search header files in all the specified directories

dirs = sys.argv[1:]
all_files = []
for root_dir in dirs:
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            if file.endswith('.h'):
                all_files.append(os.path.join(root, file))

subprocess.call(['glib-mkenums',
  '--comments', '<!-- @comment@ -->',
  '--fhead',    '<schemalist>',
  '--vhead',    '  <@type@ id="org.gnome.gthumb.@EnumName@">',
  '--vprod',    '    <value nick="@valuenick@" value="@valuenum@"/>',
  '--vtail',    '  </@type@>',
  '--ftail',    '</schemalist>' 
] + all_files)
