#include "serial.h"


struct file_operations serial_fops = {
    .read = serial_dd_read,
    .write = serial_dd_write,
    .ioctl = serial_dd_ioctl
};


void serial_confiure_baud_rate(uint16_t com, uint16_t divisor)
{
    outb(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
    outb(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF);
    outb(SERIAL_DATA_PORT(com), divisor & 0x00FF);
}

void serial_configure_line(uint16_t com)
{
    outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

void serial_configure_fifo_buffer(uint16_t com)
{
    outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

void serial_configure_modem(uint16_t com)
{
    outb(SERIAL_MODEM_COMMAND_PORT(com), 0x0B);
}

uint64_t serial_is_transmit_fifo_empty(unsigned short com) 
{
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

void serial_write_byte(uint16_t com, char byte_data) 
{
    outb(SERIAL_DATA_PORT(com), byte_data);
}

uint64_t serial_dd_data_rcvd(uint16_t com)
{
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 1;
}

char* create_serial_dd(uint16_t com_base_addr, uint16_t baud_rate)
{
    outb(com_base_addr + 1, 0x00);                          // disable all interrupts

    serial_confiure_baud_rate(com_base_addr, baud_rate);    // set the baud rate
    serial_configure_line(com_base_addr);                   // 8 bits, no parity, one stop bit
    serial_configure_fifo_buffer(com_base_addr);            // enable FIFO, clear them, with 14-byte threshold
    serial_configure_modem(com_base_addr);                  // IRQs enabled, RTS/DSR set


    //Test the serial chip
    outb(SERIAL_MODEM_COMMAND_PORT(com_base_addr), 0x1E);   // Set in loopback mode, test the serial chip
    
    outb(com_base_addr, 0xAE);      // send byte 0xAE and check if serial returns same byte
    
    //return 1 if serial is faulty
    if(inb(COM2_BASEADDR + 0) != 0xAE)
    {
        return NULL;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_MODEM_COMMAND_PORT(com_base_addr), 0x0F);
    return device_create(NULL, DEVICE_SERIAL, com_base_addr);
}

status_t init_serial_dd()
{
    return register_char(DEVICE_SERIAL, SERIAL_DD_NAME, &serial_fops); 
}

uint64_t serial_dd_write(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer)
{
    (void)offset;
    (void)size;
    while(!serial_is_transmit_fifo_empty((uint16_t)id));
    serial_write_byte((uint16_t)id, (char)buffer[0]);
    return 0;
}


uint64_t serial_dd_write_string(uint16_t com, char *buf, size_t len)
{
    size_t idx = 0;
    while(idx < len)
    {
        serial_dd_write((uint64_t)com, 1, 0, (uint8_t*)&(buf[idx]));
        idx++;
    }
    return 0;
}

uint64_t serial_dd_read(uint64_t id, uint64_t size, uint64_t offset, uint8_t* buffer)
{
    (void)offset;
    (void)size;
    while(!serial_dd_data_rcvd((uint16_t)id));
    buffer[0] = inb((uint16_t)id);
    return 0;
}

uint64_t serial_dd_ioctl(uint64_t id, uint32_t request, void* data)
{
    switch(request)
    {
        case SERIAL_IOCTL_CHANGE_BAUDRATE:
            serial_confiure_baud_rate((uint16_t)id, *((uint16_t*)data));
            break;
        case SERIAL_IOCTL_CHANGE_LINECONFIG:
            //TODO: Implement this(for example 8N1, etc)
            break;
        case SERIAL_IOCTL_ENABLE_LOOPBACK:
            outb(SERIAL_MODEM_COMMAND_PORT((uint16_t)id), 0x1E);
            break;
        case SERIAL_IOCTL_DISABLE_LOOPBACK:
            outb(SERIAL_MODEM_COMMAND_PORT((uint16_t)id), 0x0F);
            break;
        default:
            break;
    }
    return 0;
}
