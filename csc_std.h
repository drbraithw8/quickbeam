// Author: Dr Stephen Braithwaite.
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

#ifndef csc_STD_H
#define csc_STD_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

void bzero(void *s, size_t n);

#define csc_versionStr "1.15.2"

#define MinPortNo 1
#define MaxPortNo 65535
#define MaxPortNoStrSize 5

#define csc_streq(a,b)  (!strcmp((a),(b)))
#define csc_strieq(a,b)  (!strcasecmp((a),(b)))

#define csc_isalu(ch) (isalpha(ch) || (ch)=='_')
#define csc_ungetc(ch,fp)  ((ch)==EOF?(ch):ungetc(ch,fp))

#define csc_CKCK fprintf(csc_stderr, \
                "Got to line %d in file %s !\n", __LINE__, __FILE__)

typedef unsigned int csc_uint;
typedef unsigned long csc_ulong;
typedef unsigned char csc_uchar;
typedef unsigned short csc_ushort;

// Bit manipulations on array of bits
// implemented as array of csc_uchar, a.
#define  csc_bm_set(a,i) ((a)[(i)>>3] |= 1<<((i)&7))
#define  csc_bm_clr(a,i) ((a)[(i)>>3] &= ~(1<<((i)&7)))
#define  csc_bm_isSet(a,i)  ((a)[(i)>>3] & (1<<((i)&7)))
#define  csc_bm_isClr(a,i)  (((a)[(i)>>3] & (1<<((i)&7))) == 0)
#define  csc_bm_nBytes(n)  ((n+7)>>3)


#define csc_dim(array)      (sizeof(array) / sizeof(array[0]))
#define csc_fdim(typ,arr) (sizeof(((typ*)NULL)->arr)/sizeof(((typ*)NULL)->arr[0]))
#define csc_fsizeof(type,field)     (sizeof(((type *)NULL)->field))

#define csc_max(a,b) ((a>b)?(a):(b))
#define csc_min(a,b) ((a<b)?(a):(b))

