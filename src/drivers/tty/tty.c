//This is perhaps the world's worst TTY implementation
//Don't use it for anything serious, or even for fun

#include "tty.h"
#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/termios.h>
#include <omen/managers/dev/devices.h>
#include <omen/managers/cpu/sline.h>
#include <omen/libraries/allocators/heap_allocator.h>

//TODO: This dependencies are awful
#include <serial/serial_dd.h>
#include <ps2/ps2.h>

struct tty ttys[32] = {0};
const char noindev[] = "none";
const char nooutdev[] = "none";

void _tty_check_termios_support(struct termios* termios) {
    if (termios == 0) return;

    //Check if the termios structure is supported
    if (termios->c_iflag & ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF | IMAXBEL | IUTF8)) { 
        kprintf("Unsupported termios input flag\n");
    }
    if (termios->c_oflag & ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR | ONLRET | OFILL | OFDEL | NLDLY | NL0 | NL1 | CRDLY | CR0 | CR1 | CR2 | CR3 | TABDLY | TAB0 | TAB1 | TAB2 | TAB3 | BSDLY | BS0 | BS1 | FFDLY | FF0 | FF1 | VTDLY | VT0 | VT1)) {
        kprintf("Unsupported termios output flag\n");
    }
    if (termios->c_cflag & ~(CSIZE | CS5 | CS6 | CS7 | CS8 | CSTOPB | CREAD | PARENB | PARODD | HUPCL | CLOCAL)) {
        kprintf("Unsupported termios control flag\n");
    }
    if (termios->c_lflag & ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH | TOSTOP | IEXTEN | EXTA | EXTB | CBAUD | CBAUDEX | CIBAUD | CMSPAR | CRTSCTS | XCASE | ECHOCTL | ECHOPRT | ECHOKE | FLUSHO | PENDIN | EXTPROC | XTABS)) {
        kprintf("Unsupported termios local flag\n");
    }

    if (termios->c_line != LD_DEFAULT_TABLE) {
        panic("Unsupported termios line discipline\n");
    }
}

void _tty_set_termios(struct tty* tty, struct termios* termios) {
    if (tty == 0 || !tty->valid) return;
    if (termios == 0) return;

    _tty_check_termios_support(termios);
    //Set the termios structure
    memcpy(&(tty->termios), termios, sizeof(struct termios));
    _tty_modes(tty, _TTY_MODE_ECHO, termios->c_lflag & ECHO);
    _tty_modes(tty, _TTY_MODE_RAW, termios->c_lflag & ICANON);
}

struct termios * _tty_get_termios(struct tty* tty) {
    if (tty == 0 || !tty->valid) return;
    return &(tty->termios);
}

struct tty* get_tty(int index) {
    if (index < 0 || index >= 32) return 0;
    return &ttys[index];
}

void tty_run_subscribers(struct tty* tty, uint8_t event) {
    if (tty == 0 || !tty->valid) return;
    if (tty->subscribers == 0) return;

    struct tty_subscriber * subscriber = tty->subscribers;

    while(subscriber->next) {
        subscriber = subscriber->next;
        subscriber->handler((void*)tty, event);
    }
}

void _tty_flush(struct tty* device) {
    if (device == 0) return;
    if (device->outb == 0) return;

    if (device->outb_read == device->outb_write) return;

    //This avoids using read_outb, not sure if it's a good idea
    int len = device->outb_write - device->outb_read;
    if (len < 0) {
        len = device->outb_size - device->outb_read;
    }

    device_write(device->outdev, len, 0, (uint8_t*)(device->outb + device->outb_read));
    device->outb_read += len;
    if (device->outb_read >= device->outb_size) {
        device->outb_read = 0;
    }
    if (device->mode == TTY_MODE_SERIAL)
        device_ioctl(device->outdev, SERIAL_FLUSH_TX, 0);
    else
        tty_run_subscribers(device, TTY_EVENT_FLUSH_OUTB);
}

void tty_write_inb(struct tty* device, char c) {
    if (device == 0) return;
    if (device->inb == 0) return;

    device->inb[device->inb_write] = c;
    device->inb_write++;
    if (device->inb_write >= device->inb_size) {
        device->inb_write = 0;
    }

    if (device->inb_write == device->inb_read) {
        device->inb_read++;
        if (device->inb_read >= device->inb_size) {
            device->inb_read = 0;
        }
    }
}

void tty_write_outb(struct tty* device, char c) {
    if (device == 0) return;
    if (device->outb == 0) return;

    device->outb[device->outb_write] = c;
    device->outb_write++;
    if (device->outb_write >= device->outb_size) {
        device->outb_write = 0;
    }

    if (device->outb_write == device->outb_read) {
        device->outb_read++;
        if (device->outb_read >= device->outb_size) {
            device->outb_read = 0;
        }
    }

    if (c == TTY_FLUSH_CHAR) {
        _tty_flush(device);
        tty_run_subscribers(device, TTY_EVENT_OUTB);
    }
}

