// Author: Dr Stephen Braithwaite.
// This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License.

#ifndef csc_ISVALID_H
#define csc_ISVALID_H 1

#include "csc_std.h"

// Returns true if the string 'word' is a hexadecimal string.  Otherwise, this
// function will return false.
bool csc_isValid_hex(const char *word);


// Returns true if the string 'word' is an integer.  Otherwise, this
// function will return false.
bool csc_isValid_int(const char *word);

// If 'word' is a valid int, and its value lies between 'min' and 'max'
// inclusive, then this function returns true.  Otherwise, this
// function will return false.  Further, if the return value is
// true and 'value' is not NULL, then this function assigns the value
// to *'value'.  
bool csc_isValidRange_int(const char *word, int min, int max, int *value);


// Returns true if 'str' contains only a floating point number in floating
// point or exponential format.  Returns false otherwise.
bool csc_isValid_float(const char *str);

// If 'word' is a valid floating point number, and its value lies between
// 'min' and 'max' inclusive, then this function returns true.
// Otherwise, this function will return false.  Further, if the return
// value is true and 'value' is not NULL, then this function assigns the
// value to *'value'.  
bool csc_isValidRange_float(const char *word, double min, double max, double *value);


// Returns true if 'str' is a valid IP version 4 IP address, false if not.
bool csc_isValid_ipV4(const char *str);

// Returns true if 'str' is a valid IP version 6 IP address, false if not.
bool csc_isValid_ipV6(const char *str);

// Returns true if 'str' is a valid host or domain name, false if not.
bool csc_isValid_domain(const char *str);


// Returns true only if 'str' looks like a decent (relative, absolute, or
// either respectively) file or path name, false otherwise.  
// 
// The only characters that are actually forbidden in a UNIX file name are
// '\0' and '/'.  Therefore, it may be actually be possible to put
// unprintable or non ASCII characters, invisible characters, tabs,
// redirection characters, quotes, general punctuation, etc into a file or
// path name, but this function does not consider them to be decent.
//
// Absolute paths, empty path segments, path segments consisting only of
// one or more '.' and path segments beginning with '-' are also not
// considered by this function to be decent.  Paths ending with a '/'
// are also disallowed.
bool csc_isValid_decentRelPath(const char *str);
bool csc_isValid_decentAbsPath(const char *str);
bool csc_isValid_decentPath(const char *str);

#endif


