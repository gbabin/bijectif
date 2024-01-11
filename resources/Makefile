all: bijectif.ico

bijectif.ico: icon-16.png icon-32.png icon-256.png
	convert $^ $@

icon-%.png: icon.svg
	convert -density 1000 -resize $*x$* -background transparent $< $@
