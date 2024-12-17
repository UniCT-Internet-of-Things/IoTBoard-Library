#ifndef FRAME_DEF_H
#define FRAME_DEF_F
#include "Arduino.h"


typedef union 
{ 
    uint16_t framecontrol;

    struct
    {
        uint16_t FrameType          :3;
        uint16_t SecurityEnabled    :1;
        uint16_t FramePending       :1;
        uint16_t AckRequest         :1;
        uint16_t PanIdCompression   :1;
        uint16_t                    :1;
        uint16_t sequenceNumSuppr   :1;
        uint16_t IeField            :1;    
        uint16_t DestAddrMode       :2;
        uint16_t FrameVersion       :2;
        uint16_t SrcAddrMode        :2;
    };

}FrameControl;


typedef union
{

    struct data
    {
        FrameControl fcf;
        uint8_t num_seq;
        uint16_t dest_panid;
        uint16_t dest_addr;
        uint16_t src_addr;
        uint8_t payload[116];
        uint16_t fcs;

    }DataFrame;

    struct ack
    {
        FrameControl fcf;
        uint8_t num_seq;
        uint16_t fcs;

    }AckFrame;
    
}frame_t;


typedef struct 
{
    frame_t frame;
    uint8_t lqi;
    uint8_t rssi;

}rx_frame;



#endif