//https://wiki.osdev.org/Serial_Ports
#ifndef _SERIAL_H
#define _SERIAL_H

//VBox is fucking with the serial check, so we disable it here
//#define _VBOX_COMPAT

#include <omen/hal/arch/x86/cpu.h>

#define SERIAL_MASTER_SPEED 115200
#define SERIAL_READ 1
#define SERIAL_WRITE 2
#define SERIAL_BUFFER_SIZE 1024
#define MAX_COM_DEVICES 2
#define SERIAL_OF_IRQ 0x91
#define IRQ_COM1 0x24
#define IRQ_COM2 0x23
#define IRQ_COM3 0x24
#define IRQ_COM4 0x23
#define DEFAULT_COM1_PORT 0x3f8          // COM1
#define DEFAULT_COM2_PORT 0x2f8          // COM2
#define DEFAULT_COM3_PORT 0x3e8          // COM3
#define DEFAULT_COM4_PORT 0x2e8          // COM4

//Important: dont be dislexic
//Inb: Buffer that hold what the serial port is sending to this computer
//Outb: Buffer that hold what this computer is sending to the serial port

struct serial_subscriber {
    void* parent;
    void (*handler)(void * parent, char c, int port);
    struct serial_subscriber * next;
};

struct serial_configuration {
    int baud_rate;
    int parity;
    int stop_bits;
    int data_bits;
};

struct serial_device {
    int valid;
    int port;
    int irq;
    int interrupts_enabled;
    int echo;

    void (*handler)(cpu_context_t* ctx, uint8_t cpuid);
    struct serial_subscriber * read_subscribers;
    struct serial_subscriber * write_subscribers;
    struct serial_configuration config;
    struct serial_configuration default_config;
    char * inb;
    int inb_size;
    int inb_write;
    int inb_read;

    char * outb;
    int outb_size;
    int outb_write;
    int outb_read;
};

void init_serial(int inbs, int outbs);

struct serial_device* get_serial(int port);
struct serial_device* get_serial_by_comm(int comm);
volatile struct serial_device* get_last_interrupted_serial();
void reconfigure_serial_comm(int port, int baud_rate, int parity, int stop_bits, int data_bits);

void serial_get_ports(int * ports);
int  serial_count_ports();

void serial_echo_enable(int port);
void serial_echo_disable(int port);

void serial_read_event_add(int port, void* parent, void (*handler)(void* parent, char c, int port));
void serial_read_event_remove(int port, void* parent, void (*handler)(void* parent, char c, int port));

void serial_write_event_add(int port, void* parent, void (*handler)(void* parent, char c, int port));
void serial_write_event_remove(int port, void* parent, void (*handler)(void* parent, char c, int port));

void serial_discard(int port);

void _serial_flush(int port);
void _serial_write(int port, char c);
char _serial_read(int port);

void write_inb(struct serial_device* device, char c);
char read_inb(struct serial_device* device);
void write_outb(struct serial_device* device, char c);
char read_outb(struct serial_device* device);

#endif