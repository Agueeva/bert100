#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define POLY 0xEDB88320
#define FLASH_SIZE 		(1024 * 1024)
#define LOADER_SIZE		(4096)
#define CRC_START_VALUE 	0x73911293
static uint32_t
CRC32Byte_Eth(uint32_t crc,uint8_t val)
{
    int i;
    for(i=0;i < 8; i++) {
            int carry = !(crc & 1);
            int inbit = !!(val & (1<<i));
            crc = (crc >> 1)  | (UINT32_C(1) << 31);
            if(inbit ^ carry) {
                    crc = crc ^ POLY;
            }
    }
    return crc;
}

static uint32_t
CRC32_Eth(uint32_t crc,uint8_t *buf,uint32_t len)
{
    uint32_t i;
    for(i = 0; i < len; i++) {
        crc = CRC32Byte_Eth(crc,buf[i]);
    }
    return crc;
}

/**
 ***************************************************************************************************
 * \fn static bool CheckWriteSignature(const char *infilename,const char *outfilename)
 * Check or write the CRC Signature of a file. If there is no outfile given (outfilename == NULL).
 ***************************************************************************************************
 */
static bool
CheckWriteSignature(const char *infilename,const char *outfilename)
{
	FILE *infile,*outfile;
	struct stat stat_buf;
	unsigned int i;
	uint8_t buf[4096];
	uint32_t crc32 = CRC_START_VALUE;
	uint32_t padcnt = 0; 
	int count;
	infile = fopen(infilename,"r");
	if(!infile) {
		fprintf(stderr,"Can't open file\n");
		return false;
	}
	if(outfilename) {
		outfile = fopen(outfilename,"w+");
		if(!outfile) {
			return false;
		}
	} else {
		outfile = NULL;
	}
	if(stat(infilename,&stat_buf) != 0) {
		fprintf(stderr,"Stat on infile failed\n");
		exit(1);
	}
	if(stat_buf.st_size < FLASH_SIZE) {
		uint32_t cnt;
		padcnt = FLASH_SIZE - stat_buf.st_size;
		memset(buf,0xff,sizeof(buf));
		for(cnt = 0; cnt < padcnt;) {
			count = (padcnt - cnt) > sizeof(buf) ? sizeof(buf) : (padcnt - cnt);
			crc32 = CRC32_Eth(crc32,buf,count);
			if(outfile) {
				count = fwrite(buf,1,count,outfile);
			}
			if(count > 0) {
				cnt += count;
			} else {
				fprintf(stderr,"write failed\n");
				perror("");	
				exit(1);
			}
		}
	}
	for(i = padcnt; i < (FLASH_SIZE - LOADER_SIZE); i+= sizeof(buf)) {
		count = fread(buf,1,sizeof(buf),infile);
		if(count != sizeof(buf)) {
			fprintf(stderr,"Read failed\n");
			return false;
		}
		crc32 = CRC32_Eth(crc32,buf,count);
		if(outfile) {
			count = fwrite(buf,1,sizeof(buf),outfile);
			if(count != sizeof(buf)) {
				fprintf(stderr,"Write failed, count %d\n",count);
				return false;
			}
		}
	}
	if(!outfile)  {
		count = fread(buf,1,4,infile);
		if(count != 4) {
		    return false;
		}
		fclose(infile);
		crc32 = CRC32_Eth(crc32,buf,4);
		if(crc32 == 0xffffffff) {
				fprintf(stdout,"CRC is good\n");
		    return true;
		} else {
				fprintf(stdout,"CRC is bad %08x %02x %02x %02x %02x\n",crc32,buf[0],buf[1],buf[2],buf[3]);
		    return false;
		}
	} else {
		count = fread(buf,4096,sizeof(buf),infile);
		buf[0] = ~crc32 & 0xff;
		buf[1] = (~crc32 >> 8) & 0xff;
		buf[2] = (~crc32 >> 16) & 0xff;
		buf[3] = (~crc32 >> 24) & 0xff;
		if(fwrite(buf,1,4096,outfile) != 4096) {
		    fprintf(stderr,"Write failed\n");
		}
		fclose(infile);
		fclose(outfile);
    	}
	return true;
}

int 
main(int argc,char *argv[]) 
{
	bool result;
	if(argc < 2) {
		fprintf(stderr,"Arg missing\n");
		exit(1);
	}
	if(argc == 2) {
		result = CheckWriteSignature(argv[1],NULL);
	} else {
		result = CheckWriteSignature(argv[1],argv[2]);
	}
	if(result == false) {
		exit(1);
	}
	exit(0);
}
