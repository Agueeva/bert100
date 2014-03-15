#include <types.h>
#include <string.h>
#include "console.h"
#include "bert_mz.h"
#include "amp_mz.h"
#include "hex.h"
#include "database.h"
#include "interpreter.h"
#include "cdr.h"
#include "modreg.h"
#include "ad537x.h"
#include "pvar.h"

#define NR_TX_DRIVER_SETTINGS   (20)

typedef struct TxDriverSettings {
        uint32_t signature;
        float vg1[4];
        float vg2[4];
        float vd1[4];
        float vd2[4];

        uint8_t txaSwing[4];
        uint8_t txaEqpst[4];
        uint8_t txaEqpre[4];
        uint8_t txaSwingFine[4];
        bool    swapTxPN[4];
        float   modKi[4];
	float 	modDelay;

        char    strDescription[32];
} TxDriverSettings;

/**
 *********************************************************************************
 * Mach Zehnder specific part of the BERT.
 *********************************************************************************
 */
typedef struct BertMZ {
        int32_t currentDataSet;
        char currDataSetDescr[32];
} BertMZ;

static BertMZ gBertMZ;
/**
 ********************************************************************************
 * \fn static bool Bert_LoadDataset(uint16_t idx) 
 * Load a dataset with DAC, CDR and modulator settings from the database
 ********************************************************************************
 */
