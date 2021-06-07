#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <CscNetLib/std.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/cstr.h>
#include <CscNetLib/isvalid.h>
#include <CscNetLib/list.h>

#include "tubi.h"


const char *tubi_chars = "tubi";

typedef enum tubi_OnOff_e
{	tubi_none = 0,
	tubi_off,
	tubi_on
} tubi_OnOff_t;

typedef struct 
{	csc_bool_t oon[tubiE_n];
} tubiOon_t;


// --------- tubiTxt ---------------

typedef struct
{	char *txt;
	tubiOon_t oon;
} tubiText_t;

tubiText_t *tubiText_new(const char *txt, tubiOon_t *oon)
{	tubiText_t *tubiText = csc_allocOne(tubiText_t);
	tubiText->txt = csc_allocStr(txt);
	tubiText->oon = *oon;
	return tubiText;
}

void tubiText_free(tubiText_t *tubiText)
{	free(tubiText->txt);
	free(tubiText);
}
	

// --------- tubi ---------------

typedef struct tubi_t 
{	csc_list_t *nodes;
	tubiOon_t oon;
	csc_str_t *txt;
	csc_bool_t isRev;
} tubi_t;


tubi_t *tubi_new()
{	tubi_t *tubi = csc_allocOne(tubi_t);
	tubi->nodes = NULL;
	bzero(&tubi->oon, sizeof(tubi->oon));
	tubi->txt = csc_str_new(NULL);
	tubi->isRev = csc_FALSE;
	return tubi;
}


void tubi_free(tubi_t *tubi)
{	for (csc_list_t *p=tubi->nodes; p!=NULL; p=p->next)
	{	tubiText_free(p->data);
	}
	csc_list_free(tubi->nodes);
	csc_str_free(tubi->txt);
	free(tubi);
}


void tubi_addTubi(tubi_t *tubi, tubi_e attr, csc_bool_t isOn)
{	assert(!tubi->isRev);
 
// Only do stuff if the attributes have actually changed.
	if (tubi->oon.oon[attr] != isOn)
	{	
	// Any text for old oon needs to be pushed into the list.
		if (csc_str_length(tubi->txt) > 0)
		{	tubiText_t *tt = tubiText_new(csc_str_charr(tubi->txt), &tubi->oon);
			csc_list_add(&tubi->nodes, tt);
			csc_str_truncate(tubi->txt, 0);
		}
 
	// Make the change to the text.
		tubi->oon.oon[attr] = isOn;
	}
}


void tubi_addText(tubi_t *tubi, const char *txt)
{	assert(!tubi->isRev);
	const char *p = txt;
 
// Discard leading spaces.
	if (csc_str_length(tubi->txt) == 0)
	{	int ch = *p;
		while (isspace(ch))
		{	ch = *++p;
		}
	}
 
// Append.
    csc_str_append(tubi->txt, p);
}


void tubi_textDone(tubi_t *tubi)
{	if (csc_str_length(tubi->txt) > 0)
	{	tubiText_t *tt = tubiText_new(csc_str_charr(tubi->txt), &tubi->oon);
		csc_list_add(&tubi->nodes, tt);
		csc_str_truncate(tubi->txt, 0);
	}
}


void tubi_show(tubi_t *tubi, FILE *fout)
{	
// Reverse the list, if it has not already been done.
	if (!tubi->isRev)
	{	csc_list_rvrse(&tubi->nodes);
		tubi->isRev = csc_TRUE;
	}
 
// Print all nodes.
	for (csc_list_t *p=tubi->nodes; p!=NULL; p=p->next)
	{	tubiText_t *tt = p->data;
 
	// Print the attributes.
		for (int i=0; i<tubiE_n; i++)
		{	if (tt->oon.oon[i])
			{	fputc(toupper(tubi_chars[i]), fout);
			}
			else
			{	fputc(tubi_chars[i], fout);
			}
		}
 
	// Print the text.
		fprintf(fout, ":\"%s\"\n", tt->txt);
	}
		
	fprintf(fout, "\n");
}


void tubi_parse(tubi_t *tubi, const char *str)
{
// Resources.
	csc_str_t *cstr = csc_str_new(NULL);
 
	const char *p = str;
	int ch = *p;
	while (ch != '\0')
	{	if (ch != '@')
		{	csc_str_append_ch(cstr, ch);
		}
		else
		{	ch = *++p;
			if (ch == '@')
			{	csc_str_append_ch(cstr, ch);
			}
			else
			{	tubi_addText(tubi, csc_str_charr(cstr));
				csc_str_truncate(cstr, 0);
				switch (ch)
				{	case 'T':
						tubi_addTubi(tubi, tubiE_t, csc_TRUE);
						break;
					case 't':
						tubi_addTubi(tubi, tubiE_t, csc_FALSE);
						break;
					case 'U':
						tubi_addTubi(tubi, tubiE_u, csc_TRUE);
						break;
					case 'u':
						tubi_addTubi(tubi, tubiE_u, csc_FALSE);
						break;
					case 'B':
						tubi_addTubi(tubi, tubiE_b, csc_TRUE);
						break;
					case 'b':
						tubi_addTubi(tubi, tubiE_b, csc_FALSE);
						break;
					case 'I':
						tubi_addTubi(tubi, tubiE_i, csc_TRUE);
						break;
					case 'i':
						tubi_addTubi(tubi, tubiE_i, csc_FALSE);
						break;
 
					default:
						fprintf(stderr, "Bad @ !\n");
						exit(1);
				}
			}
		}
		ch = *++p;
	}
 
// Left over characters.
	tubi_addText(tubi, csc_str_charr(cstr));
	tubi_textDone(tubi);
 
// Free resources.
	csc_str_free(cstr);
}


int main(int argc, char **argv)
{	
// Resources.
	csc_str_t *cstr = csc_str_new(NULL);
 
	while (csc_str_getline(cstr, stdin) != -1)
	{	printf("\"%s\"\n", csc_str_charr(cstr));
		tubi_t *tubi = tubi_new();
		tubi_parse(tubi, csc_str_charr(cstr));
		tubi_show(tubi, stdout);
		tubi_free(tubi);
	}
 
// Free resources.
	csc_str_free(cstr);
 
// Testing for memory leaks.
	exit(0);
}
