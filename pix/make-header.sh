#!/bin/sh

template="$1"

shift
includes="\\\\n"
for i in "$@"; do
	includes="${includes}#include <pix/"$i">\\\\n"
done

sed -e 's|@@|'"$includes"'|' -e 's|\\n|\
|g' $template
