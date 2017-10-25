# --------------------------------------------------------------
#
#	FILE NAME:		config.mk
#   AUTHOR:	        Microsemi		
#	DESCRIPTION:    This config file contains global variables used 
#					by either the vproc SDK code the apps or by the top level Makefile	
#                   
#
# --------------------------------------------------------------
export INSTALL_MSCC_MOD_DIR =/lib/modules/`uname -r`/kernel/drivers
export INSTALL_MSCC_OVERLAYS_DIR =/boot/overlays
export INSTALL_MSCC_APPS_PATH =/usr/local/bin/
export HBI_MOD_LOCAL_PATH =$(ROOTDIR)/lnxdrivers/lnxhbi/lnxkernel
export MSCC_LOCAL_LIB_PATH =$(ROOTDIR)/libs

export platformUser :=`id -un`
export platformGroup :=`id -gn`

AMAZON_AVS_ONLINE_REPOSITORY =https://github.com/alexa/alexa-avs-sample-app.git
SENSORY_ALEXA_ONLINE_REPOSITORY =https://github.com/Sensory/alexa-rpi.git
AMAZON_AVS_LOCAL_DIR ?=$(ROOTDIR)/../amazon_avs
AMAZON_AVS_FILE_PATH=$(AMAZON_AVS_LOCAL_DIR)/automated_install.sh

HOST_PI_IMAGE_VER :=`cat /etc/os-release`
HOST_KHEADERS_DIR =/lib/modules/`uname -r`/build
HOST_MODULES_FILE_PATH =/etc/modules
HOST_BOOTCFG_FILE_PATH =/boot/config.txt
HOST_MODULES_FILE_DIR =/lib/modules
HOST_USER_APPS_START_CFG_FILE_PATH =/etc/rc.local
HOST_USER_PROF_START_CFG_FILE_PATH :=/home/$(platformUser)/.profile
HOST_USER_HOME_DIR :=/home/$(platformUser)
MSCC_LOCAL_APPS_PATH =$(ROOTDIR)/../apps
HOST_SAMBA_CFG_PATH =/etc/samba/smb.conf
HOST_SAMBA_SHARE_PATH = $(HOST_USER_HOME_DIR)/shares

MSCC_SND_COD_MOD=snd-soc-zl380xx
MSCC_SND_MAC_MOD=snd-soc-microsemi-dac
MSCC_HBI_MOD=hbi
MSCC_SND_MIXER_MOD=snd-soc-zl380xx-mixer
MSCC_APPS_FWLD=mscc_fw_loader

MSCC_DAC_OVERLAY_DTB=microsemi-dac-overlay
MSCC_SPIMULTI_DTB=microsemi-spi-multi-tw-overlay
MSCC_SPI_DTB=microsemi-spi-overlay

#TW configuration option for MICs 180 or 360 degree
MSCC_TW_CONFIG_SELECT=180

#-------DO NOT MAKE CHANGE BELOW this line ---------------------------

ifeq ($(MSCC_TW_CONFIG_SELECT),180)
	MSCC_TW_CONFIG_SELECT=$(MSCC_APPS_FWLD) 0
else
	MSCC_TW_CONFIG_SELECT=$(MSCC_APPS_FWLD) 1
endif

# if the raspberrypi kernel headers needed to compile the sdk do not exist fetch them
.PHONY: pi_kheaders alexa_install
pi_kheaders :
	@if cat /etc/os-release | grep -q 'stretch'; then \
	   echo "kernel headers do not exist, fetching and installing kernel headers..."; \
	   sudo apt-get update; \
	   sudo apt-get install raspberrypi-kernel-headers; \
	   sudo apt-get install -y xterm; \
		else \
	   echo "kernel headers do not exist, fetching and installing kernel headers..."; \
	   sudo wget https://raw.githubusercontent.com/notro/rpi-source/master/rpi-source -O /usr/bin/rpi-source && sudo chmod +x /usr/bin/rpi-source && /usr/bin/rpi-source -q --tag-update; \
	   sudo apt-get install bc; \
	   rpi-source; \
	   sudo apt-get update; \
	fi
	

MsFwLoader=""


