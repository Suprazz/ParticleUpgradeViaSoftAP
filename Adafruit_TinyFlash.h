#ifndef _TINYFLASH_H_
#define _TINYFLASH_H_

#include "application.h"

// For IS25LQ080
#define CHIP_BYTES       1L * 1024L * 1024L
#define MAN_ID 0x9D
//#define MAN_ID2 0x7F
#define DEV_ID 0x13 
//#define DEV_ID2 0x44

// W25Q80BV
#ifndef CHIP_BYTES
	#define CHIP_BYTES       1L * 1024L * 1024L
#endif
#ifndef MAN_ID
#define MAN_ID 0xEF
#endif
#ifndef DEV_ID
#define DEV_ID 0x13 
#endif

#define PAGE_SIZE 256

#define CMD_PAGEPROG     0x02
#define CMD_READDATA     0x03
#define CMD_READFASTDATA 0x0B
#define CMD_WRITEDISABLE 0x04
#define CMD_READSTAT1    0x05
#define CMD_WRITEENABLE  0x06
#define CMD_SECTORERASE  0x20
#define CMD_CHIPERASE    0x60
#define CMD_ID           0x90
#define CMD_WRITESTAT	 0x01

#define STAT_BUSY        0x01
#define STAT_WRTEN       0x02




class Adafruit_TinyFlash {
 public:

	Adafruit_TinyFlash();

  uint32_t          begin(uint8_t flashcs);
  boolean           beginRead(uint32_t addr),
	  // Write max 256 bytes.
                    writePage(uint32_t addr, uint8_t *data, uint32_t length),
                    eraseChip(void),
                    eraseSector(uint32_t addr);
  boolean           beginFastRead(uint32_t addr);
  // Write any amount of data
  //boolean writeData(const uint8_t* data, uint32_t address, uint32_t length);
  uint8_t           readNextByte(void);
  void              endRead(void);
  uint8_t ReadStatusRegister(void);
    boolean WriteStatusRegister(uint8_t data);
	boolean readData(uint8_t* data, uint32_t address, uint32_t length);

 private:
  boolean           waitForReady(uint32_t timeout = 100L),
                    writeEnable(void);
  void              writeDisable(void),
                    cmd(uint8_t c);

  volatile uint8_t cs_port;
};

#endif // _TINYFLASH_H_
