// OMEN Serial driver


#ifndef _SERIAL_DRIVER_H
#define _SERIAL_DRIVER_H

#include "../../include/omen/hal/arch/x86/io.h"
#include "../../include/omen/managers/dev/devices.h"


#define DEVICE_SERIAL 0x8d
#define SERIAL_DD_NAME "serial"


//Base addresses of the COM ports
#define COM1_BASEADDR 0x3F8
#define COM2_BASEADDR 0x2F8 


//Offsets into COM base addresses
#define SERIAL_DATA_PORT(base)          (base + 0)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)


//IOCTL Commands
#define SERIAL_IOCTL_CHANGE_BAUDRATE    0
#define SERIAL_IOCTL_CHANGE_LINECONFIG  1
#define SERIAL_IOCTL_ENABLE_LOOPBACK    2
#define SERIAL_IOCTL_DISABLE_LOOPBACK   3

//Divisors for the different baud rates
enum baud_rate { baud_115200 = 1, baud_57600 = 2, baud_38400 = 3, baud_19200 = 6, baud_9600 = 12, baud_4800 = 24 };


#define SERIAL_LINE_ENABLE_DLAB 0x80


extern struct file_operations serial_fops;


char* create_serial_dd(uint16_t com_base_addr, uint16_t baud_rate);
status_t init_serial_dd();
uint64_t serial_dd_write(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer);
uint64_t serial_dd_write_string(uint16_t com, char *buf, size_t len);
uint64_t serial_dd_ioctl(uint64_t id, uint32_t request, void* data);
uint64_t serial_dd_data_rcvd(uint16_t com);
uint64_t serial_dd_read(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer);


#endif