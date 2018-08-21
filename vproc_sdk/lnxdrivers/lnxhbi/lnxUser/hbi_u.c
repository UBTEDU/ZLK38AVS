/*
* hbi_u.c -  hbi user space driver
*
*
* Copyright 2016 Microsemi Inc.
*/

#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "typedefs.h"
#include "chip.h"
#include "hbi.h"
#include "hbi_prv.h"
#include "hbi_k.h"
#include "vproc_u_dbg.h"


struct _hbiu_driv_priv{
    hbi_init_cfg_t  cfg;
    int             drvInitialised;
    int             initCnt;
};

static struct _hbiu_driv_priv gHbiDrvPriv={
    .cfg={
        .lock=(ssl_lock_handle_t)NULL,
        },
    .drvInitialised=0,
    .initCnt=0
};
static int gDrvInitialised = FALSE;

char dev_name[256]=HBI_DEV_NAME;
hbi_dev_info_t hbi_devices_info[VPROC_MAX_NUM_DEVS];
/* Variable for current debug level set in the system */
VPROC_DBG_LVL vproc_dbg_lvl = DEBUG_LEVEL;

hbi_status_t HBI_init(hbi_init_cfg_t *pCfg)
{
   VPROC_U_DBG_PRINT(VPROC_DBG_LVL_FUNC, "%s Entry..\n",__func__);

   /* initialize user space driver*/
   if(pCfg != NULL)
   {
      gHbiDrvPriv.cfg.lock=pCfg->lock;
   }

   if(!gHbiDrvPriv.drvInitialised)
   {
        gHbiDrvPriv.drvInitialised = TRUE;
   }
   gHbiDrvPriv.initCnt++;
   VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO,"HBI initialized %d, opened count %d\n", gHbiDrvPriv.drvInitialised, gHbiDrvPriv.initCnt);

   return HBI_STATUS_SUCCESS;
}
/* Now the related dev file is opened here rightly so.
 * the dev filename is in the follow format
 * hbi.busnum.csaddr
 * Where the busnum is iether the I2c or SPI bus number
 *       the csaddr is either the SPI chipSelect or I2C address
 * As result each device are handled independantly, the handle for each device
 * is different allowing multiple instance of the device to be accessed fairly
 * (controlled by semaphoere locking of the bus and driver) simultaenously  
 */
hbi_status_t HBI_open(hbi_handle_t *pHandle, hbi_dev_cfg_t *pDevCfg)
{
   int ret;
   int fd=-1;
   char name[256];

#ifdef INIT_TERM_AUTO    
    if (HBI_init(NULL) != HBI_STATUS_SUCCESS)
    {
        return HBI_STATUS_NOT_INIT;
    }
#endif
   
   if(!gHbiDrvPriv.drvInitialised)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI Driver is not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   sprintf(name,"/dev/%s%d",dev_name,pDevCfg->deviceId);
   
   VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO, "Opening file %s\n",name);

   fd=open(name,(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH),O_RDWR);

   if (fd <0)
   {
      printf("Err 0x%x in HBI_OPEN \n",errno);
      return HBI_STATUS_RESOURCE_ERR;
   }

   *pHandle = fd;
   VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO, "Returned HBI handle 0x%x\n",*pHandle);

   return HBI_STATUS_SUCCESS;
}

hbi_status_t HBI_close(hbi_handle_t Handle)
{
   int ret;
   int fd = (int) Handle;
   
   if(!gHbiDrvPriv.drvInitialised)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI Driver is not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   ret = close(fd);
   if (ret <0)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Err 0x%x in HBI_CLOSE \n",errno);
      return HBI_STATUS_RESOURCE_ERR;
   }

#ifdef INIT_TERM_AUTO
    if (HBI_term() != HBI_STATUS_SUCCESS)
    {
        VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "driver term error\n");
        return HBI_STATUS_RESOURCE_ERR;
    }
#endif
   return HBI_STATUS_SUCCESS;
}

hbi_status_t HBI_term()
{
   int ret;

   if(!gHbiDrvPriv.drvInitialised)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI Driver is not initialized\n");
      /* driver is not initialised so do nothing */
      return HBI_STATUS_SUCCESS;
   }

   gHbiDrvPriv.initCnt--;
   
   if(gHbiDrvPriv.initCnt==0)
   {
     gHbiDrvPriv.drvInitialised=FALSE;
   }
   
   return HBI_STATUS_SUCCESS;
}

