uncrustify -c ./etc/uncrustify.cfg --replace */*.c */*.h
shfmt -w *.sh
find . -name "*.unc-backup*" -type f | xargs -I % unlink %
js-beautify -r clib.json
