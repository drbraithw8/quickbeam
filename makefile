# Uses CscNetlib library:- https://github.com/drbraithw8/CscNetlib
.c.o:
	gcc -c -std=gnu99 -I /usr/local/include  $<

LIBS :=  -L /usr/local/lib -lCscNet -lpthread

# The name of the original tex file.  "d" stands for "document name".
q = quickbeam

# main target
all: $q

$q: $q.o
	gcc $^ $(LIBS) -o $@

clean:
	-rm *.o tags 2> /dev/null ; true

cleanall: clean
	-rm $q 2> /dev/null ; true

.PHONY: clean cleanall

	
