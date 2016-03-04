#!/bin/bash

FORMATTER="clang-format"
LINES=0

for f in $(find ./src ./tests -name '*.h' -or -name '*.cpp')
do
	echo "Processing $f file..."
	${FORMATTER} -style=file -i $f

	L=`wc -l $f | cut -f1 -d' '`
	LINES=$((${LINES} + ${L}))
	echo " --> $L lines of code"
done

echo ">>=====================<<"
echo "Total lines of code: $LINES"

