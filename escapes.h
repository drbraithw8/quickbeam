#define escape_size 12

typedef struct escape_t
{	char isEsc[escape_size];
} escape_t;

extern escape_t escapesGlobal;
extern escape_t escapesFrame;
extern escape_t escapesVerbatim;
extern escape_t escapesAll;

extern char *escape_expansions[];

int escape_escInd(int ch);
void escape_setOnOff(escape_t *esc, const char **words, int nWords);
