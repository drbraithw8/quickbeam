#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <CscNetLib/std.h>
#include <CscNetLib/isvalid.h>
#include <CscNetLib/alloc.h>
#include <CscNetLib/cstr.h>
#include <CscNetLib/list.h>
#include "vidAssoc.h"

#define vidAssoc_MaxSlides 1000
#define MaxLineLen 255
#define MaxTimeStrLen 12
#define nSecsInvalid UINT16_MAX

#define prt csc_str_append_f

typedef enum
{	timeFmt_none = 0
,	timeFmt_seconds = 1
, 	timeFmt_youtube = 2
} timeFmt_t;

typedef struct titleMatch_t
{	char *word;
	int number;
	int seconds;
} titleMatch_t;


// Replaces occurences of the string 'find' in the string 'line' with the
// string 'repl'.  Case insensitive string location.
// This function is NOT general purpose, but belongs to replaceNumWords().
static void replaceNumWord(char *line, const char *find, const char *repl)
{	int ch;
	int flen;
	char *pos, *pl;
	const char *pr;
 
	flen = strlen(find);
	pos = strcasestr(line, find);
	while (pos != NULL)
	{	ch = *(pos+flen);
		if (ispunct(ch) || isspace(ch) || ch=='\0')
		{	pr = repl;
			pl = pos;
			ch = *pr;
			while (ch != '\0')
			{	*pl++ = ch;
				ch = *++pr;
			}
		}
		pos = strcasestr(pos+flen, find);
	}
}

// Overwrites text versions of numbers.
// E.g. "seven" will be replaced by "  7  "
static void replaceNumWords(char *line)
{	const char *repWds[] = 
	{	" one", "  1 "
	,	" won", "  1 "
	,	" two", "  2 "
	,	" too", "  2 "
	,	" to", " 2 "
	,	" three", "   3  "
	,	" four", "  4  "
	,	" for", "  4 "
	,	" five", "  5  "
	,	" six", "  6 "
	,	" seven", "   7  "
	,	" eight", "   8  "
	,	" ate", "  8 "
	,	" nine", "  9  "
	,	" ten", " 10 "
	};
	int nWds = sizeof(repWds) / (2*sizeof(repWds[0]));
	for (int i=0; i<nWds; i++)
	{	replaceNumWord(line, repWds[i*2], repWds[i*2+1]);
	}
}


titleMatch_t *titleMatch_new(char *word, int number, int seconds)
{	titleMatch_t *tt = csc_allocOne(titleMatch_t);
	tt->word = csc_alloc_str(word);
	tt->number = number;
	tt->seconds = seconds;
	return tt;
}

void titleMatch_free(titleMatch_t *tt)
{	if (tt->word)
		free(tt->word);
	free(tt);
}

typedef struct vidAssoc_t
{	vidAssoc_runMode_t mode;  // Do we mark slides, link to a video, or nothing.	
	char *wdMark;		// The word that will be used to mark each slide.
	char *slideSrch;		// Search for this word to look for the slide number.
	csc_list_t *titleSrch;		// Words to search, but number not slide number.
	csc_list_t *titleMatch;		// Words to search, but number not slide number.
	char *url;  // The URL which needs the time added.
	uint16_t *seconds;  // An array to hold the seconds.
	char *timeName;  // The string to introduce the time
	timeFmt_t timeFmt;  // Which format to print the seconds.
} vidAssoc_t;


vidAssoc_t *vidAssoc_new(vidAssoc_runMode_t mode, char *word)
{	vidAssoc_t *va = csc_allocOne(vidAssoc_t);
	va->timeFmt = timeFmt_none;
	va->mode = mode;
	va->wdMark = csc_alloc_str(word);
	va->slideSrch = NULL;
	va->titleMatch = NULL;
	va->titleSrch = NULL;
	va->timeName = NULL;
	va->seconds = NULL;
	va->url = NULL;
	return va;
}


void vidAssoc_free(vidAssoc_t *va)
{	assert(va != NULL);
	if (va->wdMark)
		free(va->wdMark);
	if (va->slideSrch)
		free(va->slideSrch);
	if (va->timeName)
		free(va->timeName);
	if (va->seconds)
		free(va->seconds);
	if (va->url)
		free(va->url);
	if (va->titleSrch)
		csc_list_freeblk(va->titleSrch);
	if (va->titleMatch)
	{	for (csc_list_t *lp=va->titleMatch; lp!=NULL; lp=lp->next)
			titleMatch_free((titleMatch_t*)lp->data);
		csc_list_free(va->titleMatch);
	}
	free(va);
}