void tty_read_inb(struct tty* device, char* c) {
    if (device == 0) return;
    if (device->inb == 0) return;

    if (device->inb_read == device->inb_write) {
        *c = 0;
        return;
    }

    *c = device->inb[device->inb_read];
    device->inb_read++;
    if (device->inb_read >= device->inb_size) {
        device->inb_read = 0;
    }
}

void tty_read_outb(struct tty* device, char* c) {
    if (device == 0) return;
    if (device->outb == 0) return;

    if (device->outb_read == device->outb_write) {
        *c = 0;
        return;
    }

    *c = device->outb[device->outb_read];
    device->outb_read++;
    if (device->outb_read >= device->outb_size) {
        device->outb_read = 0;
    }
}

void tty_read_cb(void* ttyb, char c, int port) {
    if (ttyb == 0) return;
    struct tty* tty = (struct tty*)ttyb;
    if (!is_valid_tty(tty)) return;
    line_discipline_read(tty->line_discipline[tty->termios.c_line], c);
    wakeup(TTY_IO_SLINE);
}

//TODO: Maybe it is best for the line discipline to send itself and contain a field with the tty! idk...
void flush_cb(void* ttyb, char* buffer, int size) {
    struct tty* tty = (struct tty*)ttyb;
    for (int i = 0; i < size; i++) {
        tty_write_inb(tty, buffer[i]);
    }
    tty_run_subscribers(tty, TTY_EVENT_INB);
}

void echo_cb(void* ttyb, char c) {
    struct tty* tty = (struct tty*)ttyb;
    if (!tty->echo) return;
    device_write(tty->outdev, 1, 0, (uint8_t*)&c);
    if (tty->mode == TTY_MODE_SERIAL)
        device_ioctl(tty->outdev, SERIAL_FLUSH_TX, 0);
}

void _tty_add_subscriber(struct tty* tty, void (*handler)(void*, uint8_t)) {
    if (tty == 0 || !tty->valid) return;
    if (tty->subscribers == 0) return;

    struct tty_subscriber * subscriber = tty->subscribers;

    while(subscriber->next) {
        subscriber = subscriber->next;
    }

    subscriber->next = kmalloc(sizeof(struct tty_subscriber));
    subscriber->next->next = 0;
    subscriber->next->handler = handler;
}

void _tty_remove_subscriber(struct tty* tty, void (*handler)(void*, uint8_t)) {
    if (tty == 0 || !tty->valid) return;
    if (tty->subscribers == 0) return;

    struct tty_subscriber * subscriber = tty->subscribers;

    while(subscriber->next) {
        if (subscriber->handler == handler) {
            struct tty_subscriber * to_remove = subscriber->next;
            subscriber->next = subscriber->next->next;
            kfree(to_remove);
            return;
        }
        subscriber = subscriber->next;
    }
}

void tty_destroy(struct tty* tty) {
    if (tty == 0 || !tty->valid) return;
    if (tty->inb) kfree(tty->inb);
    if (tty->outb) kfree(tty->outb);
    if (tty->subscribers) kfree(tty->subscribers);
    for (int i = 0; i < MAX_LD; i++) {
        if (tty->line_discipline[i]) {
            line_discipline_destroy(tty->line_discipline[i]);
            tty->line_discipline[i] = 0;
        }
    }
    tty->valid = 0;
}

