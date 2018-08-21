/*
 * Driver for the ZL380xx codec
 *
 * Copyright (c) 2016, Microsemi Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <sound/soc.h>

#if VPROC_CODEC_MIXER_ENABLE
#include <linux/moduleparam.h>
#include "typedefs.h"


#include "ssl.h"
#include "chip.h"
#include "hbi.h"

hbi_device_id_t deviceId=0;

module_param(deviceId, uint, S_IRUGO);
MODULE_PARM_DESC(dev_addr, "device Id (a value from 0 to VPROC_MAX_NUM_DEVS-1");

static int zl380xx_control_write(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol);

static int zl380xx_control_read(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol);

struct _zl380xx_priv{
    hbi_handle_t handle;
};

struct _zl380xx_priv zl380xx_priv;

#ifdef VPROC_CODEC_MIXER_ENABLE_DMUTE
/*zl380tw_mute_r() - function to Mute ROUT*/
static int zl380tw_mute_r(struct snd_soc_codec *codec, int on)
{

    user_buffer_t buf[2];
    hbi_status_t status;
    reg_addr_t reg = ZL380xx_AEC_CTRL0_REG;    
	u16 val;

    status = HBI_read(zl380xx_priv.handle, reg, buf,2);
    if (status != HBI_STATUS_SUCCESS){
        return -EIO;
    }
    val = (buf[0] << 8)| buf[1];

	if (((val >> 7) & 1) == on){
		return 0;
	}
	val &= ~(1 << 7);
	val |= on << 7;

    buf[0] = val >> 8;
    buf[1] = val & 0xFF;

    status = HBI_write(zl380xx_priv.handle, reg,buf,2);
    if (status != HBI_STATUS_SUCCESS){
        return -EIO;
    }
	return 0;
}

/*zl380tw_mute_s() - function to Mute SOUT*/
static int zl380tw_mute_s(struct snd_soc_codec *codec, int on)
{
	u16 val;
    user_buffer_t buf[2];
    hbi_status_t status;
    reg_addr_t reg = ZL380xx_AEC_CTRL0_REG;

    status = HBI_read(zl380xx_priv.handle, reg, buf,2);
    if (status != HBI_STATUS_SUCCESS){
        return -EIO;
    }
    val = (buf[0] << 8)| buf[1];
	
	if (((val >> 8) & 1) == on){
		return 0;
	}
	val &= ~(1 << 8);
	val |= on << 8;

    buf[0] = val >> 8;
    buf[1] = val & 0xFF;

    status = HBI_write(zl380xx_priv.handle, reg,buf,2);
    if (status != HBI_STATUS_SUCCESS){
        return -EIO;
    }
	return 0;
}

/*ALSA auto-handling of muting audio path when no audio is detected*/
static int zl380xx_mute(struct snd_soc_dai *codec_dai, int mute)
{

	struct snd_soc_codec *codec = codec_dai->codec;
	/*zl380tw_mute_s(codec, mute);*/ /*uncomment if you want to mute both send and receive paths*/
	return zl380tw_mute_r(codec, mute);

}

static const struct snd_soc_dai_ops zl380xx_dai_ops = {
	.digital_mute   = zl380xx_mute,
};
#endif
#endif

static struct snd_soc_dai_driver zl380xx_dai = {
    .name = "zl380xx-dai",
    .playback = {
        .channels_min = 1,
        .channels_max = 2,
        .rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_44100),
        .formats = SNDRV_PCM_FMTBIT_S16_LE
    },
    .capture = {
        .channels_min = 1,
        .channels_max = 2,
        .rates = (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_48000),
        .formats = SNDRV_PCM_FMTBIT_S16_LE
    },
#ifdef VPROC_CODEC_MIXER_ENABLE_DMUTE   
	.ops = &zl380xx_dai_ops,
#endif
};

#if VPROC_CODEC_MIXER_ENABLE
static const struct snd_kcontrol_new zl380xx_snd_controls[] = {
    SOC_SINGLE_EXT("DAC1 GAIN INA", ZL380xx_CP_DAC1_GAIN_REG, 0, 0x6, 0,
                    zl380xx_control_read, zl380xx_control_write),
    SOC_SINGLE_EXT("DAC2 GAIN INA", ZL380xx_CP_DAC2_GAIN_REG, 0, 0x6, 0,
                    zl380xx_control_read, zl380xx_control_write),
    SOC_SINGLE_EXT("DAC1 GAIN INB", ZL380xx_CP_DAC1_GAIN_REG, 8, 0x6, 0,
            zl380xx_control_read, zl380xx_control_write),
    SOC_SINGLE_EXT("DAC2 GAIN INB", ZL380xx_CP_DAC2_GAIN_REG, 8, 0x6, 0,
            zl380xx_control_read, zl380xx_control_write),
    SOC_SINGLE_EXT("MUTE SPEAKER ROUT", ZL380xx_AEC_CTRL0_REG, 7, 1, 0,
                    zl380xx_control_read, zl380xx_control_write),
    SOC_SINGLE_EXT("MUTE MIC SOUT", ZL380xx_AEC_CTRL0_REG, 8, 1, 0,
                    zl380xx_control_read, zl380xx_control_write),
    SOC_SINGLE_EXT("AEC MIC GAIN", ZL380xx_DIG_MIC_GAIN_REG, 0, 0x7, 0,
                    zl380xx_control_read, zl380xx_control_write),
	SOC_SINGLE_EXT("AEC ROUT GAIN", ZL380xx_ROUT_GAIN_CTRL_REG, 0, 0x78, 0,
		            zl380xx_control_read, zl380xx_control_write),
	SOC_SINGLE_EXT("AEC ROUT GAIN EXT", ZL380xx_ROUT_GAIN_CTRL_REG, 7, 0x7, 0,
		        	zl380xx_control_read, zl380xx_control_write),
	SOC_SINGLE_EXT("AEC SOUT GAIN", ZL380xx_SOUT_GAIN_CTRL_REG, 8, 0xf, 0,
		        	zl380xx_control_read, zl380xx_control_write),

};

