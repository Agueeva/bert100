#include <types.h>
#include <string.h>
#include "console.h"
#include "bert_eml.h"
#include "amp_eml.h"
#include "hex.h"
#include "database.h"
#include "interpreter.h"
#include "cdr.h"
#include "ad537x.h"
#include "pvar.h"

#define NR_TX_DRIVER_SETTINGS   (20)
#define DATASET_SIGNATURE	(0x08155713) 

typedef struct TxDriverSettings {
        uint32_t signature;
	float   outAmplVolt;
        char    strDescription[32];

        float vg1[4];
        float vg2[4];
        float vg3[4];
        float vd1[4];
        float vd2[4];
        float vs[4];

        uint8_t txaSwing[4];
        uint8_t txaEqpst[4];
        uint8_t txaEqpre[4];
        uint8_t txaSwingFine[4];
        bool    swapTxPN[4];
} TxDriverSettings;

/**
 *********************************************************************************
 * Mach Zehnder specific part of the BERT.
 *********************************************************************************
 */
typedef struct BertEML {
        int32_t currentDataSet;
	float outAmplVolt;
        char currDataSetDescr[32];
} BertEML;

static BertEML gBertEML;

/**
 ***********************************************************************************************
 *
 ***********************************************************************************************
 */
static bool 
BertEML_FindInterpolDs(BertEML *beml, float outAmplVolt, uint16_t *dsetLow, uint16_t *dsetHigh)
{
	int i;
	bool result;
        static TxDriverSettings txDs;
	float outAmplLow = 0;
	float outAmplHigh = 1e20;
	int16_t dataSetHigh = -1; 
	int16_t dataSetLow = 0x7fff; 
	for(i = 0; i < NR_TX_DRIVER_SETTINGS; i++) {  
        	memset(&txDs,0,sizeof(txDs));
        	result = DB_GetObj(DBKEY_BERT0_EMLTXDRIVER_SETTINGS(i),&txDs,sizeof(txDs));
		if(result == false) {
			continue;
		}	
		if(txDs.signature != DATASET_SIGNATURE) {
			continue;
		}
		if((txDs.outAmplVolt <= outAmplVolt) && (txDs.outAmplVolt > outAmplLow)) {
			outAmplLow = txDs.outAmplVolt;	
			dataSetLow = i;
		}
		if((txDs.outAmplVolt >= outAmplVolt) && (txDs.outAmplVolt < outAmplHigh)) {
			outAmplHigh = txDs.outAmplVolt;	
			dataSetHigh = i;
		}
	}
	if((dataSetHigh >= 0) && (dataSetLow < NR_TX_DRIVER_SETTINGS)) {
		*dsetLow = dataSetLow;
		*dsetHigh = dataSetHigh;
		return true;
	}
	return false;
}

/**
 ***********************************************************************************************
 * \fn static bool BertEML_SetOutAmplVolt(BertEML *beml, uint16_t chNr, float outAmplVolt); 
 ***********************************************************************************************
 */