static csc_bool_t isLineBlank(char *line)
{	int ch;
	while (ch=(*line++))
	{	if (ch!=' ' && ch!='\t')
			return csc_FALSE;
	}
	return csc_TRUE;
}


static csc_bool_t isUnderline(char *line)
{	int ch;
	if (isLineBlank(line))
		return csc_FALSE;
	while (ch=(*line++))
	{	if (ch!='-' && ch!=' ' && ch!='\t')
			return csc_FALSE;
	}
	return csc_TRUE;
}


static uint16_t timeStrToSecs(char *timeStr)
{	char *words[4];
 
	// Read the line that should contain the time.
	// Less than a minute:  0:05
	// A few minutes: 4:20
	// Longer: 20:34
	// Very long: 1:15:42
 
// Replace colons with spaces.
	int i = 0;
	int ch = timeStr[i];
	while (ch != '\0')
	{	if (ch == ':')
			timeStr[i] = ' ';
		ch = timeStr[++i];
	}
 
// Break time string into words
	int nWords = csc_param(words, timeStr, MaxTimeStrLen);
	if (nWords<2 || nWords>3)
		return nSecsInvalid;
 
// Evaluate the total seconds.
	int totSecs = 0;
	for (int i=0; i<nWords; i++)
	{	int secs;
		if (!csc_isValidRange_int(words[i], 0, 59, &secs))
			return nSecsInvalid;
		totSecs = totSecs * 60 + secs;
	}
	if (totSecs >= nSecsInvalid)
		return nSecsInvalid;
	else
		return totSecs;
}


static char *allocCheckUrl(char *line)
{	char *linep;
	char *endp;
	int ch;
 
// Skip leading spaces.
	linep = line;
	ch = *linep;
	while (isspace(ch))
	{	ch = *++linep;
	}
 
// There is something wrong with a URL that does not begin with an 'h'.
	if (ch!='h' && ch!='H')
		return NULL;
 
// Remove trailing spaces.
	endp = line + strlen(line);
	ch = *(endp-1);
	while (isspace(ch))
	{	*--endp = '\n';
		ch = *(endp-1);
	}
 
// Bye.
	return csc_alloc_str(linep);
}


static void doPresets(vidAssoc_t *va, char *topic, char *timeName, timeFmt_t timeFmt)
{
// Set the slide search string.
	if (va->slideSrch == NULL)
		va->slideSrch = csc_alloc_str(va->wdMark);
 
// Add Topic.
	csc_list_add(&va->titleSrch, csc_alloc_str(topic));
 
// Set the time name.
	if (va->timeName == NULL)
	{	va->timeName = csc_alloc_str(timeName);
	}
 
// Set the time format.
	if (va->timeFmt == timeFmt_none)
		va->timeFmt = timeFmt;
}


