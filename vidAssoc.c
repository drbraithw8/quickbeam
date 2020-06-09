#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <CscNetLib/std.h>
#include <CscNetLib/isvalid.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/cstr.h>
#include "vidAssoc.h"

typedef enum
{	secsFmt_none = 0
,	secsFmt_seconds = 1
, 	secsFmt_hms = 2
} secsFmt_t;

#define vidAssoc_MaxSlides 1000
typedef struct
{	vidAssoc_runMode_t mode;  // Do we mark slides, link to a video, or nothing.	
	char *word;		// The word that will be used to search in the video.
	char *url;  // The URL which needs the time added.
	short *seconds;  // An array to hold the seconds.
	char *timeName;  // The string to introduce the time
	secsFmt_t secsFmt;  // Which format to print the seconds.
} vidAssoc_t;

static vidAssoc_t *assoc = NULL;




char *vidAssoc_init(char *qbvPath, vidAssoc_runMode_t mode, char *word)
{	assert(assoc == NULL);
	char *errStr = NULL;

// Direct assignment.
	assoc = csc_allocOne(vidAssoc_t);
	assoc->mode = mode;
	assoc->word = word;

// If we are creating the links.
	if (mode != vidAssoc_doLink)
	{	assoc->url = NULL;
		assoc->seconds = NULL;
		assoc->timeName = NULL;
		assoc->secsFmt = secsFmt_none;
	}
	else
	{  // TODO

	// Initialise all seconds to zero.
		assoc->seconds = csc_allocMany(short,vidAssoc_MaxSlides+1);
		short *seconds = assoc->seconds;
		for (int i=0; i<vidAssoc_MaxSlides+1; i++)
		{	seconds[i] = -1;
		}
	}

	return errStr;
}


void vidAssoc_end(void)
{	assert(assoc != NULL);
	if (assoc->seconds)
		free(assoc->seconds);
	free(assoc);
	assoc = NULL;
}


static void appendSecs(csc_str_t *frm, int secs)
{	if (assoc->secsFmt == secsFmt_seconds)
	{	csc_str_append_f(frm, "%d", secs);
	}
	else if (assoc->secsFmt == secsFmt_hms)
	{	int hours = secs / 3600;
		secs = secs - 3600 * hours;	
		int mins = secs / 60;
		secs = secs - 60 * mins;	
		csc_str_append_f(frm, "%dh%dm%ds", hours, mins, secs);
	}
	else
	{	assert(csc_FALSE);
	}
}


#define prt csc_str_append_f
void vidAssoc_do(csc_str_t *frm, int slideNum)
{	assert(assoc != NULL);
	if (assoc->mode == vidAssoc_noAssoc)
	{	// Do nothing.
	}
	else if (assoc->mode == vidAssoc_markSlide)
	{	prt(frm, "%s", "\\begin{textblock}{0.5}(0.85,0.1)\n\\small\n");
		prt(frm, "%s %3d\n", assoc->word, slideNum);
		prt(frm, "%s", "\\end{textblock}\n");
	}
	else if (assoc->mode == vidAssoc_doLink)
	{	if (slideNum<vidAssoc_MaxSlides && assoc->seconds[slideNum]>=0)
		{	prt(frm, "%s", "\\begin{textblock}{0.5}(0.9,0.1)\\small\n");
			prt( frm, "\\href{%s&%s=", assoc->url, assoc->timeName);
			appendSecs(frm, assoc->seconds[slideNum]);
			prt(frm, "%s", "}{\\underline{video}}\n");
			prt(frm, "%s", "\\end{textblock}\n");
		}
	}
	else
	{	fprintf(stderr, "%s\n", "Internal error: bad vrMode");
		exit(1);
	}
}
