// Program Maturity = "Work in progress".
// Problems:-
// *	Loop structure is bad.  Things that happen first should have a
// 		separate loop on the line.
// *	Bugs?

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

typedef enum bool_e { FALSE=0, TRUE=1 } bool_t;

// char *dirBeamer = NULL;
int lineNo = 0;


void complainQuit(char *msg)
{	fprintf(stderr, "Error at line %d: %s !\n", lineNo, msg);
	exit(1);
}


static char *param_skip_white(char *p)
/*  Skips over ' ', '\t'. 
 */
{   char ch = *p;
    while (ch==' ' || ch=='\t')
        ch = *(++p);
    return(p);
}

static char *param_skip_word(char *p)
/*  Skips over all chars except ' ', '\t', '\0'.
 */
{   char ch = *p;
    while (ch!=' ' && ch!='\0' && ch!='\t')
        ch = *(++p);
    return(p);
}

int param_quote(char *argv[], char *line, int n)
{   int argc = 0;
    char ch;
    char *p = param_skip_white(line);
    while(*p!='\0' && argc<n)
    {   if (*p == '\"')
        {   argv[argc++] = line = ++p;
            ch = *p;
            while (ch!='\"' && ch!='\0')
            {   if (ch=='\\' && *(p+1)!='\0')
                    ch = *++p;
                *line++ = ch;
                ch = *++p;
            }
            *line = '\0';
            if (ch == '\"')
                p = param_skip_white(p+1);
        }
        else
        {   argv[argc++] = p;
            line = param_skip_word(p);
            p = param_skip_white(line);
            *line = '\0';
        }
    }
    return(argc);
}

int fgetline(FILE *fp, char *line, int max)
{   register int ch, i;
 
/*  Look at first char for EOF. */
    ch = getc(fp);
    if (ch == EOF)
        return -1;
 
/* Read in line */
    i=0;
    while (ch!='\n' && ch!=EOF && i<max)
    {   if (ch != '\r')
            line[i++] = ch;
        ch = getc(fp);
    }
    line[i] = '\0';
 
/* Skip any remainder of line */
    while (ch!='\n' && ch!=EOF)
    {   ch = getc(fp);
        i++;
    }
    return(i);
}

int isLineBlank(char *line)
{	int ch;
	while (ch=(*line++))
	{	if (ch!=' ' && ch!='\t')
			return FALSE;
	}
	return TRUE;
}


int isImage(char *line)
{	int ch;
 
// Skip spaces.
	while (ch=(*line++))
	{	if (ch!=' ' && ch!='\t')
			break;
	}
 
// Look at the character.
	return (ch == '@');
}


int isUnderline(char *line)
{	int ch;
	if (isLineBlank(line))
		return FALSE;
	while (ch=(*line++))
	{	if (ch!='-' && ch!=' ' && ch!='\t')
			return FALSE;
	}
	return TRUE;
}


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
			return FALSE;
	}
}	


// void sendFile(char *fname)
// {	FILE *fin = NULL;
// 	char filePath[256];
// 	sprintf(filePath, "%s/%s", dirBeamer, fname);
// 	fin = fopen(filePath, "r");
// 	if (fin == NULL)
// 	{	char buffer[256];
// 		sprintf(buffer, "Could not open file %s for read", filePath);
// 		complainQuit(buffer);
// 	}
// 	csc_xferBytes(fin, stdout);
// 	fclose(fin);
// }
// 
// void sendOpenFile(void)
// {	sendFile("openFile.tex");
// }

void sendCloseFile(void)
{	printf("%s", "\\end{document}\n");
}

void sendOpenBullets(int level, int bullet)
{	
// Open the bullet level.
	if (bullet == '*')
		printf("%s", "\\begin{itemize}");
	else if (bullet == '+')
		printf("%s", "\\begin{enumerate}");
	else
		assert(bullet=='*' || bullet=='+');

// Set the text size.
	if (level == 1)
		printf("\\Large");
	else if (level == 2)
		printf("\\large");
	else if (level == 3)
		printf("\\normalsize");

// EOL.
	printf("\n");
}

void sendCloseBullets(int bullet)
{	if (bullet == '*')
		printf("%s", "\\end{itemize}\n");
	else if (bullet == '+')
		printf("%s", "\\end{enumerate}\n");
	else
		assert(bullet=='*' || bullet=='+');
}

void sendOpenFrame(char *title)
{	printf("\\begin{frame}  \\LARGE\n"
		   "\\frametitle{%s}\n"
		   , title);
}

void sendCloseFrame(void)
{	printf("%s\n\n", "\\end{frame}");
}

