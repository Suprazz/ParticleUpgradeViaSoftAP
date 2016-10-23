// Barebones library for Winbond W25Q80BV serial flash memory.  Written
// with the limited space of the ATtiny85 in mind, but works on regular
// Arduinos too.  Provides basic functions needed for audio playback
// system only: chip and sector erase, block write, sequential byte read.
// Other than possibly adding support for other Winbond flash in the
// future, the plan is to NOT bloat this out with all bells and whistles;
// functions like block reads or buffered writes can be implemented in
// client code where RAM can be better managed in the context of the
// overall application (1 flash page = 1/2 the ATtiny85's RAM).
// Written by Limor Fried and Phillip Burgess for Adafruit Industries.
// MIT license.

#include "Adafruit_TinyFlash.h"


#ifdef __AVR_ATtiny85__

#define CHIP_SELECT   PORTB &= ~cs_mask;
#define CHIP_DESELECT PORTB |=  cs_mask;
#define SPIBIT                      \
  USICR = ((1<<USIWM0)|(1<<USITC)); \
  USICR = ((1<<USIWM0)|(1<<USITC)|(1<<USICLK));
static uint8_t spi_xfer(uint8_t n) {
	USIDR = n;
	SPIBIT
	SPIBIT
	SPIBIT
	SPIBIT
	SPIBIT
	SPIBIT
	SPIBIT
	SPIBIT
	return USIDR;
}

#else

//#include <SPI.h>
#define CHIP_SELECT   digitalWriteFast(cs_port, LOW); //*cs_port &= ~cs_mask;
#define CHIP_DESELECT digitalWriteFast(cs_port, HIGH);
#define spi_xfer(n)   SPI.transfer(n)

#endif

// Constructor
Adafruit_TinyFlash::Adafruit_TinyFlash() {
}

// Select chip and issue command (don't deselect; data may follow)
void Adafruit_TinyFlash::cmd(uint8_t c) {
	CHIP_SELECT
	(void)spi_xfer(c);
}

// Initialize SPI pins, validate chip is present
uint32_t Adafruit_TinyFlash::begin(uint8_t flashcs) {
	uint8_t manID, devID;

	pinMode(flashcs, OUTPUT);
	cs_port = flashcs;
	CHIP_DESELECT

#ifdef __AVR_ATtiny85__
	PORTB &= ~(_BV(PORTB0) | _BV(PORTB1) | _BV(PORTB2) | cs_mask);
	DDRB  &= ~_BV(PORTB0); // DI (NOT MISO)
	DDRB  |=  _BV(PORTB1)  // DO (NOT MOSI)
              |   _BV(PORTB2)  // SCK
              |   cs_mask;     // CS
#else
	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2); 
#endif

	cmd(CMD_ID);
	for(uint8_t i=0; i<4; i++) manID = spi_xfer(0); // 3 dummy bytes first
	devID = spi_xfer(0);
	CHIP_DESELECT

	// Chip capacity is hardcoded for now
	return ((manID == MAN_ID) && (devID == DEV_ID)) ? CHIP_BYTES : 0L;
	//return CHIP_BYTES;
}

// Poll status register until busy flag is clear or timeout occurs
boolean Adafruit_TinyFlash::waitForReady(uint32_t timeout) {
	uint8_t  status;
	uint32_t startTime = millis();

	do {
		cmd(CMD_READSTAT1);
		status = spi_xfer(0);
		CHIP_DESELECT
		if((millis() - startTime) > timeout) return false;
	} while(status & STAT_BUSY);

	return true;
}

// Set up a read operation (no data is returned yet)
boolean Adafruit_TinyFlash::beginRead(uint32_t addr) {

	if((addr >= CHIP_BYTES) || !waitForReady(10)) return false;

	cmd(CMD_READDATA);
	(void)spi_xfer(addr >> 16);
	(void)spi_xfer(addr >>  8);
	(void)spi_xfer(addr      );
	// Chip is held in selected state until endRead()

	return true;
}
// Set up a read operation (no data is returned yet)
boolean Adafruit_TinyFlash::beginFastRead(uint32_t addr) {

	if((addr >= CHIP_BYTES) || !waitForReady(10)) return false;

	cmd(CMD_READFASTDATA);
	(void)spi_xfer(addr >> 16);
	(void)spi_xfer(addr >>  8);
	(void)spi_xfer(addr      );
	(void)spi_xfer(0);
	// Chip is held in selected state until endRead()

	return true;
}

