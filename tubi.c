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

#include "escapes.h"
#include "tubi.h"


// -------- Error handling -------------
static int lineNo = 0;

static void complainQuit(char *msg)
{	fprintf(stderr, "Error at line %d: %s !\n", lineNo, msg);
	exit(1);
}


escape_t escapesGlobal;
escape_t escapesFrame;
escape_t escapesVerbatim;


static void doEscLine(csc_str_t *out, char *line, escape_t *esc)
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


// --------- tubiOon ---------------

const char *tubi_chars = "tubi";

typedef struct 
{	csc_bool_t oon[tubiE_n];
} tubiOon_t;


// --------- tubiTxt ---------------

typedef enum
{	tubiTE_txt,
	tubiTE_eqn
} tubiTE;
	

typedef struct
{	tubiTE nodeType;
	char *txt;
	tubiOon_t oon;
	tubiE changes[tubiE_n];
	int nChanges;
	int iChange;
} tubiText_t;


static tubiText_t *tubiText_new( tubiTE nodeType
						, const char *txt
						, tubiOon_t *oon
						, csc_list_t *prev
						)
{	tubiText_t *tubiText = csc_allocOne(tubiText_t);
	tubiText->txt = csc_allocStr(txt);
	tubiText->oon = *oon;
	tubiText->nodeType = nodeType;
 
// Previous node.
	tubiText_t *tPrev;
	if (prev == NULL)
		tPrev = NULL;
	else
		tPrev = prev->data;
 
// What are the changes.
	tubiText->iChange = 0;
	tubiText->nChanges = 0;
	if (tPrev == NULL)
	{ // All turned on attr are changes. 
		for (int i=0; i<tubiE_n; i++)
		{	if (oon->oon[i])
				tubiText->changes[tubiText->nChanges++] = i;
		}
	}
	else
	{ // Those turned on since prev are changes.
		for (int i=0; i<tubiE_n; i++)
		{	if (oon->oon[i] > tPrev->oon.oon[i])
				tubiText->changes[tubiText->nChanges++] = i;
		}
	}
 
// Done.
	return tubiText;
}


static void tubiText_free(tubiText_t *tubiText)
{	free(tubiText->txt);
	free(tubiText);
}
	

// --------- tubi ---------------

char *tubi_expansions[] = { "texttt" , "underline" , "textbf" , "textit" };

typedef struct tubi_t 
{	csc_list_t *nodes;
	tubiOon_t oon;
	csc_str_t *txt;
} tubi_t;


