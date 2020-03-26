// Author: Dr Stephen Braithwaite.
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

// *	Uses very lazy code.  Quick and nasty does it.
// *	Works well.  Does what it was meant to do.  There are no plans to change it.
// *	Requires library CscNetlib.   https://github.com/drbraithw8/CscNetlib


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <CscNetLib/std.h>
#include <CscNetLib/cstr.h>
#include <CscNetLib/isvalid.h>

#define MaxLineLen 255
#define MaxWords 25
 
//-------- Global Variables  ----------

int lineNo = 0;

// Stores output while processing frame.
csc_str_t *preFrm;
csc_str_t *frm;
csc_str_t *postFrm;


//-------- Miscellaneous ---------------

void complainQuit(char *msg)
{	fprintf(stderr, "Error at line %d: %s !\n", lineNo, msg);
	exit(1);
}


int isLineBlank(char *line)
{	int ch;
	while (ch=(*line++))
	{	if (ch!=' ' && ch!='\t')
			return csc_FALSE;
	}
	return csc_TRUE;
}



int isUnderline(char *line)
{	int ch;
	if (isLineBlank(line))
		return csc_FALSE;
	while (ch=(*line++))
	{	if (ch!='-' && ch!=' ' && ch!='\t')
			return csc_FALSE;
	}
	return csc_TRUE;
}


// void removeComment(char *line)
// {	char prevCh;
// 	char ch = ' ';
// 	char *lineP;
// 	for (lineP=line; prevCh=ch,ch=*lineP; lineP++)
// 	{	if (prevCh=='/' && ch=='/')
// 		{	*(lineP-1) = '\0';
// 			break;
// 		}
// 	}
// }
	

// void removeComment(char *line)
// {	if (line[0]=='/' && line[1]=='/')
// 		line[0] = '\0';
// }
	

int getBulletLevel(char *line, int *level)
// if bullet char is '*' or '+"
// then returns bullet char and sets *'level'.
// else returns 0
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


//-------------- Prepare stuff to send ---------

#define prt csc_str_append_f

void doOpenFrame(char *title)
{	csc_str_out(frm,stdout);
	csc_str_truncate(frm, 0);
	prt(frm,"\\begin{frame}  \\LARGE\n"
		   "\\frametitle{%s}\n"
		   , title);
}


void doOpenBullets(int level, int bullet, csc_bool_t isImageLeft)
{	
// Open the bullet level.
	if (bullet == '*')
		prt(frm,"%s", "\\begin{itemize}");
	else if (bullet == '+')
		prt(frm,"%s", "\\begin{enumerate}");
	else
		assert(bullet=='*' || bullet=='+');

// Make the font smaller if we are in imageLeft mode.
	if (isImageLeft)
		level += 1;

// Set the text size.
	if (level == 1)
		prt(frm,"\\Large");
	else if (level == 2)
		prt(frm,"\\large");
	else if (level >= 3)
		prt(frm,"\\normalsize");
 
// EOL.
	prt(frm,"\n");
}


void doCloseBullets(int bullet)
{	if (bullet == '*')
		prt(frm,"%s", "\\end{itemize}\n");
	else if (bullet == '+')
		prt(frm,"%s", "\\end{enumerate}\n");
	else
		assert(bullet=='*' || bullet=='+');
}


