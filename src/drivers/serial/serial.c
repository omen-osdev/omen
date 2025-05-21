#include "serial.h"

#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/cpu.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>

//Just for visibility
void _serial_flush(int port);
void serial_discard(int port);

void COM1_HANDLER(cpu_context_t* ctx, uint8_t cpuid);
void COM2_HANDLER(cpu_context_t* ctx, uint8_t cpuid);
void COM3_HANDLER(cpu_context_t* ctx, uint8_t cpuid);
void COM4_HANDLER(cpu_context_t* ctx, uint8_t cpuid);


volatile struct serial_device* interrupted_serial_device = 0;

//The handler for a dynamic interrupt cannot have __attribute__((interrupt))
void SERIAL_OF_HANDLER(cpu_context_t* ctx, uint8_t cpuid) {
   (void)ctx;
   (void)cpuid;
   //Warning: Buffered input from serial device will be lost
   if (interrupted_serial_device) {
      _serial_flush(interrupted_serial_device->port);
      serial_discard(interrupted_serial_device->port);
   } else {
      kprintf("[WARNIG] Serial overflow interrupt, but cant find device to flush\n");
   }
}

//TODO: This are not the correct handlers!
struct serial_device serial_devices[] = {
   {
      .port = DEFAULT_COM1_PORT,
      .irq = IRQ_COM1,
      .handler = COM1_HANDLER,
      .read_subscribers = 0,
      .write_subscribers = 0,
      .valid = 0
   },
   {
      .port = DEFAULT_COM2_PORT,
      .irq = IRQ_COM2,
      .handler = COM2_HANDLER,
      .read_subscribers = 0,
      .write_subscribers = 0,
      .valid = 0
   },
   {
      .port = DEFAULT_COM3_PORT,
      .irq = IRQ_COM3,
      .handler = COM3_HANDLER,
      .read_subscribers = 0,
      .write_subscribers = 0,
      .valid = 0
   },
   {
      .port = DEFAULT_COM4_PORT,
      .irq = IRQ_COM4,
      .handler = COM4_HANDLER,
      .read_subscribers = 0,
      .write_subscribers = 0,
      .valid = 0
   }
};

uint8_t initialized = 0;

int serial_received(int port) {
   return inb(port + 5) & 1;
}
 
char read_serial(int port) {
   while (serial_received(port) == 0);
 
   return inb(port);
}

int is_transmit_empty(int port) {
   return inb(port + 5) & 0x20;
}
 
void write_serial(int port, char a) {
   while (is_transmit_empty(port) == 0);
   outb(port,a);
}

void add_subscriber(struct serial_device* device, uint8_t type, void* parent, void* handler) {
   struct serial_subscriber* subscriber;
   if (type == SERIAL_READ) {
      subscriber = device->read_subscribers;
   } else if (type == SERIAL_WRITE) {
      subscriber = device->write_subscribers;
   } else {
      return;
   }

   while (subscriber->next) {
      subscriber = subscriber->next;
   }

   subscriber->next = kmalloc(sizeof(struct serial_subscriber));
   subscriber->next->handler = handler;
   subscriber->next->parent = parent;
   subscriber->next->next = 0;
}

void remove_subscriber(struct serial_device* device, uint8_t type, void* parent, void* handler) {
   struct serial_subscriber* subscriber;
   if (type == SERIAL_READ) {
      subscriber = device->read_subscribers;
   } else if (type == SERIAL_WRITE) {
      subscriber = device->write_subscribers;
   } else {
      return;
   }
   
   while (subscriber->next) {
      if (subscriber->next->handler == handler && subscriber->next->parent == parent) {
         struct serial_subscriber* to_remove = subscriber->next;
         subscriber->next = subscriber->next->next;
         kfree(to_remove);
         return;
      }
      subscriber = subscriber->next;
   }
}

void run_subscribers(struct serial_device* device, uint8_t type, char c) {
   struct serial_subscriber* subscriber;
   if (type == SERIAL_READ) {
      subscriber = device->read_subscribers;
   } else if (type == SERIAL_WRITE) {
      subscriber = device->write_subscribers;
   } else {
      return;
   }

   while (subscriber->next) {
      subscriber = subscriber->next;
      subscriber->handler(subscriber->parent, c, device->port);
   }
}

void write_inb(struct serial_device* device, char c) {
   device->inb[device->inb_write++] = c;
   if (device->inb_write >= device->inb_size) device->inb_write = 0;
   if (device->inb_write == device->inb_read) {
      raise_interrupt(SERIAL_OF_IRQ);
   }
   if (device->echo) write_serial(device->port, c);
   run_subscribers(device, SERIAL_READ, c);
}

char read_inb(struct serial_device* device) {
   
   if (device->inb_read == device->inb_write) return 0; //No data available
   char c = device->inb[device->inb_read++];
   if (device->inb_read >= device->inb_size) device->inb_read = 0;
   //run_subscribers(device, SERIAL_WRITE, c);
   return c;
}