tubi_t *tubi_new()
{	tubi_t *tubi = csc_allocOne(tubi_t);
	tubi->nodes = NULL;
	bzero(&tubi->oon, sizeof(tubi->oon));
	tubi->txt = csc_str_new(NULL);
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


static void tubi_addTubi(tubi_t *tubi, tubiE attr, csc_bool_t isOn)
{	
// Only do stuff if the attributes have actually changed.
	if (tubi->oon.oon[attr] != isOn)
	{	
	// Any text for old oon needs to be pushed into the list.
		if (csc_str_length(tubi->txt) > 0)
		{	tubiText_t *tt = tubiText_new( tubiTE_txt
										 , csc_str_charr(tubi->txt)
										 , &tubi->oon
										 , tubi->nodes
										 );
			csc_list_add(&tubi->nodes, tt);
			csc_str_truncate(tubi->txt, 0);
		}
 
	// Make the change to the text.
		tubi->oon.oon[attr] = isOn;
	}
}


static void tubi_addEqn(tubi_t *tubi, const char *txt)
{
// Any text for old oon needs to be pushed into the list.
	if (csc_str_length(tubi->txt) > 0)
	{	tubiText_t *tt = tubiText_new( tubiTE_txt
									 , csc_str_charr(tubi->txt)
									 , &tubi->oon
									 , tubi->nodes
									 );
		csc_list_add(&tubi->nodes, tt);
		csc_str_truncate(tubi->txt, 0);
	}
 
// Add the equation node.
	tubiText_t *tt = tubiText_new( tubiTE_eqn
								 , txt
								 , &tubi->oon
								 , tubi->nodes
								 );
	csc_list_add(&tubi->nodes, tt);
}


static void tubi_addText(tubi_t *tubi, const char *txt)
{	const char *p = txt;
 
// // Discard leading spaces.
// 	if (csc_str_length(tubi->txt) == 0)
// 	{	int ch = *p;
// 		while (isspace(ch))
// 		{	ch = *++p;
// 		}
// 	}
 
// Append.
    csc_str_append(tubi->txt, p);
}


static void tubi_textDone(tubi_t *tubi)
{
// Add any left over text.
	if (csc_str_length(tubi->txt) > 0)
	{	tubiText_t *tt = tubiText_new( tubiTE_txt
									 , csc_str_charr(tubi->txt)
									 , &tubi->oon
									 , tubi->nodes
									 );
		csc_list_add(&tubi->nodes, tt);
		csc_str_truncate(tubi->txt, 0);
	}
 
// Reverse the list.
	csc_list_rvrse(&tubi->nodes);
 
// Set the change orders.
	tubiText_t *changed[tubiE_n];
	bzero(&changed, sizeof(changed));
 
// Current state.
	tubiOon_t oon;
	bzero(&oon, sizeof(oon));
 
// For each tubiText node.
	for (csc_list_t *p=tubi->nodes; p!=NULL; p=p->next)
	{	tubiText_t *tt = p->data;
 
	// Point changed to the changes.
		for (int i=0; i < tt->nChanges; i++)
			changed[tt->changes[i]] = tt;
		tt->iChange = tt->nChanges;
 
	// For each attr that was reset.
		for (int i=0; i<tubiE_n; i++)
		{	if (tt->oon.oon[i] < oon.oon[i])
			{	tubiText_t *ptt = changed[i];
				assert(ptt != NULL);
 
			// Move the reset attr to the end of the changes array.
				int iChange = --ptt->iChange;
				for (int j=0; j<iChange; j++)
				{	if (ptt->changes[j] == i)
					{	ptt->changes[j] = ptt->changes[iChange];
						ptt->changes[iChange] = i;
					}
				}
 
			// I think we dont need to tidy that.
			// If we reset, then we must have set.
			// e.g. We never checked changed[i] in the first place.
				// changed[i] = NULL;
			}
		}
	
	// The oon of the previous node.
		oon = tt->oon;
	}
 
// We dont need to order the left over changed, since they are at the
// beginning of their tubiText node because the others were moved to the end.
}


static void tubi_parseEqn(tubi_t *tubi, const char **pp)
{	const char *p = *pp;
	int ch = *p;  assert(ch == 'E');
 
// Resources.
	csc_str_t *cstr = csc_str_new(NULL);
 
// Read the equation.
	for (;;)
	{	ch = *++p;
 
		if (ch == '@')
		{	ch = *++p;
			if (ch == '@')
			{	csc_str_append_ch(cstr, ch);
			}
			else if (ch == 'e')
			{	break;
			}
			else
			{	complainQuit("Unrecognised \"@\" escape within equation");
			}
		}
		else if (ch == '\0')
		{	complainQuit("Unterminated equation");
		}
		else
		{	csc_str_append_ch(cstr, ch);
		}
	}
			
// Add equation node to the tubi.
	tubi_addEqn(tubi, csc_str_charr(cstr));
 
// Free resources.
	csc_str_free(cstr);
 
// Assign the string pointer.
	*pp = p;
}


static void tubi_parseLineNum(csc_str_t *cstr, const char **pp)
{	const char *p = *pp;
	int ch = *p;  assert(ch == 'L');
 
// Get first character. Must be a digit.
	ch = *++p;
	if (!isdigit(ch))
	{	complainQuit("Missing line number in @L");
	}
 
// Read in line number.
	int lNum = 0;
	while (isdigit(ch))
	{	lNum = lNum * 10 + ch - '0';
		ch = *++p;
	}
 
// Number should have been terminated by a space.
	if (ch != ' ')
	{	complainQuit("Bad termination of line number in @L");
	}
 
// Set new line number.
	lineNo = lNum;
 
// Bye.
	*pp = p;
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
			else if (ch == 'E')
			{	if (csc_str_length(cstr) > 0)
				{	tubi_addText(tubi, csc_str_charr(cstr));
					csc_str_truncate(cstr, 0);
				}
				tubi_parseEqn(tubi, &p);
			}
			else if (ch == 'L')
			{	tubi_parseLineNum(cstr, &p);
			}
			else
			{	if (csc_str_length(cstr) > 0)
				{	tubi_addText(tubi, csc_str_charr(cstr));
					csc_str_truncate(cstr, 0);
				}
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
						complainQuit("Unknown @ escape");
				}
			}
		}
		ch = *++p;
	}
 
// Left over characters.
	if (csc_str_length(cstr) > 0)
	{	tubi_addText(tubi, csc_str_charr(cstr));
	}
 
