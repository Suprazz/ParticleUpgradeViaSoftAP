#include "Particle.h"
#include "myFileSystem.h"
#include "Adafruit_TinyFlash.h"

#define LOG_PAGE_SIZE       256

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * 4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];
Adafruit_TinyFlash flash;

spiffs fs;

static int32_t my_spiffs_read(uint32_t addr, uint32_t size, uint8_t *dst) {
	flash.readData(dst, addr, size);
	return SPIFFS_OK;
}

static int32_t my_spiffs_write(uint32_t addr, uint32_t size, uint8_t *src) {
	if (flash.writePage(addr, src, size))
		return SPIFFS_OK;
	return SSPIFFS_WRITE_ERR;
}

static int32_t my_spiffs_erase(uint32_t addr, uint32_t size) {
	if (flash.eraseSector(addr))
		return SPIFFS_OK;
	return SPIFFS_ERASE_ERR;
}


static int my_spiffs_mount() {
	spiffs_config cfg;
	// 	cfg.phys_size = 2 * 1024 * 1024; // use all spi flash
	// 	cfg.phys_addr = 0; // start spiffs at start of spi flash
	// 	cfg.phys_erase_block = 65536; // according to datasheet
	// 	cfg.log_block_size = 65536; // let us not complicate things
	// 	cfg.log_page_size = LOG_PAGE_SIZE; // as we said

	cfg.hal_read_f = my_spiffs_read;
	cfg.hal_write_f = my_spiffs_write;
	cfg.hal_erase_f = my_spiffs_erase;

	int res = SPIFFS_mount(&fs,
		&cfg,
		spiffs_work_buf,
		spiffs_fds,
		sizeof(spiffs_fds),
		spiffs_cache_buf,
		sizeof(spiffs_cache_buf),
		0);
	//Serial.printf("mount res: %i\n", res);
	return res;
}

void myFileSystemInit()
{
	if (flash.begin(A7))
	{
		// Flash is compatible
	}
	else
	{
		// Incorrect model number or unable to communicate with flash
		Serial.println("Flash init error");
		while (1);
	}
	flash.WriteStatusRegister(0);

	// Erase all flash. Do this only the first time you use the flash chip!
	//flash.eraseChip();

	my_spiffs_mount();
}

uint16_t myFileOpen(const char * filename, uint32_t * fileSize)
{
	uint16_t destinationFile;
	spiffs_stat s;

	destinationFile = SPIFFS_open(&fs, filename, SPIFFS_RDONLY, 0);
	SPIFFS_fstat(&fs, destinationFile, &s);
	if (fileSize != NULL)
	{
		*fileSize = s.sizet;
	}
	return destinationFile;
}

uint16_t myFileOpenEmpty(const char * filename)
{
	uint16_t destinationFile;
	destinationFile = SPIFFS_open(&fs, filename, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	//SPIFFS_fstat(&fs, ff, &s);
	return destinationFile;
}