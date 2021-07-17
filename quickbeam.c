// Author: Dr Stephen Braithwaite.
// This work is licensed under a Creative Commons
// Attribution-ShareAlike 4.0 International License.

// Uses very lazy code.  Quick and nasty, but seems to work well.  
// Requires library CscNetlib.   https://github.com/drbraithw8/CscNetlib

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define MEMCHECK_SILENT 1
#define version "1.5.1"

#include <CscNetLib/std.h>
#include <CscNetLib/cstr.h>
#include <CscNetLib/isvalid.h>

#define MaxLineLen 255
#define MaxWords 25
#define MinColSep 0.03

//-------- Global Variables  ----------
int lineNo = 0;
int topicNo = 0;
csc_bool_t isVerbatim = csc_FALSE;
csc_bool_t wasVerbatim = csc_FALSE;

// Stores output while processing frame.
csc_str_t *frmPre;
csc_str_t *frmTitle;
csc_str_t *frmSubtitle;
csc_str_t *frmGen;
csc_str_t *frmPost;

typedef enum
{	fontTarget_body=0
,	fontTarget_bullet1
,	fontTarget_bullet2
,	fontTarget_bullet3
,	fontTarget_topic
,	fontTarget_title
,	fontTarget_ref
,	fontTarget_n
} fontTarget_t;

typedef enum
{	bullType_none = 0
,	bullType_item
,	bullType_enum
,	bullType_desc
,	bullType_n
} bullType_t;

const char *sizeNames[] = {
				/* 0 */	    "tiny"
				/* 1 */	  , "scriptsize"
				/* 2 */	  , "footnotesize"
				/* 3 */	  , "small"
				/* 4 */	  , "normalsize"
				/* 5 */	  , "large"
				/* 6 */	  , "Large"
				/* 7 */	  , "LARGE"
				/* 8 */	  , "huge"
				/* 9 */	  , "Huge"
						  };

int fontSizNdxGlobal[fontTarget_n] = { 7, 6, 5, 4, 9, 4, 6 };
int fontSizNdxFrame[fontTarget_n];

const char *colors[] = { "pink", "red", "blue", "cyan", "green", "yellow",
						"brown", "black", "white", "purple", "orange",
						"magenta", "lime", "violet", "gray",
						"darkgray", "olive", "teal"
					  };
const char *shapes[] = { "square", "ball", "triangle", "circle"};
const char *items[] = { "item", "subitem", "subsubitem"};




//-------- Miscellaneous ---------------


csc_bool_t isSgnInt(const char *word)
{   char ch;
    if (word == NULL)
        return csc_FALSE;
    ch = *word;
    if (ch!='-' && ch!='+')
		return csc_FALSE;
	ch = *(++word);
    if (ch == '\0')
        return csc_FALSE;
    while((ch = *(word++)) != '\0')
        if (ch<'0' || ch>'9')
            return csc_FALSE;
    return csc_TRUE;
}


int arrStrIndex(const char **arr, int arrSiz, const char *str)
{	for (int i=0; i<arrSiz; i++)
	{	if (csc_streq(arr[i], str))
		{	return i;
		}
	}
	return -1;
}


csc_bool_t arrStrIncludes(const char **arr, int arrSiz, const char *str)
{	return arrStrIndex(arr,arrSiz,str) != -1;
}