int tty_init(char* indev, char* outdev, int mode, int inbs, int outbs) {
    if (indev == 0) return -1;
    if (outdev == 0) return -1;
    struct tty * tty = 0;   
    int index = 0;
    for (index = 0; index < 32; index++) {
        if (ttys[index].valid == 0) {
            tty = &ttys[index];
            break;
        }
    }
    if (tty == 0) return -1;
    if (mode == TTY_MODE_SERIAL || mode == TTY_MODE_PTY) tty->mode = mode;
    else return -1;
    
    tty->valid = 0;
    strncpy(tty->indev, indev, 32);
    strncpy(tty->outdev, outdev, 32);
    
    tty->termios.c_iflag = 0;
    tty->termios.c_oflag = 0;
    tty->termios.c_cflag = 0;
    tty->termios.c_lflag = 0;
    tty->termios.c_line = LD_DEFAULT_TABLE;
    tty->termios.c_cc[VINTR] = 0x03;
    tty->termios.c_cc[VQUIT] = 0x1C;
    tty->termios.c_cc[VERASE] = 0x7F;
    tty->termios.c_cc[VKILL] = 0x15;
    tty->termios.c_cc[VEOF] = 0x04;
    tty->termios.c_cc[VTIME] = 0x00;
    tty->termios.c_cc[VMIN] = 0x01;
    tty->termios.c_cc[VSWTC] = 0x00;
    tty->termios.c_cc[VSTART] = 0x11;
    tty->termios.c_cc[VSTOP] = 0x13;
    tty->termios.c_cc[VSUSP] = 0x1A;
    tty->termios.c_cc[VEOL] = 0x00;
    tty->termios.c_cc[VREPRINT] = 0x00;
    tty->termios.c_cc[VDISCARD] = 0x00;
    tty->termios.c_cc[VWERASE] = 0x17;
    tty->termios.c_cc[VLNEXT] = 0x16;
    tty->termios.c_cc[VEOL2] = 0x00;
    tty->termios.ibaud = 9800;
    tty->termios.obaud = 9800;

    tty->signal = 0;
    tty->inb = (char*)kmalloc(inbs);
    if (tty->inb == 0) return -1;
    memset(tty->inb, 0, inbs);
    tty->inb_size = inbs;
    tty->inb_write = 0;
    tty->inb_read = 0;
    tty->outb = (char*)kmalloc(outbs);
    if (tty->outb == 0) {
        kfree(tty->inb);
        return -1;
    }
    memset(tty->outb, 0, outbs);
    tty->outb_size = outbs;
    tty->outb_write = 0;
    tty->outb_read = 0;
    tty->cols = TTY_DEFAULT_COLS;
    tty->rows = TTY_DEFAULT_ROWS;
    tty->subscribers = kmalloc(sizeof(struct tty_subscriber));
    if (tty->subscribers == 0) {
        kfree(tty->inb);
        kfree(tty->outb);
        return -1;
    }
    tty->subscribers->next = 0;
    tty->subscribers->handler = 0;

    tty->line_discipline[LD_DEFAULT_TABLE] = line_discipline_create(
        LINE_DISCIPLINE_MODE_CANONICAL,
        LINE_DISCIPLINE_MODE_ECHO_ON,
        LD_DEFAULT_TABLE,
        1024,
        tty,
        flush_cb,
        echo_cb
    );
    tty->israw = 0;
    tty->echo = 1;
    tty->pty_read = tty_read_cb;
    if (tty->line_discipline[LD_DEFAULT_TABLE] == 0) {
        kfree(tty->inb);
        kfree(tty->outb);
        kfree(tty->subscribers);
        return -1;
    }
    
    //Wow this is utterly stupid
    //All of this to avoid a single #include

    switch (mode) {
        case TTY_MODE_SERIAL: {
            struct serial_ioctl_subscriptor sync_data = {
                .parent = tty,
                .handler = tty_read_cb
            };
            device_ioctl(tty->indev, SERIAL_SUBSCRIBE_READ, (void*)&sync_data);
            break;
        }
        case TTY_MODE_PTY: {
            break;
        }
        default:
            return -1;
    }

    tty->valid = 1;
    kprintf("tty_init(index: %d, in:%s, out:%s, %d, %d, %d)\n", index, indev, outdev, mode, inbs, outbs);
    return index;
}

void _tty_write(struct tty* tty, char* buffer, int size) {
    if (tty == 0 || !tty->valid) return;
    if (tty->line_discipline[tty->termios.c_line] == 0) return;

    for (int i = 0; i < size; i++) {
        char tbuffer[4];
        int translation = line_discipline_translate(tty->line_discipline[tty->termios.c_line], buffer[i], tbuffer);
        for (int j = 0; j < translation; j++) {
            tty_write_outb(tty, tbuffer[j]);
        }
    }
}

int is_valid_tty(struct tty* tty) {
    if (tty == 0) return 0;
    if (tty->valid == 0) return 0;
    return 1;
}

int _tty_get_size(struct tty* tty) {
    if (tty == 0 || !tty->valid) return 0;
    return tty->inb_write - tty->inb_read;
}

int _tty_has_input(struct tty* tty) {
    if (tty == 0 || !tty->valid) return 0;
    return tty->inb_write != tty->inb_read;
}

void _tty_modes(struct tty* tty, int mode, int value) {
    if (tty == 0 || !tty->valid) return;
    if (tty->line_discipline[tty->termios.c_line] == 0) return;

    switch (mode) {
        case _TTY_MODE_RAW: {
            tty->israw = value;
            if (value) line_discipline_set_mode(tty->line_discipline[tty->termios.c_line], LINE_DISCIPLINE_MODE_RAW);
            else line_discipline_set_mode(tty->line_discipline[tty->termios.c_line], LINE_DISCIPLINE_MODE_CANONICAL);
            break;
        }
        case _TTY_MODE_ECHO:
            tty->echo = value;
            break;
        default:
            return;
    }
}

void _tty_read(struct tty* tty, char* buffer, int size) {
    if (tty == 0 || !tty->valid) return;
    if (tty->line_discipline[tty->termios.c_line] == 0) return;

    for (int i = 0; i < size; i++) {
        tty_read_inb(tty, &buffer[i]);
        if (buffer[i] == 0) return;
    }
}