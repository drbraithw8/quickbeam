# Uses CscNetlib library:- https://github.com/drbraithw8/CscNetlib
.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

# The name of the original tex file.  "d" stands for "document name".
d = slides
q = quickbeam

# The following is in order to determine the names of files.
doc = $d.qb

# main target
all: $(pdfFile)

$q: $q.o vidAssoc.o
	gcc $^ $(LIBS) -o $@

texFile = $d.tex
$(texFile): $(doc) $q
	./$q < $< > $@

texFile_r = $d_r.tex
$(texFile_r): $(doc) $q
	./$q -vr < $< > $@

texFile_R = $d_R.tex
vrefs = $d.vrefs
$(texFile_R): $(doc) $q $(vrefs)
	./$q -vR $(vrefs) < $< > $@

pdfFile = $d.pdf
$(pdfFile): $(texFile)
	pdflatex $<

pdfFile_r = $d_r.pdf
$(pdfFile_r): $(texFile_r)
	pdflatex $<

pdfFile_R = $d_R.pdf
$(pdfFile_R): $(texFile_R)
	pdflatex $<

clean:
	-rm *.aux *.oldaux *.bbl *.blg *.ind *.toc *.log *.out *.snm *.tex *.nav *.snm *.o \
	  tags 2> /dev/null ; true

cleanall: clean
	-rm $q *.pdf 2> /dev/null ; true

.PHONY: clean cleanall

	
