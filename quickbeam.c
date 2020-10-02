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
#define version "1.2.3"

#include <CscNetLib/std.h>
#include <CscNetLib/cstr.h>
#include <CscNetLib/isvalid.h>

#include "vidAssoc.h"

#define MaxLineLen 255
#define MaxWords 25
#define MinColSep 0.03

//-------- Global Variables  ----------
int lineNo = 0;
int topicNo = 0;
csc_bool_t isRefInTitle = csc_FALSE;
csc_bool_t isNoRef = csc_FALSE;

// Stores output while processing frame.
csc_str_t *frmPre;
csc_str_t *frmTitle;
csc_str_t *frmGen;
csc_str_t *frmPost;

char *refWord = "vref";


//-------- Miscellaneous ---------------

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
		else if (ch=='*' || ch=='+')
		{	*level = nTabs+1;
			return ch;
		}
		else if (ch == ' ')
		{
		}
		else
			return csc_FALSE;
	}
}	


int testAtLine(char *line, char **words)
{	int ch, nWords;
 
// Skip spaces.
	while (ch=(*line++))
	{	if (ch!=' ' && ch!='\t')
			break;
	}
 
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


int escape_escInd(int ch)
{	int ind;
	switch(ch)
	{	case '\\':ind=0; break;
		case '<': ind=1; break;
		case '>': ind=2; break;
		case '~': ind=3; break;
		case '^': ind=4; break;
		case '{': ind=5; break;
		case '}': ind=6; break;
		case '&': ind=7; break;
		case '%': ind=8; break;
		case '$': ind=9; break;
		case '#': ind=10; break;
		case '_': ind=11; break;
		default: ind=-1; break;
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


void doOpenBullets(int level, int bullet, csc_bool_t isImageLeft)
{	
// Open the bullet level.
	if (bullet == '*')
		prt(frmGen,"%s", "\\begin{itemize}");
	else if (bullet == '+')
		prt(frmGen,"%s", "\\begin{enumerate}");
	else
		assert(bullet=='*' || bullet=='+');
 
// Make the font smaller if we are in imageLeft mode.
	if (isImageLeft)
		level += 1;
 
// Set the text size.
	if (level == 1)
		prt(frmGen,"\\Large");
	else if (level == 2)
		prt(frmGen,"\\large");
	else if (level >= 3)
		prt(frmGen,"\\normalsize");
 
// EOL.
	prt(frmGen,"\n");
}


void doCloseBullets(int bullet)
{	if (bullet == '*')
		prt(frmGen,"%s", "\\end{itemize}\n");
	else if (bullet == '+')
		prt(frmGen,"%s", "\\end{enumerate}\n");
	else
		assert(bullet=='*' || bullet=='+');
}


void doTextLine(char *line, escape_t *esc)
{	int isBul = csc_FALSE;
	int ch = *line;
	while (ch=='\t' || ch==' ' || ch=='*' || ch=='+')
	{	if (ch=='*' || ch=='+')
			isBul = csc_TRUE;
		ch = *++line;
	}
	if (isBul)
	{	csc_str_append(frmGen,"\\item ");
	}
	doEscLine(frmGen, line, esc);
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


void doOpenFrame(vidAssoc_t *va, FILE *fout, char *title, int slideNum)
{	
// Flush out.
	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
 
// Open the frame.
	// prt(frmGen,"\\begin{frame}\\LARGE\n\\frametitle{%s}\n", title);
    csc_str_assign(frmTitle, title);
 
// Slide-Video associations.
	vidAssoc_do(va, frmGen, title, slideNum);
}


void sendCloseFrame(FILE *fout, int slideNum)
{
// Output any pre frame.
	if (csc_str_length(frmPre) > 0)
	{	fprintf(fout, "{\n");
		csc_str_out(frmPre,fout);
	}
 
// The frame title.
	fprintf(fout, "%s", "\\begin{frame}\\LARGE\n\\frametitle{");
	if (isNoRef)
	{	isNoRef = csc_FALSE;
	}
	else if (isRefInTitle)
	{	fprintf( fout, "\\textcolor{black}{\\Large %s%d:} "
			   , refWord, slideNum
			   );
	}
	fprintf(fout, "%s}\n", csc_str_charr(frmTitle));
 
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


void doTopic(vidAssoc_t *va, FILE *fout, char *body, int slideNo)
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
	fprintf( fout, "%s", "\\begin{frame}\\LARGE\n");
	fprintf( fout, "\\frametitle{%s}\n", title);
 
// Video reference.
	vidAssoc_do(va, frmGen, title, slideNo);
	csc_str_out(frmGen,fout);
	csc_str_truncate(frmGen, 0);
 
// Frame body and frame.
	fprintf(fout, "\\centerline{\\huge %s}\n", body);
	fprintf(fout, "%s\n\n", "\\end{frame}");
 
// end the topic color.
	fprintf( fout, "}\n");
 
// Free resources.
	if (titleS)
		csc_str_free(titleS);
}


void sendCloseFile(FILE *fout)
{	fprintf(fout, "%s", "\\end{document}\n");
}


void work(vidAssoc_t *va, FILE *fin, FILE *fout)
{	char line[MaxLineLen];
	char *words[MaxWords];
	int nWords;
	char bulletStack[10];  // Bullet stack. 
	int level;
	int bullet;
	int slideNum = 0;
	csc_bool_t isImageLeft = csc_FALSE;
 
// Resources.
	frmPre = NULL;
	frmGen = NULL;
	frmTitle = NULL;
	frmPost = NULL;
 
// For storing output, until end of frame.
	frmPre = csc_str_new(NULL);   assert(frmPre);
	frmGen = csc_str_new(NULL);   assert(frmGen);
	frmTitle = csc_str_new(NULL);   assert(frmGen);
	frmPost = csc_str_new(NULL);   assert(frmPost);
 
	prt(frmGen, "%s",
"% NOTICE: Before you edit this file, know that this latex file was\n"
"% created using the following command:-\n"
"%   prompt$ quickbeam < ???.qb > ???.tex\n"
"% You need to edit the .qb file instead.\n"
"% quickbeam is available from https://github.com/drbraithw8/quickbeam.\n\n"
		);

// Escapes.
	escape_t escapesGlobal;
	escape_t escapesFrame;
	if (csc_TRUE)
	{	char *words[] = {"escOff", "all"};
		escape_setOnOff(&escapesGlobal, words, 2);
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
				}
				else
					complainQuit("Expected underline");
			}
			else if ((nWords = testAtLine(line,words)) > 0)
			{	if (csc_streq(words[0],"topic"))
				{	doTopic(va, fout, words[1], ++slideNum);
				}
	 			else if (csc_streq(words[0],"escOff") || csc_streq(words[0],"escOn"))
				{	escape_setOnOff(&escapesGlobal, words, nWords);
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
					doOpenFrame(va, fout, line, slideNum);
					isExpectUnderline = csc_TRUE;
				}
			}
		}
 
		else // Deal with inside frame.
		{
			if (isLineBlank(line))
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
			{  // It means we found @ line.
 
				if (csc_streq(words[0],"bgcolor"))
				{ // Background colour for this slide.
					doBgColor(words, nWords);
				}
	 			else if (csc_streq(words[0],"noref"))
				{	isNoRef = csc_TRUE;
				}
	 			else if (csc_streq(words[0],"image"))
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
 
				// Do the image.
					doImage(words, nWords);
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
	 			else if (csc_streq(words[0],"escOff") || csc_streq(words[0],"escOn"))
				{	escape_setOnOff(&escapesFrame, words, nWords);
				}
				else
				{
// 					{ // Assume we have a NASTY OLD backward compatability style image.
// 						char *reWords[3];
// 	 
// 					// Debugging.
// 						// fprintf(stderr, "words=");
// 						// for (int i=0; i<nWords; i++)
// 							// fprintf(stderr, "\"%s\" ", words[i]);
// 						// fprintf(stderr, "\n");
// 	 
// 					// Close each bullet level.
// 						while (bulletLevel > 0)
// 						{	doCloseBullets(bulletStack[--bulletLevel]);
// 						}
// 	 
// 					// Close imageLeft.
// 						if (isImageLeft || isColumn)
// 						{	prt(frmGen,"%s", "\\end{column}\n");
// 							prt(frmGen,"%s", "\\end{columns}\n");
// 							isImageLeft = csc_FALSE;
// 							isColumn = csc_FALSE;
// 						}
// 	 
// 					// Process the image.
// 						reWords[0] = "";
// 						reWords[1] = words[0];
// 						reWords[2] = words[1];
// 						doImage(reWords, 3);
// 					}
					complainQuit("unknown \"@\" directive");
				}
			}
			else	// It means that we found a line.
			{	bullet = getBulletLevel(line, &level);
				if (bullet != 0)  // Its a bulleted line.
				{
				// Adjust the bullet level of the line accordingly.
					while (bulletLevel > level)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
					if (bulletLevel == level)
					{  // Were good, so do nothing.
					}
					else if (bulletLevel == level-1)
					{	doOpenBullets(level, bullet, isImageLeft||isColumn);
						bulletStack[bulletLevel++] = bullet;
					}
					else  // bulletLevel < level-1.
					{	complainQuit("Opening too many levels of bullets.");
					}
				} // Bulleted line.
				doTextLine(line, &escapesFrame);
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
{	vidAssoc_runMode_t vMode = vidAssoc_noAssoc;
 
// Resources.
	FILE *fin=stdin;
	FILE *fout=stdout;
	vidAssoc_t *va = NULL;
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
		else if (*p=='-' && *(p+1)=='v' && *(p+3)=='\0')
		{
			if (*(p+2) == 'r')
			{	vMode = vidAssoc_markSlide;
				isRefInTitle = csc_TRUE;
			}
			else if (*(p+2)=='R' && iArg+1<argc)
			{
				vMode = vidAssoc_doLink;
				iArg++;
				qbvPath = argv[iArg];
			}
			else if (*(p+2)=='w' && iArg+1<argc)
			{	iArg++;
				refWord = argv[iArg];
			}
			else
			{	fprintf(stderr, "Error: Unknown option 1 \"%s\".\n", p);
				usage();
			}
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
 
// Create video associations object.
	if (vMode==vidAssoc_markSlide || vMode==vidAssoc_doLink)
		va = vidAssoc_new(vMode, refWord);
 
// Read slide association.
	if (vMode == vidAssoc_doLink)
	{
		errStr = csc_str_new(NULL);
		vidAssoc_readAssocs(va, qbvPath, errStr);
		if (csc_str_length(errStr) > 0)
		{	fprintf( stderr, "Error reading file \"%s\":\n%s\n"
				   , qbvPath, csc_str_charr(errStr)
				   );
			exit(1);
		}
	}
 
// Process the quickbeam.
	work(va, fin, fout);
 
// Release resources.
	if (errStr)
		csc_str_free(errStr);
	if (va)
		vidAssoc_free(va);
	if (fin != stdin)
		fclose(fin);
	if (fout != stdout)
		fclose(fout);
 
// Bye.
	exit(0);
}

