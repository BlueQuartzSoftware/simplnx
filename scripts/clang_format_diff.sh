#!/bin/bash

PREVIOUS_COMMIT="$1"
CURRENT_COMMIT="$2"
OPTION="$3"

format_options="--dry-run --Werror"

if [ "$OPTION" = "modify" ]; then
  format_options="-i"
fi

# regex
# h, H, hpp, hh, h++, hxx
# c, C, cpp, cc, c++, cxx
files=$(git diff --name-only "$PREVIOUS_COMMIT"..."$CURRENT_COMMIT" | grep -E '\.((c|C)c?(pp|xx|\+\+)*$|(h|H)h?(pp|xx|\+\+)*$)')

for file in $files; do
  clang-format-10 $format_options --style=file "$file"
done