static bool 
BertEML_SetOutAmplVolt(BertEML *beml, uint16_t chNr, float outAmplVolt) {
	bool result;
	uint16_t dsetLow,dsetHigh;
        TxDriverSettings txDsLow;
        TxDriverSettings txDsHigh;
	float weightLow, weightHigh, weightSum;
        float vg1;
        float vg2;
        float vg3;
        float vd1;
        float vd2;
        float vs;
	if(chNr >= 4) {
		return false;
	}
	if(BertEML_FindInterpolDs(beml, outAmplVolt, &dsetLow, &dsetHigh) == false) {
		return false;
	}
	result = DB_GetObj(DBKEY_BERT0_EMLTXDRIVER_SETTINGS(dsetLow),&txDsLow,sizeof(txDsLow));
	if(result == false) {
		return false;
	}	
	result = DB_GetObj(DBKEY_BERT0_EMLTXDRIVER_SETTINGS(dsetHigh),&txDsHigh,sizeof(txDsHigh));
	if(result == false) {
		return false;
	}	
	if(txDsHigh.signature != DATASET_SIGNATURE) {
		return false;
	}
	if(txDsLow.signature != DATASET_SIGNATURE) {
		return false;
	}
	weightLow = txDsHigh.outAmplVolt - outAmplVolt;
	weightHigh = outAmplVolt -  txDsLow.outAmplVolt;
	Printf("Ds %u, %u, weight %f %f\n",dsetLow, dsetHigh, weightLow, weightHigh);
	if((weightLow < 0.0001) && (weightHigh < 0.0001)) {
		weightHigh = 1;
		weightLow = 0;	
	}
	weightSum = weightHigh + weightLow;
	if(weightSum == 0) {
		return false;
	}
	weightHigh /= weightSum;
	weightLow /= weightSum; 
	weightSum = 1;
	vg1 = txDsHigh.vg1[chNr] *  weightHigh + txDsLow.vg1[chNr] * weightLow;
	DAC_Set(DAC_EMLAMP1_VG1(chNr),vg1);

	vg2 = txDsHigh.vg2[chNr] *  weightHigh + txDsLow.vg2[chNr] * weightLow;
        DAC_Set(DAC_EMLAMP1_VG2(chNr),vg2);

	vg3 = txDsHigh.vg3[chNr] *  weightHigh + txDsLow.vg3[chNr] * weightLow;
	DAC_Set(DAC_EMLAMP1_VG3(chNr),vg3);

	vd2 = txDsHigh.vd2[chNr] *  weightHigh + txDsLow.vd2[chNr] * weightLow;
	DAC_Set(DAC_EMLAMP1_VD2(chNr),vd2);

	vd1 = txDsHigh.vd1[chNr] *  weightHigh + txDsLow.vd1[chNr] * weightLow;
	DAC_Set(DAC_EMLAMP1_VD1(chNr),vd1);

	vs = txDsHigh.vs[chNr] *  weightHigh + txDsLow.vs[chNr] * weightLow;
	DAC_Set(DAC_EMLAMP1_VS(chNr),vs);
	return true;
}

/*
 ************************************************************************************************
 * \fn static void BertEML_LoadDataset(); 
 ************************************************************************************************
 */