startupcfg:
	@if [ ! -f $(HOST_USER_HOME_DIR)/.profile.backup ]; then \
	   sudo cp $(HOST_USER_PROF_START_CFG_FILE_PATH) $(HOST_USER_HOME_DIR)/.profile.backup ; \
	   echo "sudo chown -R $(platformUser):$(platformGroup) /dev/$(MSCC_HBI_MOD)" | sudo tee -a $(HOST_USER_PROF_START_CFG_FILE_PATH);	\
	fi
	@if [ ! -f $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_USER_APPS_START_CFG_FILE_PATH) $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ; \
	fi
	@ (	\
	if grep -e "$(MSCC_APPS_FWLD) 0" $(HOST_USER_APPS_START_CFG_FILE_PATH); then  \
	   echo "Found 0, updating ... "$(MSCC_TW_CONFIG_SELECT);	\
	   sudo sed -i "s/$(MSCC_APPS_FWLD) 0/$(MSCC_TW_CONFIG_SELECT)/g" $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	elif grep -e "$(MSCC_APPS_FWLD) 1" $(HOST_USER_APPS_START_CFG_FILE_PATH); then  \
	   echo "Found 1, updating ... "$(MSCC_TW_CONFIG_SELECT);	\
	   sudo sed -i "s/$(MSCC_APPS_FWLD) 1/$(MSCC_TW_CONFIG_SELECT)/g" $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	else \
	   echo "not Found , updating ... "$(MSCC_TW_CONFIG_SELECT);	\
	   sudo sed -i "s/exit 0/$(MSCC_TW_CONFIG_SELECT)/g" $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	   echo "exit 0" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH);	\
	fi \
	)
		
