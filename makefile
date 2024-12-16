.c.o:
	gcc -c -std=gnu11 -fPIE -I /usr/local/include  $<

LIBS :=  -fPIE 

FILES := csc_std.o csc_alloc.o csc_str.o csc_isvalid.o csc_list.o \
         tubi.o quickbeam.o memcheck.o

# main target
all: quickbeam

quickbeam: $(FILES)
	gcc $^ $(LIBS) -o $@

clean:
	-rm *.o tags 2> /dev/null ; true

cleanall: clean
	-rm quickbeam 2> /dev/null ; true

.PHONY: clean cleanall

	