hbi_status_t HBI_dev_info(hbi_handle_t handle)
{
   int i = 0;
   int ret;
   int fd = (int) handle;
   
   ret=ioctl(fd,HBI_UPDATE,&hbi_devices_info);
   if(ret <0)
   {
      return HBI_STATUS_RESOURCE_ERR;
   }
   for (i=0; i<VPROC_MAX_NUM_DEVS; i++) {
       VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO,"hbi_devices_info[%d].chip = %d\n", i, hbi_devices_info[i].chip);
       VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO,"hbi_devices_info[%d].dev_addr = %d\n", i, hbi_devices_info[i].dev_addr);
       VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO,"hbi_devices_info[%d].isboot = %d\n", i, hbi_devices_info[i].isboot);
       VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO,"hbi_devices_info[%d].bus_num = %d\n", i, hbi_devices_info[i].bus_num);
       VPROC_U_DBG_PRINT(VPROC_DBG_LVL_INFO,"hbi_devices_info[%d].dev_lock = %d\n", i, hbi_devices_info[i].dev_lock);
   }

   return HBI_STATUS_SUCCESS;
}

hbi_status_t HBI_read(hbi_handle_t handle,reg_addr_t reg,user_buffer_t * pOutput,size_t length)
{
   hbi_lnx_drv_rw_arg_t  rwArg;
   int ret;
   int fd = (int) handle;
   
   if(!gHbiDrvPriv.drvInitialised)
   {
      return HBI_STATUS_NOT_INIT;
   }
   memset(&rwArg,0,sizeof(rwArg));

   rwArg.handle = handle;
   rwArg.pData = pOutput;
   rwArg.len = length;
   rwArg.reg = reg;

   ret = ioctl(fd,HBI_READ,&rwArg);

   if(ret <0)
   {
      return HBI_STATUS_RESOURCE_ERR;
   }

   return rwArg.status;
}

hbi_status_t HBI_write(hbi_handle_t handle,reg_addr_t reg,user_buffer_t * pInput,size_t length)
{
   hbi_lnx_drv_rw_arg_t  rwArg;
   int ret;
   int fd=(int)handle;
   
   if(!gHbiDrvPriv.drvInitialised)
   {
      return HBI_STATUS_NOT_INIT;
   }
   memset(&rwArg,0,sizeof(rwArg));

   rwArg.handle = handle;
   rwArg.pData = pInput;
   rwArg.len = length;
   rwArg.reg = reg;

   ret = ioctl(fd,HBI_WRITE,&rwArg);

   if(ret <0)
   {
      return HBI_STATUS_RESOURCE_ERR;
   }

   return rwArg.status;
}

