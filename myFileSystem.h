#ifndef MYFILESYSTEM_H
#define MYFILESYSTEM_H
#include "generalDefines.h"

int  myFileSystemInit();
void test_spiffs();

int16_t myFileOpen(const char * filename);
int16_t myFileOpenEmpty(const char * filename);
int32_t myFileRead(int16_t file, uint8_t * buffer, uint32_t length);
int32_t myFileWrite(int16_t file, uint8_t * buffer, uint32_t length);
int32_t myFileSeek(int16_t file, int32_t offset);
//int32_t myFileGetError();
void myFileClose(int16_t file);
int32_t myFileGetSize(int16_t file);
#endif