#!/bin/sh

template="$1"

shift
includes="\\\\n"
for i in "$@"; do
	includes="${includes}#include <gthumb/"$i">\\\\n"
done

sed -e 's|@@|'"$includes"'|' -e 's|\\n|\
|g' $template