install_sub:
	sudo install -m 0755 $(HBI_MOD_LOCAL_PATH)/*.ko $(INSTALL_MSCC_MOD_DIR)
	sudo install -m 0755 $(MSCC_LOCAL_LIB_PATH)/*.ko $(INSTALL_MSCC_MOD_DIR)
	sudo install -m 0755 $(MSCC_LOCAL_LIB_PATH)/*.dtbo $(INSTALL_MSCC_OVERLAYS_DIR)	
	sudo install -m 0755 $(MSCC_LOCAL_APPS_PATH)/$(MSCC_APPS_FWLD) $(INSTALL_MSCC_APPS_PATH)	

.PHONY: modcfg_edit bootcfg_edit startupcfg cleanmod_sub cleanov_sub modcfg_cp 
cleanmod_sub:
	sudo rm $(INSTALL_MSCC_APPS_PATH)/$(MSCC_APPS_FWLD)
	@ (	\
	for line in $(MSCC_MOD_NAMES); 	do	\
	   echo "$$line" ;\
	   if grep -Fxq "$$line"  $(HOST_MODULES_FILE_PATH) ; then  \
	      echo "Found, then removing $$line...";	\
		  sudo sed -i "/$$line/ d" $(HOST_MODULES_FILE_PATH); \
		  TEMPVAR=$$line.ko; \
		  echo "$$TEMPVAR ..." ; \
		  sudo rm $(INSTALL_MSCC_MOD_DIR)/$$TEMPVAR; \
	   fi \
	done	\
	)
	
.PHONY: cleansnd_sub cleanprof_sub cleanrc_sub cleanboot_sub cleanmodcfg_sub bootcfg_sub message
cleansnd_sub:
	@if [ -f $(HOST_USER_HOME_DIR)/.asoundrc.backup ]; then \
	   sudo cp $(HOST_USER_HOME_DIR)/.asoundrc.backup $(HOST_USER_HOME_DIR)/.asoundrc	; \
	   sudo rm $(HOST_USER_HOME_DIR)/.asoundrc.backup  ; \
	   sudo rm /etc/asound.conf  ; \
	fi	

cleanprof_sub:
	@if [ -f $(HOST_USER_HOME_DIR)/.profile.backup ]; then \
	   sudo cp $(HOST_USER_HOME_DIR)/.profile.backup $(HOST_USER_HOME_DIR)/.profile	; \
	   sudo rm $(HOST_USER_HOME_DIR)/.profile.backup ; \
	fi	

cleanrc_sub:
	@if [ -f $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_USER_APPS_START_CFG_FILE_PATH).backup $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	   sudo rm $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ; \
	fi

cleanboot_sub: 
	@if [ -f $(HOST_BOOTCFG_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_BOOTCFG_FILE_PATH).backup $(HOST_BOOTCFG_FILE_PATH) ; \
	   sudo rm $(HOST_BOOTCFG_FILE_PATH).backup ; \
	fi	


cleanmodcfg_sub:
	@if [ -f $(HOST_MODULES_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_MODULES_FILE_PATH).backup $(HOST_MODULES_FILE_PATH) ; \
	   sudo rm $(HOST_MODULES_FILE_PATH).backup; \
	fi	
	

cleanov_sub: 
	@ (	\
	for line in $(MSCC_DTB_NAMES); 	do	\
	   echo "$$line" ;\
	   if grep -Fxq "dtoverlay=$$line"  $(HOST_BOOTCFG_FILE_PATH) ; then  \
	      echo "Found, then removing $$line...";	\
		  sudo sed -i "/dtoverlay=$$line/ d" $(HOST_BOOTCFG_FILE_PATH); \
		  TEMPVAR=$$line.dtbo; \
		  echo "$$TEMPVAR ..."; \
		  sudo rm $(INSTALL_MSCC_OVERLAYS_DIR)/$$TEMPVAR ; \
	   fi \
	done	\
	)

modcfg_cp: 
	sudo cp $(HOST_MODULES_FILE_PATH) $(HOST_MODULES_FILE_DIR)
	sudo depmod -a

modcfg_edit:
	@if [ ! -f $(HOST_MODULES_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_MODULES_FILE_PATH) $(HOST_MODULES_FILE_PATH).backup ; \
	fi	
	@ (	\
	for line in $(MSCC_MOD_NAMES); 	do	\
	   echo "$$line" ;\
	   if grep -Fxq "$$line"  $(HOST_MODULES_FILE_PATH) ; then  \
	      echo "Found, then nothing to do...";	\
	   else  \
	      echo "adding $$line into that config file...";	\
	      sudo bash -c "echo "$$line" >> $(HOST_MODULES_FILE_PATH)"; \
	   fi \
	done	\
	)

bootcfg_sub:
	@if [ ! -f $(HOST_BOOTCFG_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_BOOTCFG_FILE_PATH) $(HOST_BOOTCFG_FILE_PATH).backup ; \
	fi	
	sudo sed -i "s/dtparam=audio=on/#dtparam=audio=on/g" $(HOST_BOOTCFG_FILE_PATH)	
	sudo sed -i "s/#dtparam=i2s=on/dtparam=i2s=on/g" $(HOST_BOOTCFG_FILE_PATH)
	@echo "dtoverlay=i2s-mmap" | sudo tee -a $(HOST_BOOTCFG_FILE_PATH);	
	sudo sed -i "s/#dtparam=i2c_arm=on/dtparam=i2c_arm=on/g" $(HOST_BOOTCFG_FILE_PATH)
	sudo sed -i "s/#dtparam=spi=on/dtparam=spi=on/g" $(HOST_BOOTCFG_FILE_PATH)


bootcfg_edit: bootcfg_sub
	@ (	\
	for line in $(MSCC_DTB_NAMES); 	do	\
	   echo "$$line" ;\
	   if grep -Fxq "$$line"  $(HOST_BOOTCFG_FILE_PATH) ; then  \
	      echo "Found ... nothing to do";	\
	   else  \
	      echo "adding $$line into the config file...";	\
	      sudo bash -c "echo "dtoverlay=$$line" >> $(HOST_BOOTCFG_FILE_PATH)"; \
	   fi \
	done	\
	)

amazon_sub:
	@echo "--****************************************************************************--" 
	@echo "--****************************************************************************--" 
	@echo " Downloading and installing Amazon Alexa Make sure you have the Amazon developer"
	@echo " account/product info needed to install the alexa sample app"	
	@echo "--****************************************************************************--" 
	@echo "--****************************************************************************--" 
	sudo chmod 777 $(AMAZON_AVS_LOCAL_DIR)/*.sh
	sudo sed -i "/sudo apt-get upgrade -y/ d" $(AMAZON_AVS_FILE_PATH)
	
AvsDevid=""	
AvsClientid=""
AvsSecreid=""


alexa_install: amazon_sub
	@read -p "enter the device ID obtained from Amazon:" AvsDevid; \
	echo "You entered AVS Device ID: $$AvsDevid "; \
	read -p "enter the Client ID obtained from Amazon:" AvsClientid; \
	echo "You entered AVS Client ID: $$AvsClientid "; \
	read -p "enter Client Secret obtained from Amazon :" AvsSecreid; \
	echo "You entered AVS Client secret : $$AvsClientid "; \
	sudo sed -i "s/ProductID=.*/ProductID=$$AvsDevid/" $(AMAZON_AVS_FILE_PATH); \
	sudo sed -i "s/ClientID=.*/ClientID=$$AvsClientid/" $(AMAZON_AVS_FILE_PATH); \
	sudo sed -i "s/ClientSecret=.*/ClientSecret=$$AvsSecreid/" $(AMAZON_AVS_FILE_PATH); \
	cd $(AMAZON_AVS_LOCAL_DIR) ; \
	./automated_install.sh

	