void write_outb(struct serial_device* device, char c) {
   device->outb[device->outb_write++] = c;
   if (device->outb_write >= device->outb_size) device->outb_write = 0;
   if (device->outb_write == device->outb_read) {
      raise_interrupt(SERIAL_OF_IRQ);
   }
   //run_subscribers(device, SERIAL_WRITE, c);
}

char read_outb(struct serial_device* device) {
   if (device->outb_read == device->outb_write) return 0; //No data available
   char c = device->outb[device->outb_read++];
   if (device->outb_read >= device->outb_size) device->outb_read = 0;
   run_subscribers(device, SERIAL_WRITE, c);
   return c;
}

void COM1_HANDLER(cpu_context_t* ctx, uint8_t cpuid) {
   (void)ctx;
   (void)cpuid;
   struct serial_device* device = &(serial_devices[0]);
   if (!device->valid) return;
   interrupted_serial_device = device;
   char c = read_serial(device->port);
   write_inb(device, c);
}

void COM2_HANDLER(cpu_context_t* ctx, uint8_t cpuid) {
   (void)ctx;
   (void)cpuid;
   struct serial_device* device = &(serial_devices[1]);
   if (!device->valid) return;
   interrupted_serial_device = device;
   char c = read_serial(device->port);
   write_inb(device, c);
}

void COM3_HANDLER(cpu_context_t* ctx, uint8_t cpuid) {
   (void)ctx;
   (void)cpuid;
   struct serial_device* device = &(serial_devices[2]);
   if (!device->valid) return;
   interrupted_serial_device = device;
   char c = read_serial(device->port);
   write_inb(device, c);
}

void COM4_HANDLER(cpu_context_t* ctx, uint8_t cpuid) {
   (void)ctx;
   (void)cpuid;
   struct serial_device* device = &(serial_devices[3]);
   if (!device->valid) return;
   interrupted_serial_device = device;
   char c = read_serial(device->port);
   write_inb(device, c);
}

struct serial_device* get_serial_by_comm(int comm) {
   if (!initialized) return 0;
   if (comm < 0 || comm >= MAX_COM_DEVICES) return 0;
   if (!serial_devices[comm].valid) return 0;
   return &(serial_devices[comm]);
}

struct serial_device* get_serial(int port) {
   if (!initialized) return 0;

   for (int i = 0; i < MAX_COM_DEVICES; i++) {
      if (serial_devices[i].valid && serial_devices[i].port == port) {
         return &(serial_devices[i]);
      }
   }
   return 0;
}

volatile struct serial_device* get_last_interrupted_serial() {
   return interrupted_serial_device;
}

void reconfigure_serial_comm(int port, int baud_rate, int parity, int stop_bits, int data_bits) {
   struct serial_device* device = get_serial(port);
   if (!device) return;
   
   device->config.baud_rate = baud_rate;
   device->config.parity = parity;
   device->config.stop_bits = stop_bits;
   device->config.data_bits = data_bits;
   
   uint64_t divisor = SERIAL_MASTER_SPEED / baud_rate;
   outb(port + 1, 0x00);    // Disable all interrupts
   outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(port + 0, (divisor & 0xFF));    // Set divisor to baud rate (lo byte)
   outb(port + 1, (divisor >> 8));    //                  (hi byte)
   outb(port + 3, (data_bits << 0) | (parity << 3) | (stop_bits << 2));    // Set data bits, parity and stop bits
   outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   //Enable interrupts
   if (device->interrupts_enabled) {
      outb(port + 1, 0x01);    // Enable all interrupts
   }
}

