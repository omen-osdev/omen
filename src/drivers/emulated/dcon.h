// OMEN Debug Console driver
// This piece of code registers a virtual device that
// prints to the terminal. It supports framebuffers, serials, etc

#ifndef _DCON_DRIVER_H
#define _DCON_DRIVER_H

#define DEVICE_DCON 0x90
#define DCON_DD_NAME "dcon"

#define DCON_IOCTL_GET_DEV 0

//Startup of the device driver, creates a dummy console over the bootloader's debug device
char * init_dcon_dd();

//Create a new DCON device with a custom writer
char * hook_dcon(void * writer);
#endif