# make sure the default sound card is always the microsemi card	
.PHONY: samba_sh soundcfg

soundcfg:
	@echo "--****************************************************************************--" 
	@echo "--****************************************************************************--" 
	@echo " configuring the host ALSA related sound configuration"
	@echo "--****************************************************************************--" 
	@echo "--****************************************************************************--" 
	@if [ -f $(HOST_USER_HOME_DIR)/.asoundrc.backup ]; then \
	   exit 0 ; \
	fi	
	
	@if [ -f /home/$(platformUser)/.asoundrc ]; then sudo cp $(HOST_USER_HOME_DIR)/.asoundrc $(HOST_USER_HOME_DIR)/.asoundrc.backup; rm /home/$(platformUser)/.asoundrc ; \
	fi
	@printf "\npcm.dmixed {\n    ipc_key 1025\n    type dmix\n    slave {\n        pcm \"hw:sndmicrosemidac,0\"\n        channels 2\n        rate 16000\n    }\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\npcm.dsnooped {\n    ipc_key 1027\n    type dsnoop\n    slave {\n        pcm \"hw:sndmicrosemidac,0\"\n        channels 1\n        rate 16000\n    }\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\npcm.asymed {\n    type asym\n    playback.pcm \"dmixed\"\n    capture.pcm \"dsnooped\"\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\npcm.!default {\n    type plug\n    slave.pcm \"asymed\"\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\nctl.!default {\n    type hw\n    card sndmicrosemidac\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@sudo cp $(HOST_USER_HOME_DIR)/.asoundrc /etc/asound.conf	
	
samba:
	sudo apt-get update
	sudo apt-get install samba samba-common-bin
	sudo cp /etc/samba/smb.conf /etc/samba/smb.conf.old
	sudo smbpasswd -a $(platformUser)
	@if [ ! -d $(HOST_SAMBA_SHARE_PATH) ] ; then \
	   mkdir $(HOST_SAMBA_SHARE_PATH); \
	   sudo chown -R root:users $(HOST_SAMBA_SHARE_PATH); \
	   sudo chmod -R ug=rwx,o=rx $(HOST_SAMBA_SHARE_PATH); \
	fi
	@sudo sed -i "s/server role = standalone server/security = user \n   server role = standalone/" $(HOST_SAMBA_CFG_PATH); 
	@sudo sed -i "s/read only = yes/read only = no/g" $(HOST_SAMBA_CFG_PATH); 
	@echo "[shares]" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   comment = Public Storage" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   path = $(HOST_SAMBA_SHARE_PATH)" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   valid users = users" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   force group = users" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   create mask = 0660" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   directory mask = 0771" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	@echo "   read only = no" | sudo tee -a $(HOST_SAMBA_CFG_PATH);
	sudo /etc/init.d/samba restart

message:
	@echo "--****************************************************************************--" 
	@echo "--****************************************************************************--" 
	@echo " System setup completed successfully...."
	@echo " NOTE: For the changes made to the host to take effect Please do a:  sudo reboot "
	@echo "--****************************************************************************--" 
	@echo "--****************************************************************************--" 

.PHONY: alexa_exec


alexa_exec:
	@sudo cp /etc/asound.conf $(HOST_USER_HOME_DIR)/.asoundrc 
	xterm -hold -e 'bash -c "cd $(AMAZON_AVS_LOCAL_DIR)/samples/companionService && npm start; exec bash"' &
	xterm -hold -e 'bash -c "cd $(AMAZON_AVS_LOCAL_DIR)/samples/javaclient && mvn exec:exec; exec bash"' &
	xterm -hold -e 'bash -c "cd $(AMAZON_AVS_LOCAL_DIR)/samples/wakeWordAgent/src && ./wakeWordAgent -e sensory; exec bash"' &
