#!/usr/bin/env ruby1.8

strings = []
Dir.glob("**/*.gthtml") do |filename|
  File.open("./#{ filename }").each_line do |line|
    if line =~ /<%=[ \t]+translate[ \t]+'(([^\\']|\\.)*)'/
      strings << $1
    end
  end
end

puts <<EOF
/* DO NOT EDIT.  This file is generated automatically. */

/*
 *  GThumb
 *
 *  Copyright (C) #{ Time::now.year } Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <glib/gi18n.h>

static char *text[] = {
EOF

strings.uniq.sort.each do |string|
  puts "\tN_(\"#{ string.gsub('\\', '\\\\').gsub('"', '\\"') }\"),"  
end

puts <<EOF
\tNULL
};
EOF