static void readParams(vidAssoc_t *va, FILE *fin, csc_str_t *errStr)
{	char line[MaxLineLen+1];
	char *words[3];
	char *wp;
	int nWords;
	int lineLen;
	int lineNo = 0;
 
// Check Args.
	csc_str_reset(errStr); // No errors found so far.
 
// Read the lines.
	csc_bool_t isErr = csc_FALSE;
	csc_bool_t isFin = csc_FALSE;
	while (!isFin && !isErr)
	{	
	// Read the line.
		if (!isFin && !isErr)
		{	lineLen = csc_fgetline(fin, line, MaxLineLen);
			lineNo++;
		}
 
	// Break line into parameters.
		if (!isFin && !isErr)
		{	nWords = csc_param_quote(words, line, MaxTimeStrLen);
			if (nWords == 0)
				continue;
			else if (nWords==1 && isUnderline(words[0]))
					isFin = csc_TRUE;
			else if (nWords != 2)
			{	prt( errStr, "%s %d: %s"
				   , "Bad parameter specification on line"
				   , lineNo, "Wrong number of args"
				   );
				isErr = csc_TRUE;
			}
		}
 
	// Interpret parameter.
		if (!isFin && !isErr)
		{	char *param = words[0];
			char *value = words[1];
			if (csc_streq(param,"@url"))
			{	if (va->url != NULL)
				{	prt( errStr, "@url specified more than once");
					isErr = csc_TRUE;
				}
				if (  value[0]!='h' || value[1]!='t'
				   || value[2]!='t' || value[3]!='p'
				   )
				{	prt( errStr, "@url value not plausible");
					isErr = csc_TRUE;
				}
				else
				{	va->url = csc_alloc_str(value);
				}
			}
			else if (csc_streq(param,"@titleSrch"))
			{	csc_list_add(&va->titleSrch, csc_alloc_str(value));
			}
			else if (csc_streq(param,"@slideSrch"))
			{	if (va->slideSrch != NULL)
				{	prt( errStr, "@slideSrch specified more than once");
					isErr = csc_TRUE;
				}
				else
				{	va->slideSrch = csc_alloc_str(value);
				}
			}
			else if (csc_streq(param,"@timeName"))
			{	if (va->timeName != NULL)
				{	prt( errStr, "@timeName specified more than once");
					isErr = csc_TRUE;
				}
				else
				{	va->timeName = csc_alloc_str(value);
				}
			}
			else if (csc_streq(param,"@timeFormat"))
			{	if (va->timeFmt != timeFmt_none)
				{	prt( errStr, "@timeFormat specified more than once");
					isErr = csc_TRUE;
				}
				else if (csc_streq(value,"seconds"))
				{	va->timeFmt = timeFmt_seconds;
				}
				else if (csc_streq(value,"youtube"))
				{	va->timeFmt = timeFmt_youtube;
				}
				else
				{	prt( errStr
					   , "Bad time format \"%s\" specified on line %d"
					   , value, lineNo
					   );
					isErr = csc_TRUE;
				}
			}
			else if (csc_streq(param,"@presets"))
			{	if (csc_streq(value,"panopto"))
				{	doPresets(va, "Topic", "start", timeFmt_seconds);
				}
				else if (csc_streq(value,"youtube"))
				{	doPresets(va, "Topic", "t", timeFmt_youtube);
				}
				else
				{	prt( errStr
					   , "Unknown preset \"%s\" specified on line %d"
					   , value, lineNo
					   );
					isErr = csc_TRUE;
				}
			}
			else
			{	prt( errStr
				   , "Unknown parameter \"%s\" specified on line %d"
				   , param, lineNo
				   );
				isErr = csc_TRUE;
			}
		} // if (!isFin && !isErr)
	} // while (!isFin && !isErr)
 
// Some checking.
	if (!isErr && va->slideSrch==NULL && va->titleSrch==NULL)
	{	prt( errStr, "At least one search word must be specified");
	}
	if (!isErr && va->timeName==NULL)
	{	prt( errStr, "@timeName must be specified");
	}
	if (!isErr && va->timeFmt==timeFmt_none)
	{	prt( errStr, "@timeFormat must be specified");
	}
}


// Finds word 'word' in string 'line'.  Expects integer following word,
// which is extracted. Integer is interpreted as follows.  Any 'l's or 'i's
// are converted to digit '1's.  Any 'o' or 'O' are converted to digit
// '0's.  Any leading minus sign is ignored. Returns -1 if 'word' does not
// exist in line or it is not followed by an integer.  
static int getNumWdFromStr(const char *line, const char *word)
{	char *wordp;
	int num;
	int ch;
 
// Find the word in the line.
	wordp = strcasestr(line, word);
	if (wordp == NULL)
		return -1;
 
// Look for integer following.
	wordp += strlen(word);
	ch = *wordp;
	while (isspace(ch) || ch=='-' || ch=='#')
	{	ch = *++wordp;
	}
 
// Does it seem that an integer follows?
	if (ch!='l' && ch!='i' && ch!='o' && ch !='O' && !isdigit(ch))
	{	return -1;
	}
 
// Process and terminate our integer.
	num = 0;
	while (csc_TRUE)
	{	
	// Look at this char.
		if (isdigit(ch))
		{  // OCR gets it right.
		}
		else if (ch=='l' || ch=='i')
		{	ch = '1';  // OCR, like human, makes this mistake.
		}
		else if (ch=='o' || ch=='O')
		{	ch = '0';  // OCR, like human, makes this mistake.
		}
		else
		{	break;  // Not a digit.
		}
 	
	// Move to next char.
		num = num*10 + (ch-'0');
		ch = *++wordp;
	}
 
// Return the number.
	if (num>0 && num<=vidAssoc_MaxSlides)
		return num;
	else
		return -1;
}