static bool 
BertEML_ActivateDataset(BertEML *beml, const TxDriverSettings *txDs) {
        unsigned int chNr;
        if(txDs->signature != DATASET_SIGNATURE) {
                Con_Printf("Dataset not valid\n");
                return false;
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_EMLAMP1_VG1(chNr),txDs->vg1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_EMLAMP1_VG2(chNr),txDs->vg2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_EMLAMP1_VG3(chNr),txDs->vg3[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_EMLAMP1_VD2(chNr),txDs->vd2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_EMLAMP1_VD1(chNr),txDs->vd1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Set(DAC_EMLAMP1_VS(chNr),txDs->vs[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                CDR_Write(CDR_ID_TX,CDR_TXA_SWING(chNr),txDs->txaSwing[chNr]);
                CDR_Write(CDR_ID_TX,CDR_TXA_EQPST(chNr),txDs->txaEqpst[chNr]);
                CDR_Write(CDR_ID_TX,CDR_TXA_EQPRE(chNr),txDs->txaEqpre[chNr]);
                CDR_Write(CDR_ID_TX,CDR_TXA_SWING_FINE(chNr),txDs->txaSwingFine[chNr]);
                CDR_Write(CDR_ID_TX,CDR_SWAP_TXP_N(chNr),txDs->swapTxPN[chNr]);
        }
	beml->outAmplVolt = txDs->outAmplVolt;
        SNPrintf(beml->currDataSetDescr,array_size(beml->currDataSetDescr),"%s",txDs->strDescription);
	return true;
}

/**
 ********************************************************************************
 * \fn static bool Bert_LoadDataset(uint16_t idx) 
 * Load a dataset with DAC, CDR and modulator settings from the database
 ********************************************************************************
 */
static bool
BertEML_LoadDataset(BertEML *beml,uint16_t idx)
{
        TxDriverSettings txDs;
        bool result;
        unsigned int descrLen;
        if(idx >= NR_TX_DRIVER_SETTINGS) {
                Con_Printf("Selected bad driver setting with index %u\n",idx);
                return false;
        }
        memset(&txDs,0,sizeof(txDs));
        result = DB_GetObj(DBKEY_BERT0_EMLTXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
        if(result == false) {
                Con_Printf("Failed to load dataset %u\n",idx);
                return false;
        }
        descrLen = array_size(txDs.strDescription);
        if(txDs.strDescription[descrLen - 1] != 0) {
                txDs.strDescription[0] = 0;     /* Completely invalidate it in this case */
        }
	return BertEML_ActivateDataset(beml, &txDs); 
}

/**
 ********************************************************************************
 * \nf static bool Bert_SaveDataset(uint16_t idx) 
 * Write a dataset with DAC, CDR and modulator settings to the database
 ********************************************************************************
 */
static bool
BertEML_SaveDataset(BertEML *beml, uint16_t idx)
{
        TxDriverSettings txDs;
        bool result;
        unsigned int chNr;
        memset(&txDs,0,sizeof(txDs));
        if(idx >= NR_TX_DRIVER_SETTINGS) {
                Con_Printf("Selected bad driver setting with index %u\n",idx);
                return false;
        }
	txDs.outAmplVolt = beml->outAmplVolt;
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_EMLAMP1_VG1(chNr),&txDs.vg1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_EMLAMP1_VG2(chNr),&txDs.vg2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_EMLAMP1_VG3(chNr),&txDs.vg3[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_EMLAMP1_VD2(chNr),&txDs.vd2[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_EMLAMP1_VD1(chNr),&txDs.vd1[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                DAC_Get(DAC_EMLAMP1_VS(chNr),&txDs.vs[chNr]);
        }
        for(chNr = 0; chNr < 4; chNr++) {
                txDs.txaSwing[chNr] =   CDR_Read(CDR_ID_TX,CDR_TXA_SWING(chNr));
                txDs.txaEqpst[chNr] =   CDR_Read(CDR_ID_TX,CDR_TXA_EQPST(chNr));
                txDs.txaEqpre[chNr] =   CDR_Read(CDR_ID_TX,CDR_TXA_EQPRE(chNr));
                txDs.txaSwingFine[chNr] = CDR_Read(CDR_ID_TX,CDR_TXA_SWING_FINE(chNr));
                txDs.swapTxPN[chNr] = CDR_Read(CDR_ID_TX,CDR_SWAP_TXP_N(chNr));
        }
        SNPrintf(txDs.strDescription,array_size(txDs.strDescription), "%s",beml->currDataSetDescr);
        txDs.signature = DATASET_SIGNATURE;
        result = DB_SetObj(DBKEY_BERT0_EMLTXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
        if(result == false) {
                Con_Printf("Failed to save dataset %u\n",idx);
                return false;
        }
        return true;
}

static bool
PVDataSet_OutAmplSet(void *cbData, uint32_t adId, const char *strP)
{
        BertEML *beml = cbData;
        beml->outAmplVolt = astrtof32(strP);
	return true;
}

static bool
PVDataSet_OutAmplGet(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        BertEML *beml = cbData;
        SNPrintf(bufP, maxlen, "%f", beml->outAmplVolt);
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
        BertEML *beml = cbData;
        idx = astrtoi16(strP);
        if(BertEML_LoadDataset(beml, idx) == false) {
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
        BertEML *beml = cbData;
        idx = astrtoi16(strP);
        if(idx < 1) {
                /* Dataset 0 is unchangeable by the user */
                return false;
        }
        if(BertEML_SaveDataset(beml, idx) == false) {
                return false;
        } else {
                return true;
        }
}

/**
 ****************************************************************************************
 * \fn static bool PVDataSet_SetDescr(void *cbData, uint32_t adId, const char *strP)
 * Set the description of a dataset.
 ****************************************************************************************
 */
static bool
PVDataSet_SetDescr(void *cbData, uint32_t adId, const char *strP)
{
        BertEML *beml = cbData;
        SNPrintf(beml->currDataSetDescr, array_size(beml->currDataSetDescr), "%s", strP);
        return true;
}

static bool
PVDataSet_GetDescr(void *cbData, uint32_t adId, char *bufP,uint16_t maxlen)
{
        BertEML *beml = cbData;
        SNPrintf(bufP,maxlen,"\"%s\"",beml->currDataSetDescr);
        return true;
}

/*
 ************************************************************************************************
 * \fn static bool Bert_ShowDataset(uint16_t idx) 
 ************************************************************************************************
 */
static bool
BertEML_ShowDataset(uint16_t idx)
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
        result = DB_GetObj(DBKEY_BERT0_EMLTXDRIVER_SETTINGS(idx),&txDs,sizeof(txDs));
        if(result == false) {
                Con_Printf("Failed to load dataset %u\n",idx);
                return false;
        }
        descrLen = array_size(txDs.strDescription);
        if(txDs.strDescription[descrLen - 1] != 0) {
                txDs.strDescription[0] = 0;     /* Completely invalidate it in this case */
        }
        if(txDs.signature != DATASET_SIGNATURE) {
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
        Con_Printf("Name: \"%s\"\n",txDs.strDescription);
        return true;
}

static bool
PV_SetOutAmpl(void *cbData, uint32_t adId, const char *strP)
{
        BertEML *beml = cbData;
	uint16_t chNr = adId;
	float outAmplVolt = astrtof32(strP); 
	return BertEML_SetOutAmplVolt(beml, chNr, outAmplVolt); 
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
        BertEML *beml = &gBertEML;
        if((argc == 3) && (strcmp(argv[1],"load") == 0)) {
                dataSetNr = astrtoi16(argv[2]);
                BertEML_LoadDataset(beml,dataSetNr);
        } else if((argc == 3) && (strcmp(argv[1],"save") == 0)) {
                dataSetNr = astrtoi16(argv[2]);
                BertEML_SaveDataset(beml,dataSetNr);
        } else if((argc == 3) && (strcmp(argv[1],"dump") == 0)) {
                dataSetNr = astrtoi16(argv[2]);
                BertEML_ShowDataset(dataSetNr);
        } else {
                return -EC_BADARG;
        }
        return 0;
}

INTERP_CMD(datasetCmd, "dataset", cmd_dataset, "dataset <load | save | dump> <DataSetNr> # ");


/**
 * EML specific part of the Bert module 
 */
void
BertEML_Init(const char *name)
{
	BertEML *beml = &gBertEML;
	uint16_t chNr;
        PVar_New(NULL,PVDataSet_Load,beml,0 ,"%s.%s",name,"loadDataSet");
        PVar_New(NULL,PVDataSet_Save,beml,0 ,"%s.%s",name,"saveDataSet");
        PVar_New(PVDataSet_GetDescr,PVDataSet_SetDescr,beml,0 ,"%s.%s",name,"dataSetDescription");
        PVar_New(PVDataSet_OutAmplGet,PVDataSet_OutAmplSet,beml,0 ,"%s.%s",name,"dataSetOutAmpl");
	for(chNr = 0; chNr < 4; chNr++) {
        	PVar_New(NULL,PV_SetOutAmpl,beml,chNr ,"%s.outAmpl%u",name,chNr);
	}
        Interp_RegisterCmd(&datasetCmd);
}
