# --------------------------------------------------------------------------
#
#	FILE NAME:		Makefile
#   AUTHOR:			Microsemi
#	DESCRIPTION:	Top level make file.
#
# --------------------------------------------------------------------------
export SHELL =/bin/bash
SDKDIRS := vproc_sdk
export ROOTDIR =$(PWD)/$(SDKDIRS)
include config.mk

.PHONY: all $(SDKDIRS) cleanall clean install_sub install_sdk host alexa start_alexa avs_config

MSCC_MOD_NAMES =$(MSCC_HBI_MOD) $(MSCC_SND_COD_MOD) $(MSCC_SND_MAC_MOD)
MSCC_DTB_NAMES =$(MSCC_SPI_DTB) $(MSCC_SPIMULTI_DTB) $(MSCC_DAC_OVERLAY_DTB)

# make all command without arguments will run this
# 1- Download kernel headers and install them if not already done
# 2  compile the vproc_sdk
# 3- install the SDK and apps into the host
# 4- get alexa sample apps and install it
# 5- configure the ALSA sound card for the host
all: pi_kheaders vproc_sdk install_sdk alexa_install soundcfg message

# make command without arguments will run this
# 1- Download kernel headers and install them if not already done
# 2 compile the vproc_sdk
# 3- install the SDK and apps into the host
# 4- configure the ALSA sound card for the host
host: pi_kheaders vproc_sdk install_sdk soundcfg message

vproc_sdk: $(SDKDIRS)

ksrc_update: ksrc_msg_st pi_ksrc ksrc_msg_end


$(SDKDIRS):
	$(MAKE) -C $@ vproc_sdk

# get alexa, install it and configure the ALSA sound card accordingly
alexa: alexa_install soundcfg

avs_config: alexa_install

start_alexa: alexa_exec

# undo everything done by the make all
cleanall: disable_autostart cleanmod_sub cleanov_sub cleansnd_sub cleanprof_sub cleanrc_sub cleanboot_sub
	rm -rf $(INSTALL_MSCC_LOCAL_LIB_PATH)
	rm -rf $(AMAZON_AVS_LOCAL_DIR)
	$(MAKE) -C $(ROOTDIR) clean

#clean: same as cleanall but without the amazon_avs removal
#however don't remove the amazon_avs folder
clean: cleanmod_sub cleanov_sub cleansnd_sub cleanprof_sub cleanrc_sub cleanboot_sub
	$(MAKE) -C $(ROOTDIR) clean


#install the executable and modify appropriate files
install_sdk: install_sub modcfg_edit modcfg_cp bootcfg_edit startupcfg
	@echo "Compilation completed successfully..."

release: release_sub

swig:
	$(MAKE) -C $(ROOTDIR)/../apps/python/wrapper

help:
	@echo "----------------------------------------------------------------------------------------"
	@echo "| Microsemi AVS Kit software version 2.5.0                                             |"
	@echo "----------------------------------------------------------------------------------------"
	@echo "| SHELL="$(SHELL)
	@echo "| ROOTDIR="$(ROOTDIR)
	@echo "| SDKDIRS="$(SDKDIRS)
	@echo "| MODDIRS="$(INSTALL_MSCC_MOD_DIR)
	@echo "| OVERLDIRS="$(INSTALL_MSCC_OVERLAYS_DIR)
	@echo "| APPSDIRS="$(INSTALL_MSCC_APPS_PATH)
	@echo "| USERDIR="$(HOST_USER_HOME_DIR)
	@echo "| USERPROFILE="$(HOST_USER_PROF_START_CFG_FILE_PATH)
	@echo "| USER:GROUP="$(platformUser):$(platformGroup)
	@echo "| make start_alexa      : Start the Amazon Alexa sample application"
	@echo "| make avs_config       : Register or re-register with an Amazon Alexa account"
	@echo "| make update_sensory   : Renew the Sensory license"
	@echo "| make enable_autostart : Enable the Autoboot (the Amazon Alexa sample application"
	@echo "|                         automatically runs at start-up)"
	@echo "| make disable_autostart: Disable the Autoboot"
	@echo "| make all              : Make everything (Microsemi driver and Amazon Alexa sample"
	@echo "|                         application)"
	@echo "| make cleanall         : Uninstall everything (Microsemi driver and Amazon Alexa sample"
	@echo "|                         application)"
	@echo "| make host             : Make the Microsemi driver"
	@echo "| make clean            : Uninstall the Microsemi driver"
	@echo "----------------------------------------------------------------------------------------"

