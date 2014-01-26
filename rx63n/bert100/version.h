#include "database.h"
#include "types.h"
#define VARIANT_EML 	(0)
#define VARIANT_MZ	(1)
#define DBKEY_VARIANT	(DBKEY_VERSION(1))
#define DBKEY_HWREV	(DBKEY_VERSION(2))
const char *Version_GetStr(void);
uint8_t Variant_Get(void);
void Version_Init(void);
