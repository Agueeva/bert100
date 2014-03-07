#include "database.h"
#include "types.h"
#define VARIANT_EML 	(0)
#define VARIANT_MZ	(1)
#define DBKEY_VARIANT		(DBKEY_VERSION(1))
#define DBKEY_HWREV		(DBKEY_VERSION(2))
#define DBKEY_SERIALNUMBER	(DBKEY_VERSION(3))
const char *System_GetVersion(void);
uint8_t System_GetVariant(void);
const char *System_GetSerialNumber(void);

void Version_Init(void);
