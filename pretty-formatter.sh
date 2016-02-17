#!/bin/bash

FORMATTER="clang-format"

for f in $(find ./src -name '*.h' -or -name '*.cpp')
do
	echo "Processing $f file..."
	${FORMATTER} -style=file -i $f
done