int init_serial_comm(int port) {

   outb(port + 1, 0x00);    // Disable all interrupts
   outb(port + 3, 0x80);    // Enable DLAB (set baud rate divisor)
   outb(port + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
   outb(port + 1, 0x00);    //                  (hi byte)
   outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
   outb(port + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
   outb(port + 4, 0x0B);    // IRQs enabled, RTS/DSR set
   outb(port + 4, 0x1E);    // Set in loopback mode, test the serial chip
   outb(port + 0, 0xAB);    // Test serial chip (send byte 0xAE and check if serial returns same byte)
 
   // Check if serial is faulty (i.e: not same byte as sent)
   if(inb(port + 0) != 0xAB) {
      kprintf("Serial port %x seems to be down\n", port);
      #ifndef _VBOX_COMPAT
      return 0;
      #endif
   }
 
   // If serial is not faulty set it in normal operation mode
   outb(port + 4, 0x0F);

   return 1;
}

void enable_serial_int(struct serial_device* device) {
   if (device) {
      hook_interrupt(device->irq, device->handler);
      unmask_interrupt(device->irq);
      outb(device->port + 1, 0x01);    // Enable all interrupts
      device->interrupts_enabled = 1;
   }
}
 
void disable_serial_int(struct serial_device* device) {
   if (device) {
      mask_interrupt(device->irq);
      unhook_interrupt(device->irq);
      outb(device->port + 1, 0x00);    // Disable all interrupts
      device->interrupts_enabled = 0;
   }
}

void init_serial(int inbs, int outbs) {
   if (initialized) return;
   kprintf("### Initializing serial devices ### \n");
   for (int i = 0; i < MAX_COM_DEVICES; i++) {
      struct serial_device* device = &(serial_devices[i]);
      if (init_serial_comm(device->port)) {
         device->interrupts_enabled = 0;
         device->inb_size = inbs;
         device->outb_size = outbs;
         device->inb = (char*)kmalloc(inbs);
         memset(device->inb, 0, inbs);
         device->outb = (char*)kmalloc(outbs);
         memset(device->outb, 0, outbs);
         device->echo = 0;
         device->inb_read = 0;
         device->outb_read = 0;
         device->inb_write = 0;
         device->outb_write = 0;
         device->default_config.baud_rate = 115200;
         device->default_config.parity = 0;
         device->default_config.stop_bits = 1;
         device->default_config.data_bits = 8;
         device->config.baud_rate = device->default_config.baud_rate;
         device->config.parity = device->default_config.parity;
         device->config.stop_bits = device->default_config.stop_bits;
         device->config.data_bits = device->default_config.data_bits;
         device->read_subscribers = kmalloc(sizeof(struct serial_subscriber));
         device->write_subscribers = kmalloc(sizeof(struct serial_subscriber));
         device->read_subscribers->next = 0;
         device->write_subscribers->next = 0;
         device->read_subscribers->handler = 0;
         device->write_subscribers->handler = 0;

         if (device->inb && device->outb) {
            enable_serial_int(device);
            device->valid = 1;
            kprintf("Serial port %x initialized successfully\n", device->port);
            serial_discard(device->port);
            reconfigure_serial_comm(device->port, device->config.baud_rate, device->config.parity, device->config.stop_bits, device->config.data_bits);
         }
      }
   }
   hook_interrupt(SERIAL_OF_IRQ, SERIAL_OF_HANDLER);
   initialized = 1;
}

void serial_echo_enable(int port) {
   if (!initialized) return;
   struct serial_device* device = get_serial(port);
   if (!device) return;

   device->echo = 1;

}

void serial_discard(int port) {
   if (!initialized) return;
   struct serial_device* device = get_serial(port);
   if (!device) return;

   memset(device->inb, 0, device->inb_size);
   memset(device->outb, 0, device->outb_size);

   device->inb_read = device->inb_write;
   device->outb_read = device->outb_write;
}

void serial_echo_disable(int port) {
   if (!initialized) return;
   struct serial_device* device = get_serial(port);
   if (!device) return;

   device->echo = 0;

}

void serial_get_ports(int * ports) {
   if (!initialized) return;
   for (int i = 0; i < MAX_COM_DEVICES; i++) {
      if (serial_devices[i].valid) {
         *ports = serial_devices[i].port;
         ports++;
      }
   }
}

int serial_count_ports() {
   if (!initialized) return 0;
   int valid = 0;
   for (int i = 0; i < MAX_COM_DEVICES; i++) {
      if (serial_devices[i].valid) {
         valid++;
      }
   }
   return valid;
}

void serial_read_event_add(int port, void* parent, void (*handler)(void* parent, char c, int port)) {
   if (!initialized) return;

   struct serial_device* device = get_serial(port);
   if (!device) return;

   add_subscriber(device, SERIAL_READ, parent, handler);
}

void serial_read_event_remove(int port, void* parent, void (*handler)(void* parent, char c, int port)) {
   if (!initialized) return;

   struct serial_device* device = get_serial(port);
   if (!device) return;

   remove_subscriber(device, SERIAL_READ, parent, handler);
}

void serial_write_event_add(int port, void* parent, void (*handler)(void* parent, char c, int port)) {
   if (!initialized) return;

   struct serial_device* device = get_serial(port);
   if (!device) return;

   add_subscriber(device, SERIAL_WRITE, parent, handler);
}

void serial_write_event_remove(int port, void* parent, void (*handler)(void* parent, char c, int port)) {
   if (!initialized) return;

   struct serial_device* device = get_serial(port);
   if (!device) return;

   remove_subscriber(device, SERIAL_WRITE, parent, handler);
}

void _serial_flush(int port) {
   if (!initialized) return;

   struct serial_device* device = get_serial(port);
   if (!device) return;

   char c = read_outb(device);
   while (c != 0) {
      write_serial(port, c);
      c = read_outb(device);
   }
}

void _serial_write(int port, char c) {
   if (!initialized) return;

   struct serial_device* device = get_serial(port);
   if (!device) return;
   write_outb(device, c);
}

char _serial_read(int port) {
   if (!initialized) return 0;

   struct serial_device* device = get_serial(port);
   if (!device) return 0;

   return read_inb(device);
}