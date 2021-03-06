#pragma once

#include <stdint.h>
typedef uint8_t ErrCode;

// The following error codes match the LFS error codes in lfs.h, but negated
#define ERR_NOENT           2   
#define ERR_IO              5   
#define ERR_BADF            9   
#define ERR_NOMEM           12 
#define ERR_EXIST           17  
#define ERR_NOTDIR          20  
#define ERR_ISDIR           21  
#define ERR_INVAL           22  
#define ERR_FBIG            27  
#define ERR_NOSPC           28 
#define ERR_NAMETOOLONG     36  
#define ERR_NOTEMPTY        39  
#define ERR_NOATTR          61 
#define ERR_CORRUPT         84  

// The following error codes are not related to file handling
#define ERR_BADARGS         100
#define ERR_LINETOOLONG     101
#define ERR_ABANDONED       102
#define ERR_BADCOMMAND      103
#define ERR_USAGE           104
#define ERR_YMODEM          105
#define ERR_INTERRUPTED     106
#define ERR_NOTIMPLEMENTED  107
#define ERR_BADPIN          108
#define ERR_NOTEXECUTABLE   109