void complainQuit(char *msg)
{	fprintf(stderr, "Error at line %d: %s !\n", lineNo, msg);
	exit(1);
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


// if bullet char is '*' or '+"
// then returns bullet char and sets *'level'.
// else returns 0
int getBulletLevel(char *line, int *level)
{	int nTabs = 0;
	int ch;
	while (ch=(*line++))
	{	if (ch == '\t')
		{	nTabs++;
		}
		else if (ch == '*')
		{	*level = nTabs+1;
			return bullType_item;
		}
		else if (ch == '+')
		{	*level = nTabs+1;
			return bullType_enum;
		}
		else if (ch == '[')
		{	*level = nTabs+1;
			return bullType_desc;
		}
		else
			return bullType_none;
	}
}	


int testAtLine(char *line, char **words)
{	int nWords;
	int ch = *line++;
 
#if (0)  // Dont skip whitespace.  The line must begin with @.
// Skip spaces.
	while (ch=(*line++))
	{	if (ch!=' ' && ch!='\t')
			break;
	}
#endif
 
// Look at the character.
	if (ch != '@')
		return -1;
 
// Break the line into words.
	nWords = csc_param_quote(words, line, MaxWords);
	return nWords;
}


//-------- Character escapes  ---------------
char *escape_expansions[] = { "\\textbackslash "
						  	, "\\textless "
						  	, "\\textgreater "
						  	, "\\textasciitilde "
						  	, "\\textasciicircum "
						  	, "\\{", "\\}", "\\&", "\\%"
						  	, "\\$", "\\#", "\\_"
						  	};
#define escape_size (sizeof(escape_expansions) / sizeof(char*))

typedef struct escape_s
{	char isEsc[escape_size];
} escape_t;

escape_t escapesGlobal;
escape_t escapesFrame;
escape_t escapesVerbatim;


int escape_escInd(int ch)
{	int ind;
	switch(ch)
	{	case '\\':ind=0;  break;
		case '<': ind=1;  break;
		case '>': ind=2;  break;
		case '~': ind=3;  break;
		case '^': ind=4;  break;
		case '{': ind=5;  break;
		case '}': ind=6;  break;
		case '&': ind=7;  break;
		case '%': ind=8;  break;
		case '$': ind=9;  break;
		case '#': ind=10; break;
		case '_': ind=11; break;
		default:  ind=-1; break;
	}
	return(ind);
}


void escape_setOnOff(escape_t *esc, char **words, int nWords)
{	
	if (nWords != 2)
		complainQuit("Wrong number of args for '@escOn' or '@escOff' line");

// Are we switching these escapes on or off?
	int val = csc_streq(words[0],"escOn");

// What characters are we switching on or off?
	char *escChars = words[1];
	if (csc_streq(escChars, "all"))
		escChars = "\\<>~^{}&%$#_";

// Switch them all on or off.
	int ch = *escChars++;
	while (ch != '\0')
	{	int ind = escape_escInd(ch);
		if (ind == -1)
			complainQuit("Unknown character in '@escOn' or '@escOff' argument");
		esc->isEsc[ind] = val;
		ch = *escChars++;
	}
}


void doEscLine(csc_str_t *out, char *line, escape_t *esc)
{	int ch = *line++;
	while (ch != '\0')
	{	int ind = escape_escInd(ch);
		if (ind==-1 || !esc->isEsc[ind])
		{	csc_str_append_ch(out, ch);
		}
		else
		{	csc_str_append(out, escape_expansions[ind]);
		}
		ch = *line++;
	}
}


//-------------- Prepare stuff to send ---------
 
#define prt csc_str_append_f


void setFontSiz(int fontSizNdx[], char **words, int nWords)
{
	typedef enum
	{	newSizNone=0
	,	newSizRel
	,	newSizAbs
	} newSizArn_t;

	int newSiz;
	int targNdx = -1;
	newSizArn_t arn = newSizNone;
	csc_bool_t isSetFontTarg[fontTarget_n];
	csc_bool_t wasSetFontTarg = csc_FALSE;

// No font target has been mentioned so far.
	for (int i=0; i<fontTarget_n; i++)
	{	isSetFontTarg[i] = csc_FALSE;
	}
 
// Loop through the arguments.
	for (int iWd=1; iWd<nWords; iWd++)
	{	
	// If its a named size, then set the size as an absolute.
		int sizNdx = arrStrIndex(sizeNames, csc_dim(sizeNames), words[iWd]);
		if (sizNdx > -1)
		{	if (arn == newSizNone)
			{	newSiz = sizNdx;
				arn = newSizAbs;
			}
			else
			{	complainQuit("@setFontSiz: Font size specified more than once");
			}
		}
 
	// If its a signed int, then set the size as a relative.
		else if (isSgnInt(words[iWd]))
		{	if (arn == newSizNone)
			{	newSiz = atoi(words[iWd]);
				arn = newSizRel;
			}
			else
			{	complainQuit("@setFontSiz: Font size specified more than once");
			}
		}
 
	// If its a font target then set the font target.
	// Multiple font targets are permitted.
		else if (csc_isValidRange_int(words[iWd], 0, 3, &targNdx))
		{	isSetFontTarg[targNdx] = csc_TRUE;
			wasSetFontTarg = csc_TRUE;
		}
		else if (csc_streq(words[iWd],"all"))
		{	for (int i=0; i<fontTarget_n; i++)
			{	isSetFontTarg[i] = csc_TRUE;
			}
			wasSetFontTarg = csc_TRUE;
		}
		else if (csc_streq(words[iWd],"title"))
		{	isSetFontTarg[fontTarget_title] = csc_TRUE;
			wasSetFontTarg = csc_TRUE;
		}
		else if (csc_streq(words[iWd],"topic"))
		{	isSetFontTarg[fontTarget_topic] = csc_TRUE;
			wasSetFontTarg = csc_TRUE;
		}
		else if (csc_streq(words[iWd],"ref"))
		{	isSetFontTarg[fontTarget_ref] = csc_TRUE;
			wasSetFontTarg = csc_TRUE;
		}
 
	// It must be none of the above
		else
		{	complainQuit("@setFontSiz: Arguments must be font target or size adjustment");
		}
	}
 
// Make that a level was specified.
	if (!wasSetFontTarg)
	{	complainQuit("@setBullet: No font target was specified");
	}
 
// Lets set font sizes.
	if (arn == newSizAbs)
	{	for (int i=0; i<fontTarget_n; i++)
		{	if (isSetFontTarg[i])
			{	fontSizNdx[i] = newSiz;
			}
		}
	}
	else if (arn == newSizRel)
	{	for (int i=0; i<fontTarget_n; i++)
		{	if (isSetFontTarg[i])
			{	int sizNdx = fontSizNdx[i] + newSiz;
				if (sizNdx < 0)
					sizNdx = 0;
				if (sizNdx >= csc_dim(sizeNames))
					sizNdx = csc_dim(sizeNames) - 1;
				fontSizNdx[i] = sizNdx;
			}
		}
	}
	else
	{	complainQuit("@setFontSiz: font size adjustment was not specified");
	}
}


void doSetBullet(char **words, int nWords)
{	char *color = NULL;
	char *shape = NULL;
	csc_bool_t levels[3] = {csc_FALSE, csc_FALSE, csc_FALSE};
	int level;
 
	for (int iWd=1; iWd<nWords; iWd++)
	{	
	// If its a color, then set the color.
		if (arrStrIncludes(colors, csc_dim(colors), words[iWd]))
		{	if (color == NULL)
			{	color = words[iWd];
			}
			else
			{	complainQuit("@setBullet: Color specified more than once");
			}
		}
 
	// If its a shape, then set the shape.
		else if (arrStrIncludes(shapes, csc_dim(shapes), words[iWd]))
		{	if (shape == NULL)
			{	shape = words[iWd];
			}
			else
			{	complainQuit("@setBullet: Shape specified more than once");
			}
		}
 
	// If its a bullet level, then set the level.
		else if (csc_isValidRange_int(words[iWd], 1, 3, &level))
		{	levels[level-1] = csc_TRUE;
		}
 
	// It must be none of the above
		else
		{	// fprintf(stderr, "words[%d]: \"%s\"\n", iWd, words[iWd]);
			complainQuit("@setBullet: Arguments can only be level, color or shape");
		}
	}
 
// Make sure that one or both of color and shape were set.
	if (shape==NULL && color==NULL)
	{	complainQuit("@setBullet: Neither color or shape was set");
	}
 
// Make that a level was specified.
	if (!levels[0] && !levels[1] && !levels[2])
	{	complainQuit("@setBullet: No level was set");
	}
 
// Lets set some shapes and colors.
	for (int iLvl=0; iLvl<3; iLvl++)
	{	if (levels[iLvl])
		{	if (shape != NULL)
			{	prt( frmGen, "\\setbeamertemplate{itemize %s}[%s]\n"
				   , items[iLvl], shape
				   );
			}
			if (color != NULL)
			{	prt( frmGen, "\\setbeamercolor{itemize %s}{fg=%s}\n"
				   , items[iLvl], color
				   );
			}
		}
	}
}


void doOpenBullets( int level
				  , bullType_t bullType
				  , csc_bool_t isImageLeft
				  )
{	
// Open the bullet level.
	if (bullType == bullType_item)
		prt(frmGen,"%s", "\\begin{itemize}");
	else if (bullType == bullType_enum)
		prt(frmGen,"%s", "\\begin{enumerate}");
	else if (bullType == bullType_desc)
		prt(frmGen,"%s", "\\begin{description}");
	else
		assert(bullType==bullType_item ||
		bullType==bullType_item || bullType==bullType_desc);
 
// Make the font smaller if we are in imageLeft mode.
	int fontSizNdx = fontSizNdxFrame[level];
	if (isImageLeft && fontSizNdx>0)
		fontSizNdx--;
 
// Set the text size.
	prt(frmGen, "\\%s", sizeNames[fontSizNdx]);
 
// EOL.
	prt(frmGen,"\n");
}


void doCloseBullets(bullType_t bullType)
{	if (bullType == bullType_item)
		prt(frmGen,"%s", "\\end{itemize}\n");
	else if (bullType == bullType_enum)
		prt(frmGen,"%s", "\\end{enumerate}\n");
	else if (bullType == bullType_desc)
		prt(frmGen,"%s", "\\end{description}\n");
	else
		assert(bullType==bullType_item || bullType==bullType_enum || bullType==bullType_desc);
}


void doTextLine(char *line)
{	int ch;
 
// Read past whitespace.
	ch = *line;
	while (ch=='\t' || ch==' ')
	{	ch = *++line;
	}
 
// Is it an enumeration.
	if (ch == '+')
	{	ch = *++line;
 
	// Does the enumeration have a starting number?
		if (ch>='0' && ch<='9')
		{	int enNum = 0;
			while (ch>='0' && ch<='9')
			{	enNum = enNum*10 + ch - '0';
				ch = *++line;
			}
			prt(frmGen,"\\setcounter{enumi}{%d}\n", enNum-1);
		}
	
	// Print the item.
		prt(frmGen, "%s", "\\item ");
	}
	else if (ch == '*')
	{	ch = *++line;
	
	// Print the item.
		prt(frmGen, "%s", "\\item ");
	}
	else if (ch == '[')
	{	ch = *++line;
		prt(frmGen, "%s", "\\item [");
 
 	// Dont accept empty description list item.
		if (ch == '[')
			complainQuit("Empty list item");
 
	// Read the description list item.
		while (ch!=']' && ch!='\0')
		{	prt(frmGen, "%c", ch);
			ch = *++line;
			if (ch == '\\')
			{	ch = *++line;
				if (ch != '\0')
				{	prt(frmGen, "%c", ch);
					ch = *++line;
				}
			}
		}
 
	// The description list item should be properly closed.
		if (ch == ']')
			ch = *++line;
		else
			complainQuit("Broken description list item");
	
	// Finish printing the item.
		prt(frmGen, "%s", "] ");
	}
 
// Write the remainder of the line.
	doEscLine(frmGen, line, &escapesFrame);
	csc_str_append_ch(frmGen,'\n');
}


void doImageLeft(char **words, int nWords)
{	double colWidth, imgWidth;
 
// Correct number of words?
	if (nWords != 4)
		complainQuit("Incorrect args for imageLeft");
 
// Get width for image.
	if (!csc_isValidRange_float(words[3], 0.01, 200, &imgWidth))
		complainQuit("Invalid image width for @imageLeft");
 
// Get column width for image.
	if (!csc_isValidRange_float(words[1], 0.1, 0.8, &colWidth))
		complainQuit("Invalid column width for @imageLeft");
 
// Print to include the file.
	prt(frmGen,
		"\\begin{columns}\n"
		"\\begin{column}{%0.3f\\textwidth}\n"
		"\\begin{figure}\n"
		"\\includegraphics[scale=%0.3f]{Images/%s}\n"
		"\\end{figure}\n"
		"\\end{column}\n"
		"\\begin{column}{%0.3f\\textwidth}\n"
		, colWidth, imgWidth, words[2], (1-colWidth-MinColSep)
		);
}


void doColumn(char **words, int nWords, double *cumColWidth)
{	double colWidth;
 
// Correct number of words?
	if (nWords == 1)
	{	colWidth = (1-*cumColWidth-MinColSep);
		*cumColWidth = 1;
	}
	else if (nWords == 2)
	{	if (!csc_isValid_float(words[1]))
			complainQuit("Invalid column width for @column");
		colWidth = atof(words[1]);
		if (colWidth <= 0.1)
			complainQuit("Column width too small for @column");
		if (colWidth >= (1-*cumColWidth-MinColSep))
			complainQuit("Cumulative column width greater than 1");
	}
	else
	{	complainQuit("Incorrect args for column");
	}
 
// Prepare to request this column.
	if (*cumColWidth > 0)
	{	prt(frmGen,"%s", "\\end{column}\n");
		*cumColWidth += (colWidth+MinColSep);
	}
	else
	{	prt(frmGen, "%s", "\\begin{columns}\n");
		*cumColWidth = (colWidth+MinColSep);
	}
 
// Request the column.
	prt(frmGen, "\\begin{column}{%0.3f\\textwidth}\n", colWidth);
}


void doImage(char **words, int nWords)
{	double imgWidth;
 
// Correct number of words?
	if (nWords != 3)
		complainQuit("Incorrect args for image");
 
// Get width for image.
	if (!csc_isValidRange_float(words[2], 0.01, 200, &imgWidth))
		complainQuit("Invalid image width for @image");
 
// Print to include the file.
	prt(frmGen,
		"\\begin{figure}\n"
		"\\includegraphics[scale=%s]{Images/%s}\n"
		"\\end{figure}\n"
		, words[2], words[1]
		);
}


void doBgColor(char **words, int nWords)
{	
// Correct number of words?
	if (nWords != 2)
		complainQuit("Incorrect args for background color");
 
// Enclose the frame with the background colour.
	prt(frmPre, "\\setbeamercolor{background canvas}{bg=%s}\n", words[1]);
}


void doBlankLine(int bulletLevel, char **words, int nWords)
{	
// Declare base size.
	int baseSize;
	csc_bool_t isBaseSize = csc_FALSE;
 
// Declare relative size.
	int relSize;
	csc_bool_t isRelSize = csc_FALSE;
 
// Final size;
	int sizeNdx;
 
// Handle @LL within a verbatim.
	if (isVerbatim)
	{	prt(frmGen, "\n");
		return;
	}
 
// Default base size.
	baseSize = fontSizNdxFrame[bulletLevel];
	relSize = 0;
 
// Loop through the arguments.
	for (int iWd=1; iWd<nWords; iWd++)
	{	int ndx;
 
	// If its a named size, then set the base size.
		ndx = arrStrIndex(sizeNames, csc_dim(sizeNames), words[iWd]);
		if (ndx > -1)
		{	if (isBaseSize)
			{	complainQuit("@LL: Base size for blank line specified more than once");
			}
			baseSize = ndx;
			isBaseSize = csc_TRUE;
		}
				
	// If its a signed int, then set the size as a relative.
		else if (isSgnInt(words[iWd]))
		{	if (isRelSize)
			{	complainQuit("@LL: Relative size for blank line specified more than once");
			}
			relSize = atoi(words[iWd]);
			isRelSize = csc_TRUE;
		}
 
	// If an unsigned int, then set base size to the size at bullet level.
		else if (csc_isValidRange_int(words[iWd], 0, 3, &ndx))
		{	if (isBaseSize)
			{	complainQuit("@LL: Base size for blank line specified more than once");
			}
			baseSize = fontSizNdxFrame[ndx];
			isBaseSize = csc_TRUE;
		}
 
	// If none of the above, then there is a problem.
		else
		{	complainQuit("@LL: Invalid argument for blank line size");
		}
	}
 
// The final size is the base size plus the relative size.
	sizeNdx = baseSize + relSize;
	if (sizeNdx >= csc_dim(sizeNames)) 
		sizeNdx = csc_dim(sizeNames) - 1;
	if (sizeNdx < 0)
		sizeNdx = 0;
				
// Print the vertical space.
	prt(frmGen, " {\\%s \\vspace{\\baselineskip} }\n", sizeNames[sizeNdx]);
}


void doOpenFrame(FILE *fout, char *title, int slideNum)
{	
// Flush out.
	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
 
// Form title.
    csc_str_assign(frmTitle, title);
 
// Copy the global font sizes to local ones.
	for (int i=0; i<fontTarget_n; i++)
	{	fontSizNdxFrame[i] = fontSizNdxGlobal[i];
	}
}


void sendCloseFrame(FILE *fout, int slideNum)
{
// Output any pre frame.
	if (csc_str_length(frmPre) > 0)
	{	fprintf(fout, "{\n");
		csc_str_out(frmPre,fout);
	}
 
// The frame title.
	fprintf(fout, "%s", "\\begin{frame}");
	if (wasVerbatim)
		fprintf(fout, "%s", "[fragile]");
	fprintf( fout, "\\%s\n\\%s"
		   , sizeNames[fontSizNdxFrame[fontTarget_title]]
		   , "frametitle{"
		   );
	fprintf(fout, "%s}\n", csc_str_charr(frmTitle));
	csc_str_truncate(frmTitle, 0);
 
// The frame subtitle.
	if (csc_str_length(frmSubtitle) > 0)
	{	fprintf(fout, "\\framesubtitle{%s}\n", csc_str_charr(frmSubtitle));
		csc_str_truncate(frmSubtitle, 0);
	}

// Set the level zero font size.
	fprintf( fout, "\\%s\n", sizeNames[fontSizNdxFrame[0]]);
 
// Output the frame.
	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
	fprintf(fout, "%s\n", "\\end{frame}");
 
// Output any post frame.  There is no post frame without pre frame.
	if (csc_str_length(frmPre) > 0)
	{	csc_str_out(frmPost,fout);
		fprintf(fout, "}\n");
	}
	csc_str_truncate(frmPre, 0);
	csc_str_truncate(frmPost, 0);
 
// A blank line for beauty.
	fprintf(fout,"\n");
}


void doTopic(FILE *fout, char *body, int slideNo)
{	
// Resources.
	csc_str_t *titleS = csc_str_new(NULL);
 
// Title as a string.
	prt(titleS, "Topic %d", ++topicNo); 
	const char *title = csc_str_charr(titleS);
 
// Flush out.
	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
 
// Use the topic color.
	fprintf( fout, "%s"
		   , "{\\setbeamercolor{background canvas}{bg=topicColor}\n"
		   );
 
// Open the frame, and frame title.
	fprintf( fout, "%s", "\\begin{frame}");
	fprintf( fout, "\\%s\n", sizeNames[fontSizNdxGlobal[fontTarget_title]]);
	fprintf( fout, "\\frametitle{%s}\n", title);
 
// Output the general form text.
	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
 
// Frame body and frame.
	fprintf(fout, "%s\n", "\\begin{figure}[htp]");
	fprintf(fout, "%s{\\%s %s}\n", "\\centering"
		   , sizeNames[fontSizNdxGlobal[fontTarget_topic]]
		   , body
		   );
	fprintf(fout, "%s\n", "\\end{figure}");
 
// end the frame and the topic color.
	fprintf(fout, "%s\n", "\\end{frame}");
	fprintf(fout, "%s\n", "}\n");
 
// Free resources.
	if (titleS)
		csc_str_free(titleS);
}


void sendCloseFile(FILE *fout)
{	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
	fprintf(fout, "%s", "\\end{document}\n");
}


void work(FILE *fin, FILE *fout)
{	char line[MaxLineLen];
	char *words[MaxWords];
	int nWords;
	bullType_t bulletStack[10];  // Bullet stack. 
	int level;
	bullType_t bullType;
	int slideNum = 0;
	csc_bool_t isImageLeft = csc_FALSE;
 
// Resources.
	frmPre = NULL;
	frmGen = NULL;
	frmTitle = NULL;
	frmSubtitle = NULL;
	frmPost = NULL;
 
// For storing output, until end of frame.
	frmPre = csc_str_new(NULL);   assert(frmPre);
	frmGen = csc_str_new(NULL);   assert(frmGen);
	frmTitle = csc_str_new(NULL);   assert(frmGen);
	frmSubtitle = csc_str_new(NULL);   assert(frmGen);
	frmPost = csc_str_new(NULL);   assert(frmPost);
 
	prt(frmGen, "%s",
"% NOTICE: Before you edit this file, know that this latex file was\n"
"% created using the following command:-\n"
"%   prompt$ quickbeam < ???.qb > ???.tex\n"
"% You need to edit the .qb file instead.\n"
"% quickbeam is available from https://github.com/drbraithw8/quickbeam.\n\n"
		);
 
// Escapes.
	if (csc_TRUE)
	{	char *wordsG[] = {"escOff", "all"};
		escape_setOnOff(&escapesGlobal, wordsG, 2);
		char *wordsV[] = {"escOff", "all"};
		escape_setOnOff(&escapesVerbatim, wordsV, 2);
	}
 
// Misc.
	int isInsideFrame = csc_FALSE;
	int isExpectUnderline = csc_FALSE;
	int bulletLevel = 0;
	int isColumn = csc_FALSE;
	double cumColWidth = 0;
 
// Loop through lines of file.
	while (csc_fgetline(fin, line, MaxLineLen) > -1)
	{	lineNo++;
 
	// Comment lines.
		if (line[0]=='/' && line[1]=='/')
			continue;
 
	// Literal lines.
		if (line[0] == '#')
		{	prt(frmGen,"%s\n", line+1);
			continue;
		}
 
	// Deal with outside of frame things.
		if (!isInsideFrame)
		{	if (isLineBlank(line))
			{	continue;
			}
			else if (getBulletLevel(line, &level))
			{	complainQuit("Expected Header line.");
			}
			else if (isExpectUnderline)
			{	if (isUnderline(line))
				{	isExpectUnderline = csc_FALSE;
					isInsideFrame = csc_TRUE;
					bulletLevel = 0;
					escapesFrame = escapesGlobal;
					wasVerbatim = csc_FALSE;
				}
				else
					complainQuit("Expected underline");
			}
			else if ((nWords = testAtLine(line,words)) > 0)
			{	if (csc_streq(words[0],"topic"))
				{	doTopic(fout, words[1], ++slideNum);
				}
	 			else if (csc_streq(words[0],"escOff") || csc_streq(words[0],"escOn"))
				{	escape_setOnOff(&escapesGlobal, words, nWords);
				}
				else if (csc_streq(words[0],"setBullet"))
				{	doSetBullet(words, nWords);
				}
				else if (csc_streq(words[0],"setFontSize"))
				{	setFontSiz(fontSizNdxGlobal, words, nWords);
				}
				else
				{	complainQuit("Unexpected @line");
				}
			}
			else
			{	if (isUnderline(line))
					complainQuit("Unexpected underline");
				else
				{	slideNum++;
					doOpenFrame(fout, line, slideNum);
					isExpectUnderline = csc_TRUE;
				}
			}
		}
 
		else // Deal with inside frame.
		{
			if (!isVerbatim && isLineBlank(line))
			{ // It means the frame is finished.
	 
			// Close each bullet level.
				while (bulletLevel > 0)
				{	doCloseBullets(bulletStack[--bulletLevel]);
				}
 
			// Close imageLeft.
				if (isImageLeft || isColumn)
				{	prt(frmGen,"%s", "\\end{column}\n");
					prt(frmGen,"%s", "\\end{columns}\n");
					isImageLeft = csc_FALSE;
					isColumn = csc_FALSE;
				}
 
			// Close the frame.
				sendCloseFrame(fout, slideNum);
				isInsideFrame = csc_FALSE;
			}
			else if ((nWords = testAtLine(line,words)) > 0)
			{ 
			// We found an @ line.
				if (csc_streq(words[0],"LL"))
				{	doBlankLine(bulletLevel, words, nWords);
				}	
	 			else if (csc_streq(words[0],"subtitle"))
				{	if (csc_str_length(frmSubtitle) > 0)
						complainQuit("@subtitle specified twice in frame");
					csc_str_assign(frmSubtitle, words[1]);
				}
	 			else if (csc_streq(words[0],"image"))
				{  // Process image.
 
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
 
				// Close image.
					if (isImageLeft)
					{	prt(frmGen,"%s", "\\end{column}\n");
						prt(frmGen,"%s", "\\end{columns}\n");
						isImageLeft = csc_FALSE;
					}
 
				// Do the image.
					doImage(words, nWords);
				}
	 			else if (csc_streq(words[0],"imageLeft"))
				{  // Process image.
 
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
 
				// Close imageLeft.
					if (isImageLeft || isColumn)
					{	prt(frmGen,"%s", "\\end{column}\n");
						prt(frmGen,"%s", "\\end{columns}\n");
						isImageLeft = csc_FALSE;
						isColumn = csc_FALSE;
					}
 
				// Do the image.
					doImageLeft(words, nWords);
					isImageLeft = csc_TRUE;
				}
	 			else if (csc_streq(words[0],"column"))
				{  // Process image.
 
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
 
				// Close imageLeft.
					if (isImageLeft)
					{	prt(frmGen,"%s", "\\end{column}\n");
						prt(frmGen,"%s", "\\end{columns}\n");
						isImageLeft = csc_FALSE;
					}
 
				// Do the column.
					if (!isColumn)
						cumColWidth = 0;
					doColumn(words, nWords, &cumColWidth);
					isColumn = csc_TRUE;
				}
				else if (csc_streq(words[0],"setFontSize"))
				{	setFontSiz(fontSizNdxFrame, words, nWords);
				}
				else if (csc_streq(words[0],"setBullet"))
				{	doSetBullet(words, nWords);
				}
	 			else if (csc_streq(words[0],"close"))
				{
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
 
				// Close imageLeft.
					if (isImageLeft || isColumn)
					{	prt(frmGen,"%s", "\\end{column}\n");
						prt(frmGen,"%s", "\\end{columns}\n");
						isImageLeft = csc_FALSE;
						isColumn = csc_FALSE;
					}
				}
	 			else if (csc_streq(words[0],"closeLists"))
				{
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
				}
				// else if (csc_streq(words[0],"closeList"))
				// {  /* Thought to be not useful. */
				// // Close one bullet level.
				// 	if (bulletLevel > 0)
				// 	{	doCloseBullets(bulletStack[--bulletLevel]);
				// 	}
				// }
	 			else if (csc_streq(words[0],"escOff") || csc_streq(words[0],"escOn"))
				{	escape_setOnOff(&escapesFrame, words, nWords);
				}
	 			else if (csc_streq(words[0],"verbatim"))
				{	if (nWords != 1)
						complainQuit("@verbatim does not take any arguments.");
					else if (isVerbatim)
						complainQuit("Already in verbatim mode.");
					else
					{	const char *escWords[] = {"escOn", "all"}; 
						isVerbatim = csc_TRUE;
						wasVerbatim = csc_TRUE;
						prt(frmGen, "%s", "\\begin{verbatim}\n");
					}
				}
	 			else if (csc_streq(words[0],"endVerbatim"))
				{	if (nWords != 1)
						complainQuit("@endVerbatim does not take any arguments.");
					else if (!isVerbatim)
						complainQuit("Not in verbatim mode.");
					else
					{	isVerbatim = csc_FALSE;
						prt(frmGen, "%s", "\\end{verbatim}\n");
					}
				}
				else if (csc_streq(words[0],"bgcolor"))
				{ // Background colour for this slide.
					doBgColor(words, nWords);
				}
				else
				{	complainQuit("unknown \"@\" directive");
				}
			}
			else	// It means that we found a line.
			{	if (isVerbatim)
				{	doEscLine(frmGen, line, &escapesVerbatim);
					csc_str_append_ch(frmGen,'\n');
				}
				else
				{	// We found a normal line.
					bullType = getBulletLevel(line, &level);
					if (bullType != bullType_none)  // Its a bulleted line.
					{
					// Close bullet levels.
						while (bulletLevel > level)
						{	doCloseBullets(bulletStack[--bulletLevel]);
						}
 
						if (bulletLevel == level)
						{ // Check if the bullet type is correct.
							if (bullType != bulletStack[bulletLevel-1])
							{	doCloseBullets(bulletStack[--bulletLevel]);
								doOpenBullets(level, bullType, isImageLeft||isColumn);
								bulletStack[bulletLevel++] = bullType;
							}
						}
						else if (bulletLevel == level-1)
						{ // Open another bullet level.
							doOpenBullets(level, bullType, isImageLeft||isColumn);
							bulletStack[bulletLevel++] = bullType;
						}
						else  // bulletLevel < level-1.
						{	complainQuit("Opening too many levels of bullets.");
						}
					} // Bulleted line.
					doTextLine(line);
				} // Found text line.
			} // Found text line.
		} // inside a frame.
	} // loop thru lines.
 
// We are finished. Close each bullet level.
	while (bulletLevel > 0)
	{	doCloseBullets(bulletStack[--bulletLevel]);
	}
 
// We are finished. Close imageLeft.
	if (isImageLeft || isColumn)
	{	prt(frmGen,"%s", "\\end{column}\n");
		prt(frmGen,"%s", "\\end{columns}\n");
		isImageLeft = csc_FALSE;
		isColumn = csc_FALSE;
	}
 
// Close the frame if needed.
	if (isInsideFrame)
	{	sendCloseFrame(fout, slideNum);
	}
 
// Close the frame.
	sendCloseFile(fout);
 
// Free resources.
	csc_str_free(frmPre);
	csc_str_free(frmGen);
	csc_str_free(frmTitle);
	csc_str_free(frmSubtitle);
	csc_str_free(frmPost);
}
 

void usage()
{	fprintf(stderr, "\n%s\n",
"usage: quickbeam [options]\n"
"\n"
"General Options\n"
"---------------\n"
"* --help : Shows help.\n"
"* --version : Displays the version\n"
"\n"
"Options for Input and Output\n"
"----------------------------\n"
"* -fi inPath : This program will read quickbeam input from the file\n"
"         \"inPath\".  Otherwise it will read from standard input.\n"
"* -fo outPath : This program will write its LaTeX output to the file\n"
"         \"outPath\".  Otherwise it will write to standard output.\n"
"\n"
"Options for Panopto Video Referencing\n"
"-------------------------------------\n"
"* -vr : Add the reference word followed by the slide number to every slide.\n"
"* -vR qbvPath : Read video-slide associations from the file \"qbvPath\"\n"
"                  and use it to provide links on each slide.\n"
"                  -vr and -vR are mutually exclusive.\n"
"* -vw word : The reference word for the video-slide associations is \"word\".\n"
"\n"
          );
	exit(1);
}


void help()
{	fprintf(stderr, "\n%s\n",
"usage: quickbeam [options]\n"
"\n"
"General Options\n"
"---------------\n"
"* --help : Shows this help.\n"
"\n"
"* --version : Displays the version\n"
"\n"
"Options for Input and Output\n"
"----------------------------\n"
"* -fi inPath : This program will read quickbeam input from the file\n"
"    \"inPath\".  Otherwise it will read from standard input.\n"
"\n"
"* -fo outPath : This program will write its LaTeX output to the file\n"
"    \"outPath\".  Otherwise it will write to standard output.\n"
"\n"
"Options for Panopto Video Referencing\n"
"-------------------------------------\n"
"* -vr : Add the reference word followed by the slide number to every slide.\n"
"\n"
"* -vR qbvPath : Read video-slide associations from the file \"qbvPath\"\n"
"                  and use it to provide links on each slide.\n"
"                  -vr and -vR are mutually exclusive.\n"
"\n"
"* -vw word : The reference word for the video-slide associations is \"word\".\n"
"\n"
"Parameters in video-slide associations file\n"
"-------------------------------------------\n"
"@url <URL of the video to link>\n"
"       This parameter is required.\n"
"\n"
"@slideSrch <word>\n"
"       QuickBeam will match this word in the video for the slide number.\n"
"       You can only specify this once.\n"
"\n"
"@titleSrch <word>\n"
"       Add a word to be matched in the title of the slides.\n"
"       You need to have titleSrch words or a SlideSrch word or both.\n"
"\n"
"@timeName <word>\n"
"       Specify the name of the time to add to the URL for a link into the video.\n"
"       This parameter is required.  You can only specify this once.\n"
"\n"
"@timeFormat <word>\n"
"       Specify the time format for a link into the video.\n"
"       This parameter is required.  You can only specify this once.  \n"
"       Two formats are recognised:-\n"
"       * \"seconds\" - Just an integer specifying the number of seconds\n"
"            from the start of the video, as used by Panopto.\n"
"       * \"youtube\" - The hours, minutes and seconds into the video, as\n"
"            used by Youtube.\n"
"\n"
"@presets <presetsName>\n"
"       A shorthand for specifying slideSrch, titleSrch, timeName and timeFormat\n"
"       with sensible values.  QuickBeam recognises the following presetsNames\n"
"       and give the following values to the above mentioned parameters in order:-\n"
"       * \"panopto\":-  \"vref\"  \"Topic\"  \"start\"  \"seconds\"\n"
"       * \"youtube\":-  \"vref\"  \"Topic\"  \"t\"  \"youtube\"\n"
"\n"
          );
	exit(0);
}


int main(int argc, char **argv)
{	
// Resources.
	FILE *fin=stdin;
	FILE *fout=stdout;
	csc_str_t *errStr = NULL;
 
// Parse the argument list.
	char *inPath = NULL;
	char *outPath = NULL;
	char *qbvPath = NULL;
	for (int iArg=1; iArg<argc; iArg++)
	{	char *p = argv[iArg];
 
		if (csc_streq(p, "--help"))
		{	help();
			exit(0);
		}
		if (csc_streq(p, "--version"))
		{	fprintf(stdout, "QuickBeam version %s\n", version);
			exit(0);
		}
		else if (*p=='-' && *(p+1)=='f' && *(p+3)=='\0' && iArg+1<argc)
		{	if (*(p+2) == 'i')
			{	iArg++;
				if (iArg == argc)
				{	fprintf(stderr, "Error: Unknown option 2 \"%s\".\n", p);
					usage();
				}
				inPath = argv[iArg];
			}
			else if (*(p+2) == 'o')
			{	iArg++;
				if (iArg == argc)
				{	fprintf(stderr, "Error: Unknown option 3 \"%s\".\n", p);
					usage();
				}
				outPath = argv[iArg];
			}
		}
		else
		{	fprintf(stderr, "Error: Unknown option 4 \"%s\".\n", p);
			usage();
		}
	}
 
// Open the input file.
	if (inPath)
	{	fin = fopen(inPath, "r");
		if (fin == NULL)
		{	fprintf(stderr, "Error: failed to open input file \"%s\"!\n", inPath);
			usage();
		}
	}
 
// Open the output file.
	if (outPath)
	{	fout = fopen(outPath, "w");
		if (fout == NULL)
		{	fprintf(stderr, "Error: failed to open input file \"%s\"!\n", outPath);
			usage();
		}
	}
 
// Process the quickbeam.
	work(fin, fout);
 
// Release resources.
	if (errStr)
		csc_str_free(errStr);
	if (fin != stdin)
		fclose(fin);
	if (fout != stdout)
		fclose(fout);
 
// Bye.
	exit(0);
}

