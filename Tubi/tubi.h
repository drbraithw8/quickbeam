
typedef enum
{	tubiE_t = 0,  // typewriter font.
	tubiE_u,      // underline.
	tubiE_b,      // bold.
	tubiE_i,      // italics.
	tubiE_n,      // number of orthogonal text attributes.
	tubiE_p      // plain text: indicates not a tubi token.
} tubi_e;




typedef struct tubi_t tubi_t;

tubi_t *tubi_new(); 
void tubi_free(tubi_t *tubi); 
void tubi_addText(tubi_t *tubi, const char *txt);
void tubi_addTubi(tubi_t *tubi, tubi_e attr, csc_bool_t isOn); 

	
