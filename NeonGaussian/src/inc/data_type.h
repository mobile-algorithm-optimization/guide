#ifndef DATA_TYPE_H_
#define DATA_TYPE_H_

typedef int int32_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;

#define CAST_U8(t)               (uint8_t)(!((t) & ~255) ? (t) : (t) > 0 ? 255 : 0)

#endif
