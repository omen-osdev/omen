#ifndef _X86_INT_H
#define _X86_INT_H

#include <omen/libraries/std/stdint.h>

struct rflags{
    uint8_t CF:1;
    uint8_t Reserved0:1; //this is bit is set as default
    uint8_t PF:1;
    uint8_t Reserved1:1;
    uint8_t AF:1;

    uint8_t Reserved2:1;
    uint8_t ZF:1;
    uint8_t SF:1;
    uint8_t TF:1;
    uint8_t IF:1;

    uint8_t DF:1;
    uint8_t OF:1;
    uint8_t IOPL:2;
    uint8_t NT:1;
    uint8_t Reserved3:1;
    uint8_t RF:1;

    uint8_t VM:1;
    uint8_t AC:1;
    uint8_t VIF:1;
    uint8_t VIP:1;
    uint8_t ID:1;

    uint8_t Reserved4:1;
    uint8_t Reserved5:1;
    uint8_t Reserved6:1;
    uint8_t Reserved7:1;
    uint8_t Reserved8:1;

    uint8_t Reserved9:1;
    uint8_t Reserved10:1;
    uint8_t Reserved11:1;
    uint8_t Reserved12:1;
    uint8_t Reserved13:1;

    uint32_t Reserved14;
}__attribute__((packed));

struct interrupt_frame {   
    uint64_t rip; 
    uint64_t cs; 
    struct rflags rflags; 
    uint64_t rsp; 
    uint64_t ss;
}__attribute__((packed)); 

struct interrupt_frame_error {   
    uint64_t error_code;
    uint64_t rip; 
    uint64_t cs; 
    struct rflags rflags; 
    uint64_t rsp; 
    uint64_t ss;
}__attribute__((packed)) int_error_frame_t;

#endif