hbi_status_t HBI_set_command(hbi_handle_t handle,hbi_cmd_t cmd,void *pCmdArgs)
{
   int            ret;
   hbi_status_t   status;
   int fd=(int)handle;

   if(!gHbiDrvPriv.drvInitialised)
   {
      VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "Driver not initialized\n");
      return HBI_STATUS_NOT_INIT;
   }

   
   switch(cmd)
   {
      case HBI_CMD_LOAD_FWR_FROM_HOST:
      {
         hbi_lnx_send_data_arg_t dataArg;

         memset(&dataArg,0,sizeof(dataArg));

         dataArg.data.pData = ((hbi_data_t *)pCmdArgs)->pData;
         dataArg.data.size  = ((hbi_data_t *)pCmdArgs)->size;
         dataArg.handle = handle;

         ret=ioctl(fd,HBI_LOAD_FW,&dataArg);
         if(ret < 0)
         {
            VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "call to LOAD_FW failed\n");
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
         {
            status = dataArg.status;
         }
         break;
      }
      case HBI_CMD_LOAD_CFGREC_FROM_HOST:
      {
         hbi_lnx_send_data_arg_t dataArg;

         memset(&dataArg,0,sizeof(dataArg));

         dataArg.data.pData = ((hbi_data_t *)pCmdArgs)->pData;
         dataArg.data.size  = ((hbi_data_t *)pCmdArgs)->size;
         dataArg.handle = handle;

         ret=ioctl(fd,HBI_LOAD_CFG,&dataArg);
         if(ret < 0)
         {
            VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "call to LOAD_CFG failed\n");
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
         {
            status = dataArg.status;
         }
         break;
      }
      case HBI_CMD_LOAD_FWR_COMPLETE:
      {
         hbi_lnx_ldfw_done_arg_t args;

         args.handle = handle;

         ret = ioctl(fd,HBI_LOAD_FW_COMPLETE,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      
      case HBI_CMD_START_FWR:
      {
         hbi_lnx_start_fw_arg_t args;

         args.handle = handle;

         ret = ioctl(fd,HBI_START_FW,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;

         break;
      }
      case HBI_CMD_LOAD_FWRCFG_FROM_FLASH:
      {
         hbi_lnx_flash_load_fwrcfg_arg_t args;

         memset(&args,0,sizeof(args));

         args.handle = handle;
         args.image_num = *((int32_t *)pCmdArgs);

         ret = ioctl(fd,HBI_FLASH_LOAD_FWR_CFGREC,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;

         break;
      }
      case HBI_CMD_ERASE_WHOLE_FLASH:
      case HBI_CMD_ERASE_FWRCFG_FROM_FLASH:
      {
         hbi_lnx_flash_erase_fwcfg_arg_t args;

         memset(&args,0,sizeof(args));
         args.handle = handle;

         if(cmd == HBI_CMD_ERASE_FWRCFG_FROM_FLASH && (pCmdArgs != NULL))
         {
            args.image_num = *((int32_t *)pCmdArgs);
            cmd = HBI_FLASH_ERASE_FWRCFGREC;
         }
         else
            cmd = HBI_FLASH_ERASE_WHOLE;

         ret = ioctl(fd,cmd,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      case HBI_CMD_SAVE_FWRCFG_TO_FLASH:
      {
         hbi_lnx_flash_save_fwrcfg_arg_t args;
         memset(&args,0,sizeof(args));
         args.handle = handle;
         ret = ioctl(fd,HBI_FLASH_SAVE_FWR_CFGREC,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      case HBI_CMD_SAVE_CFG_TO_FLASH:
      {
         hbi_lnx_flash_save_fwrcfg_arg_t args;
         memset(&args,0,sizeof(args));
         args.handle = handle;
         args.image_num = *((int32_t *)pCmdArgs);
         ret = ioctl(fd,HBI_FLASH_SAVE_CFGREC,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
         break;
      }
      default:
         status = HBI_STATUS_INVALID_ARG;
   }

   return status;
}

hbi_status_t HBI_reset(hbi_handle_t handle, hbi_rst_mode_t mode)
{
   if(!gHbiDrvPriv.drvInitialised)
   {
      return HBI_STATUS_NOT_INIT;
   }

   return HBI_STATUS_INTERNAL_ERR;
}

hbi_status_t HBI_sleep(hbi_handle_t handle)
{
   hbi_status_t   status;
   int fd=(int)handle;
   hbi_lnx_drv_arb_arg_t args;
   int            ret;
   
   if(!gHbiDrvPriv.drvInitialised)
   {
        VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "HBI_sleep driver not init failed\n");
	    return HBI_STATUS_NOT_INIT;
   }
         

         args.handle = handle;
         ret = ioctl(fd,HBI_SLEEP,&args);
         if(ret <0)
         {
            status = HBI_STATUS_RESOURCE_ERR;
         }
         else
            status = args.status;
			
    VPROC_U_DBG_PRINT(VPROC_DBG_LVL_ERR, "ioctl %d, HBI_sleep = %d\n", ret, status);
    return status;
}

hbi_status_t HBI_wake(hbi_handle_t handle)
{
   hbi_status_t   status;
   int fd=(int)handle;
   hbi_lnx_drv_arb_arg_t args;
   int            ret;
   
   if(!gHbiDrvPriv.drvInitialised)
   {
        return HBI_STATUS_NOT_INIT;
   }
         

        args.handle = handle;

        ret = ioctl(fd,HBI_WAKE,&args);
        if(ret <0)
        {
            status = HBI_STATUS_RESOURCE_ERR;
        }
        else
            status = args.status;   
   
   return status;
}


hbi_status_t HBI_get_header(hbi_data_t * pImg,hbi_img_hdr_t * pHdr)
{
   return internal_hbi_get_hdr(pImg,pHdr);
}

