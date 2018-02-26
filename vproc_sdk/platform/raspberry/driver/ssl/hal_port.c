/*
* hal_port.c - spi or I2C driver implmentation for HBI
*
* Hardware Abstraction Layer for Voice processor devices
* Every successful call would return 0 or
* a linux error code as defined in linux errno.h
*
* Copyright 2016 Microsemi Inc.
*
* This program is free software you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option)any later version.
*/

#undef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/moduleparam.h>
#if HBI==I2C
#include <linux/i2c.h>
#else
#include <linux/spi/spi.h>
#endif
#include <linux/list.h>
#include "typedefs.h"
#include "ssl.h"
#include "hal.h"
#include "vproc_dbg.h"
#include "fwr_image_headers.h" 

#undef HAL_DEBUG 

/*Driver probe function - Linux platform only*/
int vproc_probe(
#if HBI==I2C
            struct i2c_client *, const struct i2c_device_id *);
#else
            struct spi_device *);
#endif

int vproc_remove(
#if HBI==I2C
            struct i2c_client *);
#else
            struct spi_device *);
#endif

/* Initialized instance of devices
 * PLATFORM INDEPENDANT - 
 * There should be one entry per device as per the number of devices defined by 
 * VPROC_MAX_NUM_DEVS
 * The firmware and config record *.h files must be located in a folder named images under /platform/your_own_platform
 * EXample: for the raspberry platform  /platform/raspberry/images
 *          the example devices info structure is initialized for two slave devices
 */
static ssl_dev_info_t sdk_board_devices_info[] =
{
    {
        .chip = 38063,  /*Microsemi chip number without the ZL*/
        .bus_num = 0,   /*SPI or I2C bus number*/
        .dev_addr = 0,  /*SPI chip select or I2C address*/
        .isboot = FALSE, /*set this TRUE if a device firmware has to be loaded at boot*/
        .pFirmware = NULL, /*a pointer to either the filename if in *.bin format or data array  if in c code format*/
        .pConfig = NULL, /*a pointer to either the filename if in *.bin format or data array  if in c code format*/
        .dev_lock = 0,  /* lock to serialise device access */
        .imageType = 1, /*0: for static *.h, 1: for *.bin */
    },
    {
        .chip = 38090,  /*Microsemi chip number without the ZL*/
        .bus_num = 0,   /*SPI or I2C bus number*/
        .dev_addr = 1,  /*SPI chip select or I2C address*/
        .isboot = FALSE, /*set this TRUE if a device firmware has to be loaded at boot*/
        .pFirmware = NULL, /*a pointer to either the filename if in *.bin format or data array  if in c code format*/
        .pConfig = NULL, /*a pointer to either the filename if in *.bin format or data array  if in c code format*/
        .dev_lock = 0,  /* lock to serialise device access */
        .imageType = 1, /*0: for static *.h, 1: for *.bin */
    }
};

int dev_id = 0;

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#if HBI==I2C
static struct i2c_client devClient[VPROC_MAX_NUM_DEVS];
#else
static struct spi_device devClient[VPROC_MAX_NUM_DEVS];
#endif

/*IMPORTANT NOTE: 
 * This is for Linux platform only. Not needed for non-Linux platform 
 * This is only required if the Linux platform support device tree driver registration
 * Change string maching of .compatible= accordingly as per your *.dts or *dtsi compatible definition file
 */
static struct of_device_id vproc_of_match[] = { /*DO NOT CHANGE vproc_of_match*/
	{ .compatible = "microsemi,zl38xx0",}, /*Change this "microsemi,zl38xx0" accordingly*/
	{ .compatible = "microsemi,zl38xx1",}, /*Change this "microsemi,zl38xx1" accordingly*/
	{},
};
MODULE_DEVICE_TABLE(of, vproc_of_match);
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

/* Structure definining spi/I2C driver  - This is for Linux platform only
 * Not needed for non-Linux platform
 */
#if HBI==I2C
static struct i2c_device_id vproc_device_id[] = {
           {"zl380xx", 0 },
    {}
};

struct i2c_driver vproc_driver = {
#else
static struct spi_device_id vproc_device_id[VPROC_MAX_NUM_DEVS];
struct spi_driver vproc_driver = {
#endif
    .id_table = vproc_device_id,
    .probe = vproc_probe,
    .remove = vproc_remove,
    .driver = {
        .name = VPROC_DEV_NAME, /* name field should be equal
                                       to module name and without spaces */
        .owner = THIS_MODULE,
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
        .of_match_table = vproc_of_match,
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/
    }
};


/*vproc_probe() - This function is Linux specific it is automatically 
 *               called by the Linux kernel driver registration process whenever
 *               a spi_register_driver() or I2C_add_driver is issued
 *               It is not need for non-linux operating system
 *  Args: 
 *           pointer to where to the spi or I2C device driver data
 * Return:
 *          0 if success, a negative number if failure
 */
int vproc_probe(
#if HBI==I2C 
      struct i2c_client *pClient, const struct i2c_device_id *pDeviceId) /*I2C*/
#else
      struct spi_device *pClient)   /*SPI*/
