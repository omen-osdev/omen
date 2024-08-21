#include "serial.h"


struct file_operations serial_fops = {
    //.read = serial_dd_read,
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

int serial_is_transmit_fifo_empty(unsigned short com) 
{
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

void serial_write_byte(uint16_t com, char byte_data) 
{
    outb(SERIAL_DATA_PORT(com), byte_data);
}

int init_serial_dd(uint16_t com_base_addr, uint16_t baud_rate)
{
    register_char(DEVICE_SERIAL, SERIAL_DD_NAME, &serial_fops);


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
        return 0;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_MODEM_COMMAND_PORT(com_base_addr), 0x0F);

    return 1;
}


int serial_dd_write(uint16_t com, char *buf, size_t len)
{
    size_t idx = 0;
    while(idx < len)
    {
        if(serial_is_transmit_fifo_empty(com))
        {
            serial_write_byte(com, buf[idx]);
            idx++;
        }
    }
    return 0;
}




/*
int serial_received()
{
    return inb(COM2_BASEADDR + 5) & 1;
}

char serial_read()
{
    while(serial_received() == 0);

    return inb(COM2_BASEADDR);
}
*/
void serial_dd_ioctl()
{

}