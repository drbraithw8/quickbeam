
typedef enum
{	vidAssoc_noAssoc = 0
,	vidAssoc_markSlide = 1
,	vidAssoc_doLink = 2
} vidAssoc_runMode_t;


char *vidAssoc_init(char *qbvPath, vidAssoc_runMode_t mode, char *word);
void vidAssoc_do(csc_str_t *frm, int slideNum);
void vidAssoc_end(void);
