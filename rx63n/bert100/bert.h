#include "database.h"
#define DBKEY_BERT0_SWAP_TXPN(ch)		DBKEY_BERT(0 + (ch) * 0x100)
#define DBKEY_BERT0_TXDRIVER_SETTINGS(idx)	DBKEY_BERT(1 + (idx) * 0x100)
void Bert_Init(void);
