#ifndef _CONFIG_DEVICES_H
#define _CONFIG_DEVICES_H

#define CHAR_MIN_MAJOR 0x80

const char* DEVICE_IDENTIFIERS[] = {
    "none", // 0
    "none", // 1
    "none", // 2
    "none", // 3
    "none", // 4
    "none", // 5
    "none", // 6
    "none", // 7
    "hd",   // 8
    "cd",   // 9
    "semb", // a
    "pm",   // b
    "sd",   // c
    "none", // d
    "tty", // e
    "none", // f
    "mouse", // 10
    "net", // 11
    "fb", // 12
    "none", // 13 
    [0x8d] = "serial", // 80
    [0x8e] = "fifo",
    [0x8f] = "kbd", // 81
    [0x90] = "dcon" // 82
};

#endif