#endif
{

    /* Bind device to driver */
    pClient->dev.driver = &(vproc_driver.driver);
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"slave HBI device %d found at"  
#if HBI==I2C    
    "adapter::addr = %s::0x%02x\n", dev_id, pClient->adapter->name, pClient->addr);   /*I2C*/
#else
    "bus::cs = %d::0x%02x\n", dev_id, pClient->master->bus_num, pClient->chip_select); /*SPI*/
    /*setup the desired SPI parameters*/
    /* NOTE: the ZL380xx supports SPI speed up to 25MHz, however, this fast speed requires
     * very stringent signal integrity and shorter layout traces between the host and the ZL
     */
	pClient->mode = SPI_MODE_0;
	pClient->max_speed_hz = 20000000; /*ZL380xx supports SPI speed up to 25000000 - Set as desired*/
	if (spi_setup(pClient) < 0)
	{
		return -EAGAIN;
    }
#endif

    devClient[dev_id++] = *pClient;
#endif

    return 0;
}

int vproc_remove(
#if HBI==I2C  
     struct i2c_client *pClient)
#else
     struct spi_device *pClient)
#endif
{
    return 0;
}

/*hal_init() - This function is the first function call by the driver upon install
 *             any specific board/platform setup must be done by this funtion
 *  Args: 
 *         none 
 * Return:
 *          0 if success, a negative number if failure
 */
int hal_init(void)
{
    int status = sdk_register_board_devices_info(sdk_board_devices_info);
    if (status < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"sdk_register_board_devices_info() failed error = %d\n", status);
    }
#if HBI==I2C     
    status = i2c_add_driver(&vproc_driver);
#else
    status = spi_register_driver(&vproc_driver);
#endif
    if (status < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"device registration failed error = %d\n", status);
    }
    dev_id = 0;
    return status;
}

int hal_term()
{
#if HBI==I2C 
    i2c_del_driver(&vproc_driver);
#else
    spi_unregister_driver(&vproc_driver);
#endif
    dev_id = 0;
    return 0;
}


/*hal_open() - use this function to open one or multiple instances of the driver 
 *  Args: 
 *         pHandle : device driver handle, basically a refence to how to access the device
 *         pDevCfg : pointer to the device bus number and the address on the bus to open
 * Return:
 *         0 if success, a negative number if failure
 */
int hal_open(void **ppHandle,void *pDevCfg)
{
    tw_device_id_t deviceId;

    ssl_dev_cfg_t *pDev = (ssl_dev_cfg_t *)pDevCfg;
#if HBI==I2C    
    struct i2c_adapter      *pAdap=NULL;
    struct i2c_client  *pClient = NULL;   
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    struct i2c_board_info   bi; 
#endif 
#else     
    struct spi_master *pAdap = NULL;
    struct spi_device *pClient = NULL;
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    struct spi_board_info   bi; 
#endif 
#endif

    if ((pDev->deviceId < 0) || (pDev->deviceId >= VPROC_MAX_NUM_DEVS))
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Id, device ID must be a value from 0 to %d \n", VPROC_MAX_NUM_DEVS-1);
        return -EINVAL;
    }

    if(pDev == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Device Cfg Reference\n");
        return -EINVAL;
    }
    deviceId = pDev->deviceId;
    pDev->bus_num = sdk_board_devices_info[deviceId].bus_num;
    pDev->dev_addr =sdk_board_devices_info[deviceId].dev_addr;

    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO, "Hal Opening device %d with addr : 0x%x bus num %d\n", deviceId, pDev->dev_addr, pDev->bus_num);

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    pClient = &devClient[deviceId];
#endif
    /* get the controller driver through bus num */
#if HBI==I2C  
   
    pAdap = i2c_get_adapter(pDev->bus_num);
    if(pAdap==NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Bus Num %d \n",pDev->bus_num);
        return SSL_STATUS_INVALID_ARG;
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"i2c adap name %s \n",pAdap->name);

    pClient->addr = pDev->dev_addr;
    if(pDev->pDevName != NULL)
    {
        strcpy(pClient->name,pDev->pDevName);
    }   
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
    memset(&bi,0,sizeof(bi));
    if(pDev->pDevName != NULL)
    {
        strcpy(bi.type,pDev->pDevName);
    }
    bi.addr = pDev->dev_addr;
    pClient = i2c_new_device(pAdap,(struct i2c_board_info const *)&bi);
#endif     
    if(pClient == NULL)
    {
     /* call failed either because address is invalid, valid but occupied
        or there is resource err. Just return code to try again later */
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"i2c device instantiation failed\n");
        return -EAGAIN;
    }

   *((struct i2c_client **)ppHandle) =  pClient;
   VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Opened i2c device %d adapter:%s addr:0x%02X...\n", dev_id, pClient->adapter->name, pClient->addr);
