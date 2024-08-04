#ifndef _DCON_DRIVER_H
#define _DCON_DRIVER_H

#define DEVICE_DCON 0x90

void init_dcon_dd();
void hook_dcon(void * writer);
#endif