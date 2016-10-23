#include "Particle.h"
#include "myFileSystem.h"

#ifdef INCLUDE_SD
#include "SdFat.h"

// Pick an SPI configuration.
// See SPI configuration section below (comments are for photon).
#define SPI_CONFIGURATION 0
//------------------------------------------------------------------------------
// Setup SPI configuration.
#if SPI_CONFIGURATION == 0
// Primary SPI with DMA
// SCK => A3, MISO => A4, MOSI => A5, SS => A2 (default)
SdFat SD;
const uint8_t chipSelect = CS_MEMORY;
#elif SPI_CONFIGURATION == 1
// Secondary SPI with DMA
// SCK => D4, MISO => D3, MOSI => D2, SS => D1
SdFat sd(1);
const uint8_t chipSelect = D1;
#elif SPI_CONFIGURATION == 2
// Primary SPI with Arduino SPI library style byte I/O.
// SCK => A3, MISO => A4, MOSI => A5, SS => A2 (default)
SdFatLibSpi sd;
const uint8_t chipSelect = SS;
#elif SPI_CONFIGURATION == 3
// Software SPI.  Use any digital pins.
// MISO => D5, MOSI => D6, SCK => D7, SS => D0
SdFatSoftSpi<D5, D6, D7> sd;
const uint8_t chipSelect = D0;
#endif  // SPI_CONFIGURATION
//------------------------------------------------------------------------------
#endif

#ifdef USE_SPIFFS
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
//	if (flash.eraseBlock(addr))
//		return SPIFFS_OK;
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

#define SPIFFS_CHECK(x) \
  if ((x) < 0) { Serial.printlnf("errno:%i\n", SPIFFS_errno(&fs)); return; }
#endif 

#ifdef INCLUDE_SD
File fileArray[3]; // max 3 files opened at the same time.

int GetFirstFileAvailable()
{
	for (int i = 0; i < NumberOfElements(fileArray); i++)
	{
		if (fileArray[i] == NULL)
			return i;
	}
	Serial.println("No more space in fileArray");
	return -1;
}


#endif
int  myFileSystemInit()
{
#ifdef USE_SPIFFS
	if (flash.begin(CS_MEMORY))
	{
		// Flash is compatible
	}
	else
	{
		// Incorrect model number or unable to communicate with flash
		Serial.println("Flash init error");
		return -1;
	}
	flash.WriteStatusRegister(0);

	// Erase all flash. Do this only the first time you use the flash chip!
	//flash.eraseChip();

	return my_spiffs_mount();
#endif

	// SD
#ifdef INCLUDE_SD
// 	const uint8_t mosiPin = A5;
// 	const uint8_t misoPin = A4;
// 	const uint8_t clockPin = A3;

	for (int i = 0; i < NumberOfElements(fileArray); i++)
	{
		fileArray[i] == NULL;
	}

	if (SD.begin(CS_MEMORY) == 0)
	{
		return 1;
	}
	return 0;
#endif
}

int16_t myFileOpen(const char * filename)
{
#ifdef USE_SPIFFS
	int16_t destinationFile;
	spiffs_stat s;

	destinationFile = SPIFFS_open(&fs, filename, SPIFFS_RDWR, 0);
	//SPIFFS_CHECK(destinationFile);
	if (destinationFile < 0)
	{
		// Probleme
		return destinationFile;
	}
// 	SPIFFS_fstat(&fs, destinationFile, &s);
// 	if (fileSize != NULL)
// 	{
// 		*fileSize = s.sizet;
// 	}
	return destinationFile;
#endif

#ifdef INCLUDE_SD
	int index = GetFirstFileAvailable();
	if (index >= 0)
	{
		if (!fileArray[index].open(filename, O_READ | O_WRITE))
		{
			return -1;
		}
		return index+1;
	}
	return -2;
	
#endif
	return 0;
}

void myFileClose(int16_t file)
{
#ifdef USE_SPIFFS
	SPIFFS_close(&fs, file);
#endif
#ifdef INCLUDE_SD
	fileArray[file - 1].close();
	fileArray[file - 1] == NULL;
#endif
}

int32_t myFileWrite(int16_t file, uint8_t * buffer, uint32_t length)
{
	int32_t res;
#ifdef USE_SPIFFS
	// Si ya pas le droit d'écrire y pourra pas grossir le fichier normalement!
	res = SPIFFS_write(&fs, file, buffer, length);
#endif
#ifdef INCLUDE_SD
	res = fileArray[file-1].write(buffer, length);
#endif
	return res;
}

int32_t myFileRead(int16_t file, uint8_t * buffer, uint32_t length)
{
	int32_t res;
#ifdef USE_SPIFFS
	// Si ya pas le droit d'écrire y pourra pas grossir le fichier normalement!
	res = SPIFFS_read(&fs, file, buffer, length);
#endif
#ifdef INCLUDE_SD
	res = fileArray[file - 1].read(buffer, length);
#endif
	return res;
}

int16_t myFileOpenEmpty(const char * filename)
{
	int16_t destinationFile;
#ifdef USE_SPIFFS
	destinationFile = SPIFFS_open(&fs, filename, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	//SPIFFS_fstat(&fs, ff, &s);
#endif
#ifdef INCLUDE_SD
	int index = GetFirstFileAvailable();
	if (index >= 0)
	{
		if ((fileArray[index] = SD.open(filename, O_READ | O_WRITE | O_TRUNC | O_CREAT)) == NULL)
		{
			return -1;
		}
		return index+1;
	}
	return -2;
#endif

	return destinationFile;
}

int32_t myFileSeek(int16_t file, int32_t offset)
{
	int32_t res;
#ifdef USE_SPIFFS
	res = SPIFFS_lseek(&fs, file, offset, SPIFFS_SEEK_SET);
#endif

#ifdef INCLUDE_SD
	fileArray[file-1].seek(offset);
	res = offset;
#endif
	return res;
}
#ifdef USE_SPIFFS
void test_spiffs() 
{
	char buf[12];

	// Surely, I've mounted spiffs before entering here
	Serial.println("Testing spiffs %i\n");

	spiffs_file fd = SPIFFS_open(&fs, "my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
	if (SPIFFS_write(&fs, fd, (u8_t *)"Hello world", 12) < 0)
		Serial.printf("errno %i\n", SPIFFS_errno(&fs));
	SPIFFS_close(&fs, fd);

	fd = SPIFFS_open(&fs, "my_file", SPIFFS_RDWR, 0);
	if (SPIFFS_read(&fs, fd, (u8_t *)buf, 12) < 0)
		Serial.printf("errno %i\n", SPIFFS_errno(&fs));
	SPIFFS_close(&fs, fd);

	Serial.printf("--> %s <--\n", buf);
	uint32_t total, used;
	SPIFFS_info(&fs, &total, &used);
	Serial.printf("Total: %d, Used: %d", total, used);
}
#endif

int32_t myFileGetSize(int16_t file)
{
#ifdef USE_SPIFFS
	spiffs_stat s;
	SPIFFS_fstat(&fs, file, &s);
	return s.sizet;
#endif
#ifdef INCLUDE_SD
	return fileArray[file-1].size();
#endif
}