#else
    pAdap = spi_busnum_to_master(pDev->bus_num);

    if(pAdap==NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Bus Num %d \n",pDev->bus_num);
        return SSL_STATUS_INVALID_ARG;
    }
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"spi adap name %s \n",pAdap->dev.init_name);
    if(pDev->pDevName != NULL)
    {
        strcpy(pClient->modalias,pDev->pDevName);
    }
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING

    memset(&bi,0,sizeof(bi));
    if(pDev->pDevName != NULL)
    {
        strcpy(bi.modalias,pDev->pDevName);
    }
    bi.bus_num = pDev->bus_num;
    bi.chip_select = pDev->dev_addr;
    bi.max_speed_hz = 1000000;
    bi.mode = SPI_MODE_0;
    pClient = spi_new_device(pAdap,&bi);
#endif
    if(pClient == NULL)
    {
        /* call failed either because address is invalid, valid but occupied
           or there is resource err. Just return code to try again later */
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"device instantiation failed\n");
        return -EAGAIN;
    }
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,
                     "Updating handle 0x%x back to user\n",(uint32_t)pClient);

   *((struct spi_device **)ppHandle) =  pClient;
    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Opened slave device %d found at bus::cs = %d::%d\n", dev_id, pClient->master->bus_num, pClient->chip_select);
#endif
    return 0;
}

/*hal_close() - Since multiple instance of the driver can be opened simultaneously
 *              use this function to close a particular instance of the driver
 *  Args: 
 *         pHandle : device driver handle, basically a refence to how to access the device
 * Return:
 *         0 if success, a negative number if failure
 */

int hal_close(void *pHandle)
{
    if(pHandle == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL client handle passed\n");
        return -EINVAL;
    }

    VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,
                     "Unregistering client 0x%x\n",(u32) pHandle);

    /*Do not unregister the SPI if using device tree registration*/
#ifndef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#if HBI==I2C 
    i2c_unregister_device((struct i2c_client *) pHandle);
#else
    spi_unregister_device((struct spi_device *) pHandle);
#endif
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/
    return 0;
}


/*hal_port_rw():  this function is used for both read and write accesses
 *  write: send data from the master to the slave device
 *  read:  master receives data from the slave device
 *  Args: 
 *         pHandle : device driver handle, basically a refence to how to access the device
 *         pPortAccess: the access type, data to send and the buffer to receive the data
 * Return:
 *         0 if success, a negative number if failure
 */
int hal_port_rw(void *pHandle,void *pPortAccess)
{
    int                 ret=0;
    ssl_port_access_t   *pPort = (ssl_port_access_t *)pPortAccess;
#if HBI==I2C 
    struct i2c_client *pClient = pHandle;
    struct i2c_msg msg[2];
#else
    struct spi_device  *pClient = (struct spi_device *)pHandle;
    struct spi_transfer msg[2];
#endif
    int                 msgnum=0;
    ssl_op_t          op_type;
#ifdef HAL_DEBUG
    int                  i;
#endif

    if(pHandle == NULL || pPort == NULL)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"Invalid Parameters\n");
        return -EINVAL;
    }

    op_type = pPort->op_type;
    memset(msg,0,sizeof(msg));
    if(op_type & SSL_OP_PORT_WR)
    {
        if(pPort->pSrc == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL src buffer passed\n");
            return -EINVAL;
        }
#if HBI==I2C 
        msg[msgnum].addr = pClient->addr;
        msg[msgnum].flags = 0;
        msg[msgnum].buf = pPort->pSrc;
#else
        msg[msgnum].tx_buf = pPort->pSrc;
#endif
        msg[msgnum].len = pPort->nwrite;

#ifdef HAL_DEBUG
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"writing %d bytes..\n",pPort->nwrite);

        for(i=0;i<pPort->nwrite;i++)
        {
            printk("0x%x\t",((uint8_t *)(pPort->pSrc))[i]);
        }
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"\n");
#endif
        msgnum++;
    }

    if(op_type & SSL_OP_PORT_RD)
    {
        if(pPort->pDst == NULL)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"NULL destination buffer passed\n");
            return -EINVAL;
        }
#if HBI==I2C 
        msg[msgnum].addr = pClient->addr;
        msg[msgnum].flags = I2C_M_RD;
        msg[msgnum].buf = pPort->pDst;
#else        
        msg[msgnum].rx_buf = pPort->pDst;
#endif
        msg[msgnum].len = pPort->nread;
        msgnum++;
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"read %d bytes..\n",pPort->nread);
    }
#if HBI==I2C
    ret = i2c_transfer(pClient->adapter,msg,msgnum);
#else
    ret  = spi_sync_transfer(pClient,msg,msgnum);
#endif
    if(ret < 0)
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_ERR,"failed with Error %d\n",ret);
    }
    #ifdef HAL_DEBUG
    if(!ret && (msgnum >=1))
    {
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"Received...\n");
        for(i=0;i<pPort->nread;i++)
        {
            VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"0x%x\t",((uint8_t *)(pPort->pDst))[i]);
        }
        VPROC_DBG_PRINT(VPROC_DBG_LVL_INFO,"\n");
    }
    #endif
    return ret;
}




