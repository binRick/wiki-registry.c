uncrustify -c ./etc/uncrustify.cfg --replace src/*.c tests/*.c include/*.h
shfmt -w *.sh
find . -name "*.unc-backup*" -type f | xargs -I % unlink %
js-beautify -r clib.json
