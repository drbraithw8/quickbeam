// Author: Dr Stephen Braithwaite.
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "csc_std.h"
#include "csc_isvalid.h"


bool csc_isValid_hex(const char *word)
{   char ch;
    if (word == NULL)
        return false;
    ch = *word;
    if (ch == '\0')
        return false;
    while((ch = *(word++)) != '\0')
    {   if ( !(  (ch>='0' && ch<='9')
              || (ch>='a' && ch<='f')
              || (ch>='A' && ch<='F')
              )
           )
        {   return false;
        }
    }
    return true;
}


bool csc_isValid_int(const char *word)
{   char ch;
    if (word == NULL)
        return false;
    ch = *word;
    if (ch=='-')
        ch = *(++word);
    if (ch == '\0')
        return false;
    while((ch = *(word++)) != '\0')
        if (ch<'0' || ch>'9')
            return false;
    return true;
}


bool csc_isValidRange_int(const char *word, int min, int max, int *value)
{   int val;
    if (!csc_isValid_int(word))
        return false;
    val = atoi(word);
    if (val<min || val>max)
        return false;
    if (value != NULL)
        *value = val;
    return true;
}


bool csc_isValid_float(const char *str)
{   int has_point, has_e, has_num;
    char ch;
    if (str == NULL)
        return false;
    ch = *str;
    if (ch=='-' || ch=='+')
        str++;
    has_point = false;
    has_e = false;
    has_num = false;
    while ((ch=*(str++)) != '\0')   switch(ch)
    {   case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': 
            has_num = true;
            break;
        case 'e': case 'E':
            if (has_e || !has_num)
                return false;
            if (*str=='-' || *str=='+')
                str++;
            has_e = true;
            has_num = false;
            break;
        case '.':
            if (has_point || has_e)
                return false;
            has_point = true;
            break;
        default:
            return false;
    }
    return has_num;
}


bool csc_isValidRange_float(const char *word, double min, double max, double *value)
{   double val;
    if (!csc_isValid_float(word))
        return false;
    val = atof(word);
    if (val<min || val>max)
        return false;
    if (value != NULL)
        *value = val;
    return true;
}


bool csc_isValid_ipV4(const char *str)
{   struct sockaddr_in sa;
    if (str == NULL)
        return false;
    return inet_pton(AF_INET, str, &(sa.sin_addr)) != 0;
}

bool csc_isValid_ipV6(const char *str)
{   struct sockaddr_in6 sa;
    if (str == NULL)
        return false;
    return inet_pton(AF_INET6, str, &(sa.sin6_addr)) != 0;
}


bool csc_isValid_domain(const char *str)
{   
// Reject the empty string.
    if (str == NULL)
        return false;
    if (csc_streq(str,""))
        return false;
    int sLen=strlen(str);
 
// Look at the ends of the domain name.
    if ( str[0] == '.'
       || str[sLen-1] == '.'
       || sLen > 253
       ) 
    {   return false;
    }
 
// Look at each character in turn.
    int segLen = 0;
    for(int i=0; i<sLen; i++)
    {   if (str[i] == '.')
        {   if (segLen == 0)
                return false;
            segLen=0;
        }
        else if (  isalnum(str[i])
                || str[i]=='-' && segLen!=0 && i+1<sLen && str[i+1]!='.'
                )
        {   if (++segLen > 63)
                return false;
        }
        else
            return false; //invalid char...
    }
 
// There should be at least two segments.
    if (segLen == sLen)
        return false;
 
    return true;
}


bool csc_isValid_decentRelPath(const char *str)
{   bool result = true;
	bool isDotsOnly = true;
    const char *p;
    int segLen, ch;
 
    if (str == NULL)
    {   result = false;
    }
    else
    { // Test each char of 'str' in turn.
        p = str;
        segLen = 0;
        
        while (true)
        {	ch = *p++;
 
            if (isalnum(ch) || ch=='_' || ch==',')
            {   segLen++;
				isDotsOnly = false;
            }
            else if (ch=='/')
            {	if (isDotsOnly)
				{ // Path segment consisting of zero or more dots not allowed.
                   result = false;
                    break;
                }
                else
                {	segLen = 0;
					isDotsOnly = true;
				}
            }
			else if (ch == '\0')
			{ // Path segment consisting of zero or more dots not allowed.
                if (isDotsOnly)
                { // Path segment consisting of zero or more dots not allowed.
					result = false;
                    break;
                }
                else
                	break;
            }
            else if (ch == '.')
            {	segLen++;
            }
            else if (ch == '-')
            { // Path segements beginning with '-' are not allowed.
                if (segLen == 0)
                {   result = false;
                    break;
                }
				isDotsOnly = false;
            	segLen++;
            }
			else if (ch == ' ')
			{ // A segment should not begin or end with a space or have consecutive.
				if (segLen==0 || *p==' ' || *p=='\0' || *p=='/')
				{   result = false;
					break;
				}
            	segLen++;
			}
            else
            { // General punctuation characters are not allowed.
				result = false;
                break;
            }
        }
 
    // Path should not be empty or end with a slash or consist only of dots.
        if (segLen == 0)
            result = false;
    }
 
// Its all good if we got this far.
    return result;
}


bool csc_isValid_decentPath(const char *str)
{   bool result;
    if (str == NULL)
        result = false;
    else
    {   if (*str == '/')
            str++;
        result = csc_isValid_decentRelPath(str);
    }
    return result;
}


bool csc_isValid_decentAbsPath(const char *str)
{   bool result;
    if (str == NULL)
        result = false;
    else if (*str != '/')
        result = false;
    else
        result = csc_isValid_decentRelPath(str+1);
    return result;
}