// Free resources.
	csc_str_free(cstr);
 
// We are finished.
	tubi_textDone(tubi);
}


void tubi_send(tubi_t *tubi, csc_str_t *out, escape_t *esc)
{	
// The oon.
	tubiOon_t oon;
	bzero(&oon, sizeof(oon));
 
// Tubi Stacks
  	tubiE stack[tubiE_n];
	int stackSiz = 0;
  	tubiE tempStack[tubiE_n];
	int tempStackSiz = 0;
 
// The output.
	for (csc_list_t *p=tubi->nodes; p!=NULL; p=p->next)
	{	tubiText_t *tt = p->data;
 
		if (tt->nodeType == tubiTE_eqn)
		{	csc_str_append_f(out, "\\(%s\\)", tt->txt);
		}
		else if (tt->nodeType == tubiTE_txt)
		{	
		// Close as needed to get out of each attr.
			for (int i=0; i<tubiE_n; i++)
			{	while (tt->oon.oon[i] < oon.oon[i])
				{	tubiE attr = stack[--stackSiz];
					oon.oon[attr] = csc_FALSE;
					csc_str_append_ch(out, '}');
				}
			}
	 
		// Open any sets (changes) in order.
			for (int i=0; i < tt->nChanges; i++)
			{	tubiE iAttr = tt->changes[i];
				stack[stackSiz++] = iAttr;
				oon.oon[iAttr] = csc_TRUE;
				csc_str_append_f(out, "\\%s{", tubi_expansions[iAttr]);
			}
	 
		// Open any to get back into each attr not in changes.
			for (int i=0; i<tubiE_n; i++)
			{	if (tt->oon.oon[i] > oon.oon[i])
				{	stack[stackSiz++] = i;
					oon.oon[i] = csc_TRUE;
					csc_str_append_f(out, "\\%s{", tubi_expansions[i]);
				}
			}
	 
		// Print the text.
			doEscLine(out, tt->txt, esc);
	 
		// Next oon.
			oon = tt->oon;
		}
		else
		{	assert(csc_FALSE);
		}
	}
 
// Close any left open.
	for (int i=0; i<tubiE_n; i++)
	{	if (oon.oon[i])
			csc_str_append_ch(out, '}');
	}
 
// Final line.
	csc_str_append_ch(out, '\n');
}


// ------------------ Testing ---------------
 
// void tubi_show(tubi_t *tubi, FILE *fout)
// {	
// // Print all nodes.
// 	for (csc_list_t *p=tubi->nodes; p!=NULL; p=p->next)
// 	{	tubiText_t *tt = p->data;
//  
// 	// Print the type.
// 		if (tt->nodeType == tubiTE_txt)
// 			fprintf(fout, "T:");
// 		else if (tt->nodeType == tubiTE_eqn)
// 			fprintf(fout, "E:");
// 		else
// 			assert(csc_FALSE);
//  
// 	// Print the attributes.
// 		for (int i=0; i<tubiE_n; i++)
// 		{	if (tt->oon.oon[i])
// 			{	fputc(toupper(tubi_chars[i]), fout);
// 			}
// 			else
// 			{	fputc(tubi_chars[i], fout);
// 			}
// 		}
// 		fputc(':', fout);
//  
// 	// Print the changes.
// 		for (int i=0; i < tt->nChanges; i++)
// 		{	fputc(tubi_chars[tt->changes[i]], fout);
// 		}
// 		fputc(':', fout);
//  
// 	// Print the text.
// 		fprintf(fout, "\"%s\"\n", tt->txt);
// 	}
//  
// // Output script.
//   // The oon.
// 	tubiOon_t oon;
// 	bzero(&oon, sizeof(oon));
//  
//   // Tubi Stacks
//   	tubiE stack[tubiE_n];
// 	int stackSiz = 0;
//   	tubiE tempStack[tubiE_n];
// 	int tempStackSiz = 0;
//  
//   // The output.
// 	printf("%s\n", ".......");
// 	for (csc_list_t *p=tubi->nodes; p!=NULL; p=p->next)
// 	{	tubiText_t *tt = p->data;
//  
// 		if (tt->nodeType == tubiTE_eqn)
// 		{	fprintf(fout, "@E{%s}", tt->txt);
// 		}
// 		else if (tt->nodeType == tubiTE_txt)
// 		{	
// 		// Close as needed to get out of each attr.
// 			for (int i=0; i<tubiE_n; i++)
// 			{	while (tt->oon.oon[i] < oon.oon[i])
// 				{	tubiE attr = stack[--stackSiz];
// 					oon.oon[attr] = csc_FALSE;
// 					fprintf(fout, "}");
// 				}
// 			}
// 	 
// 		// Open any sets (changes) in order.
// 			for (int i=0; i < tt->nChanges; i++)
// 			{	tubiE iAttr = tt->changes[i];
// 				stack[stackSiz++] = iAttr;
// 				oon.oon[iAttr] = csc_TRUE;
// 				fprintf(fout, "@%c{", toupper(tubi_chars[iAttr]));
// 			}
// 	 
// 		// Open any to get back into each attr not in changes.
// 			for (int i=0; i<tubiE_n; i++)
// 			{	if (tt->oon.oon[i] > oon.oon[i])
// 				{	stack[stackSiz++] = i;
// 					oon.oon[i] = csc_TRUE;
// 					fprintf(fout, "@%c{", toupper(tubi_chars[i]));
// 				}
// 			}
// 	 
// 		// Print the text.
// 			fprintf(fout, "%s", tt->txt);
// 	 
// 		// Next oon.
// 			oon = tt->oon;
// 		}
// 		else
// 		{	assert(csc_FALSE);
// 		}
// 	}
//  
// // Close any left open.
// 	for (int i=0; i<tubiE_n; i++)
// 	{	if (oon.oon[i])
// 			fprintf(fout, "}");
// 	}
//  
// // Final line.
// 	fprintf(fout, "\n");
// }