// Read next byte (call N times following beginRead())
uint8_t Adafruit_TinyFlash::readNextByte(void) {
	return spi_xfer(0);
}

// Stop read operation
void Adafruit_TinyFlash::endRead(void) {
	CHIP_DESELECT
}

// Erase the whole chip.  Boom, gone.  Use with caution.
boolean Adafruit_TinyFlash::eraseChip(void) {

	if(!waitForReady() || !writeEnable()) return false;

	// Might want to have this clear the block protect bits

	cmd(CMD_CHIPERASE);
	CHIP_DESELECT

	if(!waitForReady(10000L)) return false; // Datasheet says 6S max

	writeDisable();

	return true;
}

// Erase one 4K sector
boolean Adafruit_TinyFlash::eraseSector(uint32_t addr) {

	if(!waitForReady() || !writeEnable()) return false;

	cmd(CMD_SECTORERASE);
	(void)spi_xfer(addr >> 16); // Chip rounds this down to
	(void)spi_xfer(addr >>  8); // prior 4K sector boundary;
	(void)spi_xfer(0         ); // lowest bits are ignored.
	CHIP_DESELECT

	if(!waitForReady(1000L)) return false; // Datasheet says 400ms max

	writeDisable();

	return true;
}

// Private function used by write and erase operations
boolean Adafruit_TinyFlash::writeEnable(void) {
	uint8_t status;

	cmd(CMD_WRITEENABLE);
	CHIP_DESELECT

	// Verify write-enable status
	cmd(CMD_READSTAT1);
	status = spi_xfer(0);
	CHIP_DESELECT
	return (status & STAT_WRTEN) ? true : false;
}
uint8_t Adafruit_TinyFlash::ReadStatusRegister(void) {
	uint8_t status;
	cmd(CMD_READSTAT1);
	status = spi_xfer(0);
	CHIP_DESELECT
	return status;
}

boolean Adafruit_TinyFlash::WriteStatusRegister(uint8_t data){
	if(!waitForReady(10) || !writeEnable())
		return false;
	
	cmd(CMD_WRITESTAT);
	spi_xfer(data);
	CHIP_DESELECT
		
	if(!waitForReady(10)) return false;

	writeDisable();
	return true;
}

// Companion to above function, used after write and erase operations
void Adafruit_TinyFlash::writeDisable(void) {
	cmd(CMD_WRITEDISABLE);
	CHIP_DESELECT
}

// Writes one page: 256 bytes, starting at 256 byte boundary.  There are
// no other options.  This is the ONLY write method provided by the library;
// other capabilities (if needed) may be implemented in client code.
boolean Adafruit_TinyFlash::writePage(uint32_t addr, uint8_t *data, uint32_t length) {
	if((addr >= CHIP_BYTES) || !waitForReady(10) || !writeEnable())
		return false;

	cmd(CMD_PAGEPROG);
	(void)spi_xfer(addr >> 16);
	(void)spi_xfer(addr >>  8);
	(void)spi_xfer(addr); // If len=256, page boundary only (datasheet 7.2.21)
	for(uint32_t i=0; i<length; i++) {
		(void)spi_xfer(data[i]);
	}
	CHIP_DESELECT // Write occurs after the CS line is de-asserted

	delay(3);     // Max page program time according to datasheet

	if(!waitForReady(10)) return false;

	writeDisable();

	return true;
}

boolean Adafruit_TinyFlash::readData(uint8_t* data, uint32_t address, uint32_t length)
{

	//DEBUG_DISKIO("Read:(%d) Length:%d", address, length);
	uint32_t i;

	beginRead(address);
	for (i = 0; i < length; i++)
	{
		data[i] = readNextByte();
	}
	endRead();
	return true;
}