void vidAssoc_readAssocs(vidAssoc_t *va, char *qbvPath, csc_str_t *errStr)
{	int slideNum;
	int titleNum;
	int lineLen;
	int secs;
	char line[MaxLineLen+1];
 
// Check Args.
	assert(va->mode == vidAssoc_doLink);  // We only read associates to use them.
	csc_str_reset(errStr); // No errors found so far.
 
// Resources.  Must be released before returning from this routine.
	FILE *fin = NULL;
 
// Initialise seconds.
	va->seconds = csc_allocMany(uint16_t,vidAssoc_MaxSlides+1);
	uint16_t *seconds = va->seconds;
	for (int i=0; i<vidAssoc_MaxSlides+1; i++)
	{	seconds[i] = nSecsInvalid;
	}
 
// Open the associations file.
	fin = fopen(qbvPath, "r");
	if (fin == NULL)
	{	csc_str_append_f(errStr, "Failed to open file \"%s\" for read", qbvPath);
		goto freeAll;
	}
 
// Read in the URL and other parameters.
	readParams(va, fin, errStr);
	if (csc_str_length(errStr) > 0)
	{	goto freeAll;
	}
 
// Read the lines.
	lineLen = 0;
	while (lineLen != -1)
	{	csc_bool_t isErr = csc_FALSE;
		slideNum = -1;
		titleNum = -1;
		char *titleWd;
 
	// Read in the line.
		if (!isErr)
		{	lineLen = csc_fgetline(fin, line, MaxLineLen);
			if (lineLen == -1)
				isErr = csc_TRUE;
		}

	// Replace spelled numbers with digits.
		replaceNumWords(line);
 
 	// Look for the slide number in the line.
		if (!isErr && va->slideSrch)
		{	slideNum = getNumWdFromStr(line, va->slideSrch);
		}
 
 	// Look for the title in the line.
		if (!isErr && slideNum==-1)
		{	for (csc_list_t *lp=va->titleSrch; lp!=NULL; lp=lp->next)
			{	titleWd = lp->data;
				titleNum = getNumWdFromStr(line, titleWd);
				if (titleNum > -1)
					break;
			}
		}
 
	// If no match, then continue.
		if (!isErr && slideNum==-1 && titleNum==-1)
			isErr = csc_TRUE;
 
	// Read the line with the time string.
		if (!isErr)
		{	lineLen = csc_fgetline(fin, line, MaxLineLen);
			if (lineLen<0 || lineLen>MaxTimeStrLen)
				isErr = csc_TRUE;
		}
 
	// Convert the time string to seconds.
		if (!isErr)
		{	secs = timeStrToSecs(line);
			if (secs == nSecsInvalid)
				isErr = csc_TRUE;
		}
 
	// If the seconds is less than this slide's seconds, then overwrite.
		if (!isErr && slideNum>0)
		{	if (secs < seconds[slideNum])
				seconds[slideNum] = secs;
		}
	
	// Add our matching title.
		if (!isErr && titleNum>0)
		{	csc_list_add( &va->titleMatch
						, titleMatch_new(titleWd, titleNum, secs)
						);
		}
	}
		
freeAll:
	if (fin)
		fclose(fin);
}


static void appendSecs(vidAssoc_t *va, csc_str_t *frm, int secs)
{	if (va->timeFmt == timeFmt_seconds)
	{	csc_str_append_f(frm, "%d", secs);
	}
	else if (va->timeFmt == timeFmt_youtube)
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


void vidAssoc_do(vidAssoc_t *va, csc_str_t *frm, const char *title, int slideNum)
{	
	if (va==NULL || va->mode==vidAssoc_noAssoc)
	{	// Do nothing.
	}
	else if (va->mode == vidAssoc_markSlide)
	{
		// prt(frm, "%s", "\\begin{textblock}{0.5}(0.55,0.1)\n\n");
		// prt(frm, "%s%3d\n", va->word, slideNum);
		// prt(frm, "%s", "\\end{textblock}\n");
	}
	else if (va->mode == vidAssoc_doLink)
	{	int seconds = -1;
		int titleNum = -1;
 
	// Find the seconds for this slide.
		if (  slideNum<vidAssoc_MaxSlides   // Guard against array bounds.
		   && va->seconds[slideNum]<nSecsInvalid
		   )
		{	seconds = va->seconds[slideNum];
		}
		else
		{	for (csc_list_t *lp=va->titleMatch; lp!=NULL; lp=lp->next)
			{	titleMatch_t *tm = lp->data;
				titleNum = getNumWdFromStr(title, tm->word);
				if (titleNum == tm->number)
				{	seconds = tm->seconds;
					break;
				}
			}
		}
 
	// If we now know the seconds, then print the URL link.
		if (seconds > -1)
		{	prt(frm, "%s", "\\begin{textblock}{0.5}(0.9,0.1)\\small\n");
			prt( frm, "\\href{%s&%s=", va->url, va->timeName);
			appendSecs(va, frm, seconds);
			prt(frm, "%s", "}{\\textcolor{blue}{\\underline{video}}}\n");
			prt(frm, "%s", "\\end{textblock}\n");
		}
	}
	else
	{	fprintf(stderr, "%s\n", "Internal error: bad vrMode");
		exit(1);
	}
}

