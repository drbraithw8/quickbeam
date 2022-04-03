

typedef enum
{	tubiE_t = 0,  // typewriter font.
	tubiE_u,      // underline.
	tubiE_b,      // bold.
	tubiE_i,      // italics.
	tubiE_n      // number of orthogonal text attributes.
} tubiE;

typedef struct tubi_t tubi_t;
typedef struct escape_t escape_t;
	
tubi_t *tubi_new();
void tubi_free(tubi_t *tubi);
void tubi_send(tubi_t *tubi, csc_str_t *out, escape_t *esc);
void tubi_parse(tubi_t *tubi, const char *str);
void escape_setOnOff(escape_t *esc, const char **words, int nWords);

extern escape_t escapesGlobal;
extern escape_t escapesFrame;
extern escape_t escapesVerbatim;

