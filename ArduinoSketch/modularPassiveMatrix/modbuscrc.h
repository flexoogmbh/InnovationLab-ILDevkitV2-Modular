#ifndef __MODBUS_CRC__
#define __MODBUS_CRC__

#ifdef __cplusplus
 extern "C" {
#endif

unsigned short mbCrc(unsigned char const *buf, unsigned short size);
unsigned short mbCrcExt(unsigned short prev_crc, unsigned char const *buf, unsigned short size);
unsigned short mbFastCrc(unsigned char *chkbuf, unsigned short len);

#ifdef __cplusplus
 }
#endif

#endif
