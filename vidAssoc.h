
typedef struct vidAssoc_t vidAssoc_t;

typedef enum
{	vidAssoc_noAssoc = 0  // We are not trying to do anything special
						  // with panopto videos.
,	vidAssoc_markSlide = 1  // We are marking the slides for panopto search.
,	vidAssoc_doLink = 2    // We are adding a URL to each slide we have info on.
} vidAssoc_runMode_t;

vidAssoc_t *vidAssoc_new(vidAssoc_runMode_t mode, char *word);
void vidAssoc_free(vidAssoc_t *va);

void vidAssoc_readAssocs(vidAssoc_t *va, char *qbvPath, csc_str_t *errStr);
void vidAssoc_do(vidAssoc_t *va, csc_str_t *frm, const char *title, int slideNum);