static bool
BertMZ_LoadDataset(BertMZ *bert,uint16_t idx)
{
        TxDriverSettings txDs;
        bool result;
        unsigned int chNr;
        unsigned int descrLen;
        if(idx >= NR_TX_DRIVER_SETTINGS) {
                Con_Printf("Selected bad driver setting with index %u\n",idx);
                return false;
        }
        memset(&txDs,0,sizeof(txDs));
        result = DB_GetObj(DBKEY_BERT0_MZTXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
        if(result == false) {
                Con_Printf("Failed to load dataset %u\n",idx);
                return false;
        }
        descrLen = array_size(txDs.strDescription);
        if(txDs.strDescription[descrLen - 1] != 0) {
                txDs.strDescription[0] = 0;     /* Completely invalidate it in this case */
        }
        if(txDs.signature != 0x08161234) {
                Con_Printf("Dataset not valid\n");
                return false;
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_MZAMP1_VG1(chNr),txDs.vg1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_MZAMP1_VG2(chNr),txDs.vg2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_MZAMP1_VD2(chNr),txDs.vd2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_MZAMP1_VD1(chNr),txDs.vd1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                CDR_Write(CDR_ID_TX,CDR_TXA_SWING(chNr),txDs.txaSwing[chNr]);
                CDR_Write(CDR_ID_TX,CDR_TXA_EQPST(chNr),txDs.txaEqpst[chNr]);
                CDR_Write(CDR_ID_TX,CDR_TXA_EQPRE(chNr),txDs.txaEqpre[chNr]);
                CDR_Write(CDR_ID_TX,CDR_TXA_SWING_FINE(chNr),txDs.txaSwingFine[chNr]);
                CDR_Write(CDR_ID_TX,CDR_SWAP_TXP_N(chNr),txDs.swapTxPN[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                ModReg_SetKi(chNr,txDs.modKi[chNr]);
        }
	ModReg_SetDelay(txDs.modDelay); 
        SNPrintf(bert->currDataSetDescr,array_size(bert->currDataSetDescr),"%s",txDs.strDescription);
	// Mod_GetDelay();
        return true;
}

/**
 ********************************************************************************
 * \nf static bool Bert_SaveDataset(uint16_t idx) 
 * Write a dataset with DAC, CDR and modulator settings to the database
 ********************************************************************************
 */
static bool
BertMZ_SaveDataset(BertMZ *bmz, uint16_t idx)
{
        TxDriverSettings txDs;
        bool result;
        unsigned int chNr;
        memset(&txDs,0,sizeof(txDs));
        if(idx >= NR_TX_DRIVER_SETTINGS) {
                Con_Printf("Selected bad driver setting with index %u\n",idx);
                return false;
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_MZAMP1_VG1(chNr),&txDs.vg1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_MZAMP1_VG2(chNr),&txDs.vg2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_MZAMP1_VD2(chNr),&txDs.vd2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_MZAMP1_VD1(chNr),&txDs.vd1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                txDs.txaSwing[chNr] =   CDR_Read(CDR_ID_TX,CDR_TXA_SWING(chNr));
                txDs.txaEqpst[chNr] =   CDR_Read(CDR_ID_TX,CDR_TXA_EQPST(chNr));
                txDs.txaEqpre[chNr] =   CDR_Read(CDR_ID_TX,CDR_TXA_EQPRE(chNr));
                txDs.txaSwingFine[chNr] = CDR_Read(CDR_ID_TX,CDR_TXA_SWING_FINE(chNr));
                txDs.swapTxPN[chNr] = CDR_Read(CDR_ID_TX,CDR_SWAP_TXP_N(chNr));
        }
        for(chNr = 0; chNr < 4; chNr++) {
                txDs.modKi[chNr] = ModReg_GetKi(chNr);
        }
        SNPrintf(txDs.strDescription,array_size(txDs.strDescription), "%s",bmz->currDataSetDescr);
	txDs.modDelay = ModReg_GetDelay(); 
        txDs.signature = 0x08161234;
        result = DB_SetObj(DBKEY_BERT0_MZTXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
        if(result == false) {
                Con_Printf("Failed to save dataset %u\n",idx);
                return false;
        }
        return true;
}

/**
 *********************************************************************************
 * \fn static bool PVDataSet_Load(void *cbData, uint32_t adId, const char *strP) 
 *********************************************************************************
 */
static bool
PVDataSet_Load(void *cbData, uint32_t adId, const char *strP)
{
        uint16_t idx;
        BertMZ *bmz = cbData;
        idx = astrtoi16(strP);
        if(BertMZ_LoadDataset(bmz, idx) == false) {
                return false;
        } else {
                return true;
        }
}

/**
 ***********************************************************************************
 * \fn static bool PVDataSet_Save(void *cbData, uint32_t adId, const char *strP) 
 * Save a dataset to the database. 
 ***********************************************************************************
 */
static bool
PVDataSet_Save(void *cbData, uint32_t adId, const char *strP)
{
        uint16_t idx;
        BertMZ *bmz = cbData;
        idx = astrtoi16(strP);
        if(idx < 1) {
                /* Dataset 0 is unchangeable by the user */
                return false;
        }
        if(BertMZ_SaveDataset(bmz, idx) == false) {
                return false;
        } else {
                return true;
        }
}

/**
 ****************************************************************************
 * Set Data set description.
 ****************************************************************************
 */
static bool
PVDataSet_SetDescr(void *cbData, uint32_t adId, const char *strP)
{
        BertMZ *bmz = cbData;
        SNPrintf(bmz->currDataSetDescr, array_size(bmz->currDataSetDescr), "%s", strP);
        return true;
}

static bool
PVDataSet_GetDescr(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        BertMZ *bmz = cbData;
        SNPrintf(bufP,maxlen,"\"%s\"",bmz->currDataSetDescr);
        return true;
}

/*
 ************************************************************************************************
 * \fn static bool Bert_ShowDataset(uint16_t idx) 
 ************************************************************************************************
 */
static bool
BertMZ_ShowDataset(uint16_t idx)
{
        TxDriverSettings txDs;
        bool result;
        unsigned int descrLen;
        unsigned int chNr;
        if(idx >= NR_TX_DRIVER_SETTINGS) {
                Con_Printf("Selected bad driver setting with index %u\n",idx);
                return false;
        }
        memset(&txDs,0,sizeof(txDs));
        result = DB_GetObj(DBKEY_BERT0_MZTXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
        if(result == false) {
                Con_Printf("Failed to load dataset %u\n",idx);
                return false;
        }
        descrLen = array_size(txDs.strDescription);
        if(txDs.strDescription[descrLen - 1] != 0) {
                txDs.strDescription[0] = 0;     /* Completely invalidate it in this case */
        }
        if(txDs.signature != 0x08161234) {
                Con_Printf("Dataset not valid\n");
                return false;
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("vg1_%u %f\n",chNr,txDs.vg1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("vg2_%u %f\n",chNr,txDs.vg2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("vd2_%u %f\n",chNr,txDs.vd2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("vd1_%u %f\n",chNr,txDs.vd1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("txaSwing_%u: %u\n",chNr,txDs.txaSwing[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("txaSwingFine_%u: %u\n",chNr,txDs.txaSwingFine[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("txaEqpst_%u: %u\n",chNr,txDs.txaEqpst[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("txaEqpre_%u: %u\n",chNr,txDs.txaEqpre[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("swapTxPN_%u: %u\n",chNr,txDs.txaEqpre[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                Con_Printf("modKI_%u: %f\n",chNr,txDs.modKi[chNr]);
        }
	Con_Printf("modDelay: %f\n",txDs.modDelay);
        Con_Printf("Name: \"%s\"\n",txDs.strDescription);
        return true;
}

/**
 ***********************************************************************************
 * \fn static int8_t cmd_dataset(Interp * interp, uint8_t argc, char *argv[])
 ***********************************************************************************
 */
static int8_t
cmd_dataset(Interp * interp, uint8_t argc, char *argv[])
{
        uint16_t dataSetNr;
        BertMZ *bmz = &gBertMZ;
        if((argc == 3) && (strcmp(argv[1],"load") == 0)) {
                dataSetNr = astrtoi16(argv[2]);
                BertMZ_LoadDataset(bmz,dataSetNr);
        } else if((argc == 3) && (strcmp(argv[1],"save") == 0)) {
                dataSetNr = astrtoi16(argv[2]);
                BertMZ_SaveDataset(bmz,dataSetNr);
        } else if((argc == 3) && (strcmp(argv[1],"dump") == 0)) {
                dataSetNr = astrtoi16(argv[2]);
                BertMZ_ShowDataset(dataSetNr);
        } else {
                return -EC_BADARG;
        }
        return 0;
}

INTERP_CMD(datasetCmd, "dataset", cmd_dataset, "dataset <load | save | dump> <DataSetNr> # ");


/**
 * MZ specific part of the Bert module 
 */
void
BertMZ_Init(const char *name)
{
	BertMZ *bmz = &gBertMZ;
        PVar_New(NULL,PVDataSet_Load,bmz,0 ,"%s.%s",name,"loadDataSet");
        PVar_New(NULL,PVDataSet_Save,bmz,0 ,"%s.%s",name,"saveDataSet");
        PVar_New(PVDataSet_GetDescr,PVDataSet_SetDescr,bmz,0 ,"%s.%s",name,"dataSetDescription");
        Interp_RegisterCmd(&datasetCmd);
}