void doTextLine(char *line)
{	int isBul = csc_FALSE;
	int ch = *line;
	while (ch=='\t' || ch==' ' || ch=='*' || ch=='+')
	{	if (ch=='*' || ch=='+')
			isBul = csc_TRUE;
		ch = *++line;
	}
	if (isBul)
		prt(frm,"\\item %s\n", line);
	else
		prt(frm,"%s\n", line);
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
	prt(frm,
		"\\begin{columns}\n"
		"\\begin{column}{%0.3f\\textwidth}\n"
		"\\begin{figure}\n"
		"\\includegraphics[scale=%0.3f]{Images/%s}\n"
		"\\end{figure}\n"
		"\\end{column}\n"
		"\\begin{column}{%0.3f\\textwidth}\n"
		, colWidth, imgWidth, words[2], (1-colWidth-0.03)
		);
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
	prt(frm,
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
	prt(preFrm, "\\setbeamercolor{background canvas}{bg=%s}\n", words[1]);
}
 	


//------------- Send --------

void sendCloseFrame(void)
{
// End the frame.
	prt(frm,"%s\n", "\\end{frame}");
 
// Output any pre frame.
	if (csc_str_length(preFrm) > 0)
	{	printf("{\n");
		csc_str_out(preFrm,stdout);
	}
 
// Output the frame.
	csc_str_out(frm,stdout); csc_str_truncate(frm, 0);
 
// Output any post frame.
	if (csc_str_length(preFrm) > 0)
	{	csc_str_out(postFrm,stdout);
		csc_str_truncate(preFrm, 0);
		csc_str_truncate(postFrm, 0);
		printf("}\n");
	}
 
// A blank line for beauty.
	printf("\n");
}

void sendCloseFile(void)
{	printf("%s", "\\end{document}\n");
}


//------------- Main --------

int main(int argc, char **argv)
{	char line[MaxLineLen];
	char bulletStack[10];  // Bullet stack. 
	int level;
	int bullet;
	csc_bool_t isImageLeft = csc_FALSE;
 
// Resources.
	preFrm = NULL;
	frm = NULL;
	postFrm = NULL;
 
// For storing output, until end of frame.
	preFrm = csc_str_new(NULL);   assert(preFrm);
	frm = csc_str_new(NULL);   assert(frm);
	postFrm = csc_str_new(NULL);   assert(postFrm);
 
	prt(frm, "%s",
"% NOTICE: Before you edit this file, know that this latex file was\n"
"% created using the following command:-\n"
"%   prompt$ quickbeam < ???.qb > ???.tex\n"
"% You need to edit the .qb file instead.\n"
"% quickbeam is available from https://github.com/drbraithw8/quickbeam.\n\n"
		);
 
	int isInsideFrame = csc_FALSE;
	int isExpectUnderline = csc_FALSE;
	int bulletLevel = 0;
 
// Loop through lines of file.
	while (csc_fgetline(stdin, line, MaxLineLen) > -1)
	{	lineNo++;
 
	// Comment lines.
		if (line[0]=='/' && line[1]=='/')
			continue;
 
	// Literal lines.
		if (line[0] == '#')
		{	prt(frm,"%s\n", line+1);
			continue;
		}
 
	// Deal with outside of frame things.
		if (!isInsideFrame)
		{	if (isLineBlank(line))
				continue;
			if (getBulletLevel(line, &level))
				complainQuit("Expected Header line.");
	
		// Deal with a possible underline.
			if (isExpectUnderline)
			{	if (isUnderline(line))
				{	isExpectUnderline = csc_FALSE;
					isInsideFrame = csc_TRUE;
					bulletLevel = 0;
				}
				else
					complainQuit("Found unexpected underline");
			}
			else
			{	if (isUnderline(line))
					complainQuit("Unexpected underline");
				else
				{	doOpenFrame(line);
					isExpectUnderline = csc_TRUE;
				}
			}
		}
 
		else // Deal with inside frame.
		{	int nWords;
			char *words[MaxWords];
 
			if (isLineBlank(line))
			{ // It means the frame is finished.
	 
			// Close each bullet level.
				while (bulletLevel > 0)
				{	doCloseBullets(bulletStack[--bulletLevel]);
				}
 
			// Close imageLeft.
				if (isImageLeft)
				{	prt(frm,"%s", "\\end{column}\n");
					prt(frm,"%s", "\\end{columns}\n");
					isImageLeft = csc_FALSE;
				}
	 
			// Close the frame.
				sendCloseFrame();
				isInsideFrame = csc_FALSE;
			}
			else if ((nWords = testAtLine(line,words)) > 0)
			{  // It means we found @ line.
 
				if (csc_streq(words[0],"bgcolor"))
				{ // Background colour for this slide.
					doBgColor(words, nWords);
				}
	 			else if (csc_streq(words[0],"image"))
				{  // Process image.
 
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
 
				// Close imageLeft.
					if (isImageLeft)
					{	prt(frm,"%s", "\\end{column}\n");
						prt(frm,"%s", "\\end{columns}\n");
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
					if (isImageLeft)
					{	prt(frm,"%s", "\\end{column}\n");
						prt(frm,"%s", "\\end{columns}\n");
						isImageLeft = csc_FALSE;
					}
		
				// Do the image.
					doImageLeft(words, nWords);
					isImageLeft = csc_TRUE;
				}
				else
				{ // Assume we have a NASTY OLD backward compatability style image.
					char *reWords[3];
 
				// Debugging.
					// fprintf(stderr, "words=");
					// for (int i=0; i<nWords; i++)
						// fprintf(stderr, "\"%s\" ", words[i]);
					// fprintf(stderr, "\n");
 
				// Close each bullet level.
					while (bulletLevel > 0)
					{	doCloseBullets(bulletStack[--bulletLevel]);
					}
 
				// Close imageLeft.
					if (isImageLeft)
					{	prt(frm,"%s", "\\end{column}\n");
						prt(frm,"%s", "\\end{columns}\n");
						isImageLeft = csc_FALSE;
					}
 
				// Process the image.
					reWords[0] = "";
					reWords[1] = words[0];
					reWords[2] = words[1];
					doImage(reWords, 3);
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
					{	doOpenBullets(level, bullet, isImageLeft);
						bulletStack[bulletLevel++] = bullet;
					}
					else  // bulletLevel < level-1.
					{	complainQuit("Opening too many levels of bullets.");
					}
				} // Bulleted line.
				doTextLine(line);
			} // Found text line.
		} // inside a frame.
	} // loop thru lines.
 
// We are finished. Close each bullet level.
	while (bulletLevel > 0)
	{	doCloseBullets(bulletStack[--bulletLevel]);
	}

// We are finished. Close imageLeft.
	if (isImageLeft)
	{	prt(frm,"%s", "\\end{column}\n");
		prt(frm,"%s", "\\end{columns}\n");
		isImageLeft = csc_FALSE;
	}
		
// Close the frame if needed.
	if (isInsideFrame)
	{	sendCloseFrame();
	}
 
// Close the frame.
	sendCloseFile();

// Free resources.
	csc_str_free(preFrm);
	csc_str_free(frm);
	csc_str_free(postFrm);
	return 0;
}
 

