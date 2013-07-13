#include "types.h"

#define SPI_FLAG_FF			(0)
#define SPI_FLAG_WRITE		(1)
#define SPI_FLAG_READ		(2)
#define SPI_FLAG_VERIFY		(4)
#define SPI_FLAG_BIDIR		(SPI_FLAG_READ | SPI_FLAG_WRITE)

#define SPIX_OK		(0)
#define SPIX_BUSY	(1)
#define SPIX_ERROR	(2)

void Spi_Init(void);

/*
 ****************************************************
 * Get the bus. Returns SPIX_OK on success.
 ****************************************************
 */
bool Spi_Xmit(uint8_t bus, uint8_t * data, int count, uint8_t flags);
uint8_t Spir(uint8_t bus);
uint8_t Spix(uint8_t bus, uint8_t data);
void Spiw(uint8_t bus, uint8_t data);
void Spir_Block(uint8_t bus, uint8_t * data, uint16_t count);
void Spiw_Block(uint8_t bus, const uint8_t * data, uint16_t count);

/*
 *************************************************************************************
 * The following functions are required if the Port pins of the SPI are used
 * for some non SPI purpose like PLD-JTAG.
 *************************************************************************************
 */
void Spi_Lock(uint8_t bus, uint8_t mode);	/* returns true if we got the lock */
void Spi_Unlock(uint8_t bus);
void Spi_Enable(uint8_t bus_nr);	/* Enable the spi (after use of Ports for other Purpose for example) */
