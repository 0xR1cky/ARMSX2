#!/bin/sh

IFS="
"

printf "<RCC>\n"
printf "\t<qresource>\n"
for i in $(find . -not -iname '*.sh' -not -iname '*.qrc' -type f | cut -d'/' -f2-99); do
	printf "\t\t<file>%s</file>\n" "$i"
done
printf "\t</qresource>\n"
printf "</RCC>\n"