void sendImage(char *line)
{	int ch = *line;
	char *words[3];
	int nWords;
 
// Go past the '@' (and any whitespace).
	while (ch=='\t' || ch==' ' || ch=='@')
		ch = *++line;
 
	nWords = param_quote(words, line, 3);
	if (nWords < 2)
		complainQuit("Incorrect args for image");
		
// Print to include the file.
printf(
	"\\begin{figure}\n"
	"\\includegraphics[scale=%s]{Images/%s}\n"
	"\\end{figure}\n"
	, words[1], words[0]
	);
}


void sendTextLine(char *line)
{	int isBul = FALSE;
	int ch = *line;
	while (ch=='\t' || ch==' ' || ch=='*' || ch=='+')
	{	if (ch=='*' || ch=='+')
			isBul = TRUE;
		ch = *++line;
	}
	if (isBul)
		printf("\\item %s\n", line);
	else
		printf("%s\n", line);
}


void removeComment(char *line)
{	char prevCh;
	char ch = ' ';
	char *lineP;
	for (lineP=line; prevCh=ch,ch=*lineP; lineP++)
	{	if (prevCh=='/' && ch=='/')
		{	*(lineP-1) = '\0';
			break;
		}
	}
}
	

int main(int argc, char **argv)
{	char line[256];
	char bulletStack[10];  // Bullet stack. 
	int level;
	int bullet;
 
	printf( "%s",
"% NOTICE: Before you edit this file, know that this latex file was\n"
"% created using the following command:-\n"
"%   prompt$ quickbeam < ???.qb > ???.tex\n"
"% You need to edit the .qb file instead.\n"
"% quickbeam is available from https://github.com/drbraithw8/quickbeam.\n\n"
		);
 
	int isInsideFrame = FALSE;
	int isExpectUnderline = FALSE;
	int bulletLevel = 0;
 
// Loop through lines of file.
	while (fgetline(stdin, line, 255) > -1)
	{	lineNo++;
 
	// Literal lines.
		if (line[0] == '#')
		{	printf("%s\n", line+1);
			continue;
		}
 
	// Remove any comment.
		removeComment(line);
		
	// Deal with outside of frame things.
		if (!isInsideFrame)
		{	if (isLineBlank(line))
				continue;
			if (getBulletLevel(line, &level))
				complainQuit("Expected Header line.");
	
		// Deal with a possible underline.
			if (isExpectUnderline)
			{	if (isUnderline(line))
				{	isExpectUnderline = FALSE;
					isInsideFrame = TRUE;
					bulletLevel = 0;
				}
				else
					complainQuit("Found unexpected underline");
			}
			else
			{	if (isUnderline(line))
					complainQuit("Unexpected underline");
				else
				{	sendOpenFrame(line);
					isExpectUnderline = TRUE;
				}
			}
		}
 
		else // Deal with inside frame.
		{	if (isLineBlank(line))
			{ // It means the frame is finished.
	 
			// Close each bullet level.
				while (bulletLevel > 0)
				{	sendCloseBullets(bulletStack[--bulletLevel]);
				}
	 
			// Close the frame.
				sendCloseFrame();
				isInsideFrame = FALSE;
			}
			else if (isImage(line))
			{  // It means we found an image.
	 
			// Close each bullet level.
				while (bulletLevel > 0)
				{	sendCloseBullets(bulletStack[--bulletLevel]);
				}
	 
			// Send the image.
				sendImage(line);
			}
			else	// It means that we found a line.
			{	bullet = getBulletLevel(line, &level);
				if (bullet != 0)  // Its a bulleted line.
				{
				// Adjust the bullet level of the line accordingly.
					while (bulletLevel > level)
					{	sendCloseBullets(bulletStack[--bulletLevel]);
					}
					if (bulletLevel == level)
					{  // Were good, so do nothing.
					}
					else if (bulletLevel == level-1)
					{	sendOpenBullets(level, bullet);
						bulletStack[bulletLevel++] = bullet;
					}
					else  // bulletLevel < level-1.
					{	complainQuit("Opening too many levels of bullets.");
					}
				} // Bulleted line.
				sendTextLine(line);
			} // Found text line.
		} // inside a frame.
	} // loop thru lines.
 
// We are finished. Close each bullet level.
	while (bulletLevel > 0)
	{	sendCloseBullets(bulletStack[--bulletLevel]);
	}
 
// Close the frame if needed.
	if (isInsideFrame)
		sendCloseFrame();
 
// Close the frame.
	sendCloseFile();
	return 0;
}
 