extern FILE *csc_errOut;
void csc_setErrOut(const char *pathErrOut);
void csc_assertFail(const char *fname, int lineNo, const char *expr);
#define csc_stderr (csc_errOut?csc_errOut:stderr)
#define csc_assert(a)  ( !(a) ? ( \
   csc_assertFail(__FILE__, __LINE__, #a) , 0) : 0)  


// This function copies 'source' to 'dest'.  If, however, 'source'
// has more than 'n' characters, only 'n' characters will be copied and
// a null byte will be appended to 'dest' after the last character.
// Note: 'n' must not be negative or else disaster.
void csc_strncpy(char *dest, const char *source, int n);


// This function copies 'source' onto the end of  'dest'.  It will only
// copy into 'dest' what 'dest' has room for.  'dest' has room for 'n'
// characters plus 1 for the null byte.
void csc_strncat(char *dest, char *source, int n);


// Overwrites string 'str' with one that has had whitespace
// removed from the beginning and end.
void csc_trim(char *str);


// Reads a line from the stream 'fp' into the array 'line'.  The
// terminating newline is not included into 'line', but it is consumed.
// 
// If the line is longer than 'max' characters, then only the
// first 'max' chars of the line will be placed into 'line'.  The
// remainder of the line will be skipped.
// 
// This function will append a '\0' to the characters read into
// 'line'.  Hence 'line' should have room for 'max'+1 characters.
// 
// If no characters were read in due to end of file, -1 will
// be returned.  Otherwise the original length of the line in the
// stream 'fp' (not including the newline) will be returned.
int csc_fgetline(FILE *fp, char *line, int max);


// Reads a whitespace separated word from the stream 'fp' into the array
// 'wd'.  The terminating whitespace is not included into 'wd' and it is
// not consumed.
// 
// If the word is longer than 'max' characters, then only the first 'max'
// chars of the line will be placed into 'wd'.  The remainder of the word
// will be skipped.
// 
// This function will append a '\0' to the characters read into 'wd'.
// Hence 'wd' should have room for 'max'+1 characters.
// 
// If no characters were read in due to end of file, -1 will be returned.
// Otherwise the original length of the word in the stream 'fp' (not
// including the terminating whitespace) will be returned.
int csc_fgetwd(FILE *fp, char *wd, int max);


// This routine takes a string 'line' as input, splits up the words
// in the string, and assigns them in order to the array of character
// strings 'argv'.   The number of words assigned in argv is returned.
// 
// On entry to this function, arguments in 'line' are separated by
// whitespace.  'line' will be overwritten, and the pointers in 'argv'
// will point into 'line'.
// 
// Argv can have at most 'n' strings.
int csc_param(char *argv[], char *line, int n);


// csc_param_quote() takes the same arguments as csc_param(), and the only
// difference is that strings enclosed in quotes (e.g. "one word") are
// treated as words.  In this case the character after the opening quote
// will appear as the first character of the word and the terminating quote
// will be replaced by a null byte.  
int csc_param_quote(char *argv[], char *line, int n);


// Fills 'str' with a a null terminated string containing the date and time
// in the format "YYYYMMDD.hhmmss".
#define csc_timeStrSize 15
void csc_dateTimeStr(char str[csc_timeStrSize+1]);


// Transfers all bytes from the stream 'fin' to the stream 'fout'.
// Returns the number of bytes actually transferred.
int64_t csc_xferBytes(FILE *fin, FILE *fout);

// Transfers up to 'N' bytes from the stream 'fin' to the stream 'fout'.
// May transfer less than 'N' if the end of file is discovered, or on
// error.  Returns the number of bytes actually transferred.
int64_t csc_xferBytesN(FILE *fin, FILE *fout, int64_t nBytes);

// Returns a basic checksum CS4 of the string 'str'.
// CS4 is not well known and is not a cryptographic checksom.
uint64_t csc_cs4(char *str);

#define csc_mck_IS_ON 1
#if csc_mck_IS_ON 

#define malloc(size)            csc_mck_malloc(size,__LINE__,__FILE__)
#define calloc(nelem,elsize)    csc_mck_calloc(nelem,elsize,__LINE__,__FILE__)
#define free(block)             csc_mck_free(block,__LINE__,__FILE__)
#define realloc(block,size)     csc_mck_realloc(block,size,__LINE__,__FILE__)
#define strdup(str)             csc_mck_strdup(str,__LINE__,__FILE__)

#define csc_mck_check(flag)     csc_mck_checkmem(flag,__LINE__,__FILE__)

#ifdef MEMCHECK_SILENT
#define exit(status)            csc_mck_sexit(status,__LINE__,__FILE__)
#else
#define exit(status)            csc_mck_exit(status,__LINE__,__FILE__)
#endif

// extern long mck_maxchunks;

#ifdef __STDC__
void *csc_mck_malloc(unsigned int size, int line, char *file);
void *csc_mck_calloc(unsigned int nelem, unsigned int elsize, int line, char *file);
char *csc_mck_strdup(char *str, int line, char *file);
void csc_mck_free(void *block, int line, char *file);
void *csc_mck_realloc(void *block, unsigned int size, int line, char *file);
void csc_mck_sexit(int status, int line, char *file);
void csc_mck_exit(int status, int line, char *file);
long csc_mck_nchunks(void);
int csc_mck_checkmem(int flag, int line, char *file);
void csc_mck_print(FILE *fout);
void csc_mck_changeMark(long oldMarkVal, long newMarkVal);
void csc_mck_printMarkEq(FILE *fout, long markVal);
void csc_mck_setMark(long newMarkVal);
#else
char *csc_mck_malloc();
char *csc_mck_calloc();
char *csc_mck_strdup();
void csc_mck_free();
char *csc_mck_realloc();
void csc_mck_exit();
void csc_mck_sexit();
long csc_mck_nchunks();
int csc_mck_check();
void csc_mck_print();
#endif

#else
#define csc_mck_check(flag) ((void)0)
#define csc_mck_print(fout) ((void)0)
#endif

#endif


