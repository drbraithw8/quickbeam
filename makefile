# Uses CscNetlib library:- https://github.com/drbraithw8/CscNetlib
.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

# The name of the original tex file.  "d" stands for "document name".
d = documentation
q = quickbeam

# The following is in order to determine the names of files.
docs = $d.qb
texFile = $d.tex
pdfFile = $d.pdf

# main target
all: $(pdfFile)

$q: $q.o
	gcc $< $(LIBS) -o $@

$(texFile): $(docs) $q
	./$q < $(docs) > $(texFile)

$(pdfFile): $(texFile)
	pdflatex $(texFile)

clean:
	-rm $d.aux $d.oldaux $d.bbl $d.blg $d.ind $d.toc $d.log $d.out \
	$d.tex $d.nav $d.snm $q.o tags 2> /dev/null \
	; true

cleanall: clean
	-rm $(pdfFile) $q  2> /dev/null \
	; true

.PHONY: clean cleanall
