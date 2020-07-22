#ifndef __PACKET_H__
#define __PACKET_H__

typedef unsigned char  u8_t;
typedef unsigned short u16_t;
typedef unsigned int   u32_t;



struct yzy_packet{
    u16_t  version_chief;       //主版本号       例如  0x0001
    u16_t  version_sub;         //次要版本号     例如  0x0001
    u32_t  service_code;        //请求的服务编号  例如 0xff00ff01
    u32_t  request_code;        //请求编号       例如 0x00000001  用于区别每个链接每次请求的唯一性
    u32_t  dataSize;            //数据域大小     例如 0x00001000
    u8_t   dataType;            //数据段类型      例如 ox00:二进制数据 0x01:json 0x02:protobuf
    u8_t   encoding;            //数据段压缩方式  例如 ox00:无压缩
    u8_t   clientType;          //终端类型       例如 0x01: uefi 0x02:linux  0x03: windows 0x04: server
    u8_t   reqOrRes;            //请求还是响应   例如 0x01: 请求 0x02: 响应
    u16_t  tokenLength;         //token长度     没有就是 0x0000
    u16_t  supplementary;       //补充协议头长度  没有就是 0x0000
}__attribute((aligned (1)));



typedef struct yzy_packet yzy_packet;


#define BYTE_TYPE 0x00
#define JSON_TYPE 0x01
#define PROTOBUF_TYPE 0x02


#endif
