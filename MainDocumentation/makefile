
# The name of the original tex file.  "d" stands for "document name".
d = slides

# The path of the quickbeam program.  Adjust the directory path as needed.
# If quickbeam is already in your path, then remove the directory path.
q = ../quickbeam

# The following is in order to determine the names of files.
doc = $d.qb

# Rules.
all: $d.pdf

$d.tex: $d.qb
	$q < $< > $@

$d.pdf: $d.tex
	pdflatex $<

clean:
	-rm *.aux *.oldaux *.bbl *.blg *.ind *.toc *.log *.out *.snm \
	  *.vrb *.tex *.nav *.snm 2> /dev/null ; true

cleanall: clean
	-rm *.pdf 2> /dev/null ; true

.PHONY: clean cleanall

