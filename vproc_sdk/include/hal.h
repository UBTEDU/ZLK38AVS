#ifndef __HAL_H_
#define __HAL_H_

#ifndef VPROC_DEV_NAME 
#define VPROC_DEV_NAME "zl380xx"
#endif

int hal_init(void);
int hal_term(void);
int hal_open(void **ppHandle,void *pDevCfg);
int hal_close(void *pHandle);
int hal_port_rw(void *pHandle,void *pPortAccess);

#endif

