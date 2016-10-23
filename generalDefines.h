#ifndef GENERAL_DEFS_H
#define GENERAL_DEFS_H

#define CS_MEMORY A7
//#define USE_SPIFFS
#define INCLUDE_SD

#ifndef NumberOfElements
#define NumberOfElements(array)     ((sizeof(array) / (sizeof(array[0]))))
#endif


#endif // GENERAL_DEFS_H