// int testShow(char *str)
// {	csc_str_t *out = csc_str_new(NULL);
// 	tubi_t *tubi = tubi_new();
// 	tubi_parse(tubi, str);
// 	tubi_send(tubi, out, &escapesFrame);
// 	printf("\"%s\"\n\"%s\"\n\n", str, csc_str_charr(out));
// 	tubi_free(tubi);
// 	csc_str_free(out);
// }


// int main(int argc, char **argv)
// {	const char *words[] = { "escOn", "all" }; 
// 	escape_setOnOff(&escapesFrame, words, csc_dim(words));
// 	testShow("");
// 	testShow("plain > text");
// 	testShow("text with < a @Bbold@b word");
// 	testShow("text with ~ an @I italics @i word");
// 	testShow("text with ^ an @I italics @i word and an @U underlined @u word");
// 	testShow("text with { a @Bbold@b word and a @Ttypewriter text@t word");
// 	testShow("@T@U@B@IAll text } with all attributes@t@u@b@i");
// 	testShow("One @T@U@B@Iwo&rd@t@u@b@i with all attributes");
// 	testShow("plain @Ttt @Utu@t @Bub @Iubi @ubi@b ii@i plain");
// 	testShow("plain @Iii @Uiu@i @Bub @Tubt @ubt@b tt@i plain");
// 	testShow("plain @B@T@Iibt@tib@ib");
// 	testShow("plain @B@T@Iibt@tib@ib@bplain");
// 	testShow("plain @B@T@Iibt@tib@bb@iplain");
// 	testShow("@Uu @B@T@Iibtu@tibu@bui@iu");
// 	testShow("plain@U@Bub@u@b@T@Iit@t@iplain");
// 	testShow("@Ttt@Utu@Btub@utb@tbb");
// 	testShow("@Ttt@Utu@Btub@utb@btt");
// 	testShow("text with an @@");
// 	testShow("text @L23 with % an @@ sdkjf");
// 	testShow("text with $ an @@ and @Bbold@b sdkjf");
// 	testShow("text with # an @Eeqn@e");
// 	testShow("text with _ an @Eeqn@@eqn@e");
// 	testShow("text with an @I italics @i word, @Eeqn@e and an @U underlined @u word");
// 	testShow("@Bbold with @Eeqn@e inside");
// }


// // Test by typing test data into console.
// int main(int argc, char **argv)
// {	
// // Resources.
// 	csc_str_t *cstr = csc_str_new(NULL);
//  
// 	while (csc_str_getline(cstr, stdin) != -1)
// 	{	printf("\"%s\"\n", csc_str_charr(cstr));
// 		tubi_t *tubi = tubi_new();
// 		tubi_parse(tubi, csc_str_charr(cstr));
// 		tubi_show(tubi, stdout);
// 		tubi_free(tubi);
// 	}
//  
// // Free resources.
// 	csc_str_free(cstr);
//  
// // Testing for memory leaks.
// 	exit(0);
// }