static int zl380xx_control_read(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{
    struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;

    unsigned int reg = mc->reg;
    unsigned int shift = mc->shift;
    unsigned int mask = mc->max;
    unsigned int invert = mc->invert;
    unsigned char buf[2];
    hbi_status_t status =HBI_STATUS_SUCCESS;
    unsigned int val=0;

    status = HBI_read(zl380xx_priv.handle, reg, buf,2);
    printk("val received 0x%x 0x%x\n",buf[0],buf[1]);
    val=buf[0];
    val=(val << 8)|buf[1];
    
    ucontrol->value.integer.value[0] = ((val >> shift) & mask);

    if (invert)
        ucontrol->value.integer.value[0] = mask - ucontrol->value.integer.value[0];

    return 0;
}


static int zl380xx_control_write(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_value *ucontrol)
{

    struct soc_mixer_control *mc = (struct soc_mixer_control *)kcontrol->private_value;

    reg_addr_t reg = mc->reg;
    unsigned int shift = mc->shift;
    unsigned int mask = mc->max;
    unsigned int invert = mc->invert;
    unsigned int val = (ucontrol->value.integer.value[0] & mask);
    unsigned int valt = 0;
    user_buffer_t buf[2];
    hbi_status_t status;

    if (invert)
        val = mask - val;

    status = HBI_read(zl380xx_priv.handle, reg, buf,2);
    if (status != HBI_STATUS_SUCCESS){
        return -EIO;
    }

    valt=buf[0];
    valt=(valt << 8)|buf[1];

    if (((valt >> shift) & mask) == val) {
        return 0;
    }


    valt &= ~(mask << shift);
    valt |= val << shift;

    buf[0]=valt>>8;
    buf[1]=valt&0xFF;

    status = HBI_write(zl380xx_priv.handle, reg,buf,2);

    if (status != HBI_STATUS_SUCCESS){
        return -EIO;
    }
    return 0;
}


int zl380xx_add_controls(struct snd_soc_codec *codec)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
    return snd_soc_add_controls(codec, zl380xx_snd_controls,
                                ARRAY_SIZE(zl380xx_snd_controls));
#else
    return snd_soc_add_codec_controls(codec, zl380xx_snd_controls,
                                        ARRAY_SIZE(zl380xx_snd_controls));
#endif
}


static int zl380xx_codec_probe(struct snd_soc_codec *codec)
{

    hbi_status_t status;
    hbi_dev_cfg_t cfg;
    
    if(zl380xx_add_controls(codec) < 0)
    {
        return -1;
    }

    status=HBI_init(NULL);
    if(status != HBI_STATUS_SUCCESS)
    {
        printk(KERN_ERR"Error in HBI_init()\n");
        return -1;
    }

    cfg.pDevName=NULL;
    cfg.deviceId = deviceId;

    status=HBI_open(&(zl380xx_priv.handle),&cfg);
    if(status != HBI_STATUS_SUCCESS)
    {
        printk(KERN_ERR"Error in HBI_open()\n");
        HBI_term();
        return -1;
    }

    return 0;
}

static int zl380xx_codec_remove(struct snd_soc_codec *codec)
{
    hbi_status_t status;
    
    status=HBI_close(zl380xx_priv.handle);
    status=HBI_term();
    
    return 0;
}
#endif
static struct snd_soc_codec_driver soc_codec_dev_zl380xx={
#if VPROC_CODEC_MIXER_ENABLE       
    .probe=zl380xx_codec_probe,
    .remove=zl380xx_codec_remove
#endif
};


static int zl380xx_probe(struct platform_device *pdev)
{
#if VPROC_CODEC_MIXER_ENABLE
    memset(&zl380xx_priv,0,sizeof(struct _zl380xx_priv));
#endif
    return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_zl380xx,&zl380xx_dai, 1);
}

static int zl380xx_remove(struct platform_device *pdev)
{
    snd_soc_unregister_codec(&pdev->dev);
    return 0;
}

static const struct of_device_id zl380xx_of_match[] = {
    { .compatible = "ms,zl38040", },
    {}
};
MODULE_DEVICE_TABLE(of, zl380xx_of_match);

static struct platform_driver zl380xx_codec_driver = {
    .probe      = zl380xx_probe,
    .remove     = zl380xx_remove,
    .driver = {
    .name   = "zl380-codec",
    .owner  = THIS_MODULE,
    .of_match_table = zl380xx_of_match,
    },
};

module_platform_driver(zl380xx_codec_driver);

MODULE_DESCRIPTION("ASoC zl380xx codec driver");
MODULE_AUTHOR("Jean Bony");
MODULE_LICENSE("GPL v2");
