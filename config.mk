# --------------------------------------------------------------
#
#	FILE NAME:		config.mk
#   AUTHOR:	        Microsemi
#	DESCRIPTION:    This config file contains global variables used
#					by either the vproc SDK code the apps or by the top level Makefile
#
#
# --------------------------------------------------------------
export INSTALL_MSCC_MOD_DIR =/usr/local/avs
export INSTALL_MSCC_OVERLAYS_DIR =/boot/overlays
export INSTALL_MSCC_APPS_PATH =/usr/local/bin/
export HBI_MOD_LOCAL_PATH =$(ROOTDIR)/lnxdrivers/lnxhbi/lnxkernel
export MSCC_LOCAL_LIB_PATH =$(ROOTDIR)/libs

export platformUser :=`id -un`
export platformGroup :=`id -gn`

AMAZON_AVS_ONLINE_REPOSITORY =https://github.com/alexa/avs-device-sdk
SENSORY_ALEXA_ONLINE_REPOSITORY =https://github.com/Sensory/alexa-rpi
AMAZON_AVS_LOCAL_DIR ?=$(ROOTDIR)/../amazon_avs_cpp
AMAZON_AVS_JSON_CONFIG =$(AMAZON_AVS_LOCAL_DIR)/sdk-build/Integration/AlexaClientSDKConfig.json
AMAZON_AVS_SDK_REL=v1.3.tar.gz

HOST_PI_IMAGE_VER :=`cat /etc/os-release`
HOST_KHEADERS_DIR =/lib/modules/`uname -r`/build
HOST_MODULES_FILE_PATH =/etc/modules
HOST_BOOTCFG_FILE_PATH =/boot/config.txt
HOST_MODULES_FILE_DIR =/lib/modules
HOST_USER_APPS_START_CFG_FILE_PATH =/etc/rc.local
HOST_USER_HOME_DIR :=/home/$(platformUser)
MSCC_LOCAL_APPS_PATH =$(ROOTDIR)/../apps
HOST_SAMBA_CFG_PATH =/etc/samba/smb.conf
HOST_SAMBA_SHARE_PATH =$(HOST_USER_HOME_DIR)/shares

MSCC_SND_COD_MOD =snd-soc-zl380xx
MSCC_SND_MAC_MOD =snd-soc-microsemi-dac
MSCC_HBI_MOD =hbi
MSCC_SND_MIXER_MOD =snd-soc-zl380xx-mixer
MSCC_APPS_FWLD =mscc_fw_loader

MSCC_DAC_OVERLAY_DTB =microsemi-dac-overlay
MSCC_SPIMULTI_DTB =microsemi-spi-multi-tw-overlay
MSCC_SPI_DTB =microsemi-spi-overlay

#TW configuration option for MICs (0: 1 mic, 1: 2 mics, 2: 3 mics)
MSCC_TW_CONFIG_SELECT_IDX =2

#-------DO NOT MAKE CHANGE BELOW this line ---------------------------
MSCC_TW_CONFIG_SELECT =$(MSCC_APPS_FWLD) $(MSCC_TW_CONFIG_SELECT_IDX)

# if the raspberrypi kernel headers needed to compile the sdk do not exist fetch them
.PHONY: pi_kheaders avs_git alexa_install

pi_kheaders :
	@if cat /etc/os-release | grep -q 'stretch'; then \
	    echo "kernel headers do not exist, fetching and installing kernel headers..."; \
	    sudo apt-get update; \
	    sudo apt-get install raspberrypi-kernel-headers; \
	    sudo apt-get install -y xterm; \
	else \
	    echo; \
	    echo; \
	    echo "--****************************************************************************--"; \
	    echo "--                                 ABORTING                                   --"; \
	    echo "--                   Only Raspbian 9 \"Stretch\" is supported                   --"; \
	    echo "--****************************************************************************--"; \
	    echo; \
	    false; \
	fi

startupcfg:
	@if [ ! -f $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ]; then \
	    sudo cp $(HOST_USER_APPS_START_CFG_FILE_PATH) $(HOST_USER_APPS_START_CFG_FILE_PATH).backup; \
	    sudo sed -i "s/^exit 0//g" $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "if ! lsmod | grep -q hbi ; then" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "    sudo insmod $(INSTALL_MSCC_MOD_DIR)/$(MSCC_HBI_MOD).ko" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "    sudo insmod $(INSTALL_MSCC_MOD_DIR)/$(MSCC_SND_COD_MOD).ko" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "    sudo insmod $(INSTALL_MSCC_MOD_DIR)/$(MSCC_SND_MAC_MOD).ko" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "fi" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "sudo chown -R $(platformUser):$(platformGroup) /dev/$(MSCC_HBI_MOD)*" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    echo "exit 0" | sudo tee -a $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	fi
	@ ( \
	if grep -e "$(MSCC_APPS_FWLD) [0-2]" $(HOST_USER_APPS_START_CFG_FILE_PATH); then \
	    echo "Found, updating ... "$(MSCC_TW_CONFIG_SELECT); \
	    sudo sed -i "s/$(MSCC_APPS_FWLD) [0-2]/$(MSCC_TW_CONFIG_SELECT)/g" $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	else \
	    echo "Not found , updating ... "$(MSCC_TW_CONFIG_SELECT); \
	    sudo sed -i "s/^exit 0/$(MSCC_TW_CONFIG_SELECT)\n\nexit 0/g" $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	fi \
	)

install_sub:
	@if [ -d $(INSTALL_MSCC_MOD_DIR) ]; then \
	    sudo rm -rf $(INSTALL_MSCC_MOD_DIR); \
	fi
	sudo mkdir $(INSTALL_MSCC_MOD_DIR)
	sudo install -m 0755 $(HBI_MOD_LOCAL_PATH)/*.ko $(INSTALL_MSCC_MOD_DIR)
	sudo install -m 0755 $(MSCC_LOCAL_LIB_PATH)/*.ko $(INSTALL_MSCC_MOD_DIR)
	sudo install -m 0755 $(MSCC_LOCAL_LIB_PATH)/*.dtbo $(INSTALL_MSCC_OVERLAYS_DIR)
	sudo install -m 0755 $(MSCC_LOCAL_APPS_PATH)/$(MSCC_APPS_FWLD) $(INSTALL_MSCC_APPS_PATH)

.PHONY: modcfg_edit bootcfg_edit startupcfg cleanmod_sub cleanov_sub modcfg_cp
cleanmod_sub:
	sudo rm $(INSTALL_MSCC_APPS_PATH)/$(MSCC_APPS_FWLD)
	sudo rm -rf $(INSTALL_MSCC_MOD_DIR)

.PHONY: disable_autostart cleansnd_sub cleanprof_sub cleanrc_sub cleanboot_sub cleanmodcfg_sub bootcfg_sub message enable_autostart set_serial
disable_autostart:
	@if [ -f $(ROOTDIR)/../\.start.sh ]; then \
	    rm $(ROOTDIR)/../\.start.sh; \
	fi
	@if [ -f $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart.backup ]; then \
	    cp $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart.backup $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart; \
	    rm $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart.backup; \
	fi

cleansnd_sub:
	@if [ -f $(HOST_USER_HOME_DIR)/.asoundrc.backup ]; then \
	    sudo cp $(HOST_USER_HOME_DIR)/.asoundrc.backup $(HOST_USER_HOME_DIR)/.asoundrc; \
	    sudo rm $(HOST_USER_HOME_DIR)/.asoundrc.backup; \
	    sudo rm /etc/asound.conf; \
	fi

cleanprof_sub:
	@if [ -f $(HOST_USER_HOME_DIR)/.profile.backup ]; then \
	    sudo cp $(HOST_USER_HOME_DIR)/.profile.backup $(HOST_USER_HOME_DIR)/.profile; \
	    sudo rm $(HOST_USER_HOME_DIR)/.profile.backup; \
	fi

cleanrc_sub:
	@if [ -f $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ]; then \
	    sudo cp $(HOST_USER_APPS_START_CFG_FILE_PATH).backup $(HOST_USER_APPS_START_CFG_FILE_PATH); \
	    sudo rm $(HOST_USER_APPS_START_CFG_FILE_PATH).backup ; \
	fi

cleanboot_sub:
	@if [ -f $(HOST_BOOTCFG_FILE_PATH).backup ]; then \
	    sudo cp $(HOST_BOOTCFG_FILE_PATH).backup $(HOST_BOOTCFG_FILE_PATH); \
	    sudo rm $(HOST_BOOTCFG_FILE_PATH).backup; \
	fi

cleanmodcfg_sub:
	@if [ -f $(HOST_MODULES_FILE_PATH).backup ]; then \
	    sudo cp $(HOST_MODULES_FILE_PATH).backup $(HOST_MODULES_FILE_PATH); \
	    sudo rm $(HOST_MODULES_FILE_PATH).backup; \
	fi

cleanov_sub:
	@ (	\
	for line in $(MSCC_DTB_NAMES); do \
	    echo "$$line"; \
	    if grep -Fxq "dtoverlay=$$line"  $(HOST_BOOTCFG_FILE_PATH); then \
	        echo "Found, then removing $$line..."; \
	        sudo sed -i "/dtoverlay=$$line/ d" $(HOST_BOOTCFG_FILE_PATH); \
	        TEMPVAR=$$line.dtbo; \
	        echo "$$TEMPVAR ..."; \
	        sudo rm $(INSTALL_MSCC_OVERLAYS_DIR)/$$TEMPVAR; \
	    fi \
	done \
	)

modcfg_cp:
	sudo cp $(HOST_MODULES_FILE_PATH) $(HOST_MODULES_FILE_DIR)
	sudo depmod -a

modcfg_edit:
	@if [ ! -f $(HOST_MODULES_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_MODULES_FILE_PATH) $(HOST_MODULES_FILE_PATH).backup; \
	fi
	@ ( \
	for line in $(MSCC_MOD_NAMES); do \
	    echo "$$line"; \
	    if grep -Fxq "$$line"  $(HOST_MODULES_FILE_PATH); then \
	        echo "Found, then nothing to do..."; \
	    else \
	        echo "adding $$line into that config file..."; \
	        sudo bash -c "echo "$$line" >> $(HOST_MODULES_FILE_PATH)"; \
	    fi \
	done \
	)

bootcfg_sub:
	@if [ ! -f $(HOST_BOOTCFG_FILE_PATH).backup ]; then \
	   sudo cp $(HOST_BOOTCFG_FILE_PATH) $(HOST_BOOTCFG_FILE_PATH).backup; \
	fi
	sudo sed -i "s/dtparam=audio=on/#dtparam=audio=on/g" $(HOST_BOOTCFG_FILE_PATH)
	sudo sed -i "s/#dtparam=i2s=on/dtparam=i2s=on/g" $(HOST_BOOTCFG_FILE_PATH)
	@echo "dtoverlay=i2s-mmap" | sudo tee -a $(HOST_BOOTCFG_FILE_PATH);
	sudo sed -i "s/#dtparam=i2c_arm=on/dtparam=i2c_arm=on/g" $(HOST_BOOTCFG_FILE_PATH)
	sudo sed -i "s/#dtparam=spi=on/dtparam=spi=on/g" $(HOST_BOOTCFG_FILE_PATH)

bootcfg_edit: bootcfg_sub
	@ ( \
	for line in $(MSCC_DTB_NAMES); do \
	    echo "$$line"; \
	    if grep -Fxq "$$line"  $(HOST_BOOTCFG_FILE_PATH) ; then \
	        echo "Found ... nothing to do"; \
	    else \
	        echo "adding $$line into the config file..."; \
	        sudo bash -c "echo "dtoverlay=$$line" >> $(HOST_BOOTCFG_FILE_PATH)"; \
	    fi \
	done \
	)

enable_autostart:
	@if [ ! -f $(ROOTDIR)/../\.start.sh ]; then \
	    echo "#!/usr/bin/env bash" > $(ROOTDIR)/../\.start.sh; \
	    echo "" >> $(ROOTDIR)/../\.start.sh; \
	    echo "sleep 2" >> $(ROOTDIR)/../\.start.sh; \
	    echo "cd $(ROOTDIR)/.." >> $(ROOTDIR)/../\.start.sh; \
	    echo "make alexa_xterm" >> $(ROOTDIR)/../\.start.sh; \
	    echo "sleep 8" >> $(ROOTDIR)/../\.start.sh; \
	    echo "aplay $(AMAZON_AVS_LOCAL_DIR)/application-necessities/sound-files/med_system_alerts_melodic_01_short._TTH_.wav" >> $(ROOTDIR)/../\.start.sh; \
	    echo "sleep 2" >> $(ROOTDIR)/../.start.sh; \
	fi
	@sudo chmod +x $(ROOTDIR)/../\.start.sh
	@if [ ! -f $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart.backup ]; then \
	    cp $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart.backup; \
	    echo "@lxterminal -e $(ROOTDIR)/../.start.sh" >> $(HOST_USER_HOME_DIR)/\.config/lxsession/LXDE-pi/autostart; \
	fi

set_serial:
	@serial=`cat /proc/cpuinfo | grep Serial | cut -d ' ' -f 2`; \
	echo "Updated AVS serial number to: $$serial" ; \
	jq '.authDelegate.deviceSerialNumber="'$$serial'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG)

alexa_install:
	@echo "--*********************************************************************************--"
	@echo "--*********************************************************************************--"
	@echo "-- Downloading and installing Amazon Alexa Make sure you have the Amazon developer --"
	@echo "-- account/product info needed to install the alexa sample app                     --"
	@echo "--                                                                                 --"
	@echo "-- Note: Run this Makefile from the Raspberry Pi Desktop environement as the last  --"
	@echo "-- step requires Chromium to run on the Raspberry Pi                               --"
	@echo "--*********************************************************************************--"
	@echo "--*********************************************************************************--"

# Install all the required packages
# Get the specified tag of the  Alexa C++ sample app and add code to turn on an LED when Alexa is detected
	@if [ ! -d  $(AMAZON_AVS_LOCAL_DIR) ]; then \
	    sudo apt-get update; \
	    sudo apt-get -y install git gcc cmake build-essential libsqlite3-dev libcurl4-openssl-dev libfaad-dev libsoup2.4-dev libgcrypt20-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-good libasound2-dev doxygen jq moreutils; \
	    sudo pip install commentjson; \
	    mkdir $(AMAZON_AVS_LOCAL_DIR) $(AMAZON_AVS_LOCAL_DIR)/sdk-source $(AMAZON_AVS_LOCAL_DIR)/third-party; \
	    cd $(AMAZON_AVS_LOCAL_DIR)/sdk-source; \
	    mkdir avs-device-sdk; \
	    wget https://github.com/alexa/avs-device-sdk/archive/$(AMAZON_AVS_SDK_REL); \
	    tar -xf $(AMAZON_AVS_SDK_REL) -C avs-device-sdk --strip-components 1; \
	    rm $(AMAZON_AVS_SDK_REL); \
	    cp -f $(MSCC_LOCAL_APPS_PATH)/reserved/CMakeLists.txt ./avs-device-sdk/SampleApp/src/; \
	    cp -f $(MSCC_LOCAL_APPS_PATH)/reserved/*.cpp ./avs-device-sdk/SampleApp/src/; \
	    cp -f $(MSCC_LOCAL_APPS_PATH)/reserved/*.h ./avs-device-sdk/SampleApp/include/SampleApp/; \
	fi

# Install Portaudio
	@if [ ! -d  $(AMAZON_AVS_LOCAL_DIR)/third-party/portaudio ]; then \
	    cd $(AMAZON_AVS_LOCAL_DIR)/third-party; \
	    wget -c http://www.portaudio.com/archives/pa_stable_v190600_20161030.tgz; \
	    tar zxf pa_stable_v190600_20161030.tgz; \
	    cd portaudio; \
	    ./configure --without-jack; \
	    make; \
	fi

# Install Sensory
	@if [ ! -d  $(AMAZON_AVS_LOCAL_DIR)/third-party/alexa-rpi ]; then \
	    cd $(AMAZON_AVS_LOCAL_DIR)/third-party; \
	    git clone $(SENSORY_ALEXA_ONLINE_REPOSITORY); \
	    rm -f alexa-rpi/models/*.*; \
	    cp ../../apps/mscc_gr.lib alexa-rpi/models/spot-alexa-rpi-20500.snsr; \
	    cp ../../apps/mscc_gr.lib alexa-rpi/models/spot-alexa-rpi-21000.snsr; \
	    cp ../../apps/mscc_gr.lib alexa-rpi/models/spot-alexa-rpi-31000.snsr; \
	    cd alexa-rpi/bin; \
	    echo; \
	    echo; \
	    echo "--****************************************************************************--"; \
	    echo "--             Read and accept Sensory's licence agreement (yes)              --"; \
	    echo "--****************************************************************************--"; \
	    echo; \
	    ./license.sh; \
	fi

# Build the Alexa sample app
	@if [ ! -d  $(AMAZON_AVS_LOCAL_DIR)/sdk-build ]; then \
	    mkdir $(AMAZON_AVS_LOCAL_DIR)/sdk-build; \
	    cd $(AMAZON_AVS_LOCAL_DIR)/sdk-build; \
	    cmake $(AMAZON_AVS_LOCAL_DIR)/sdk-source/avs-device-sdk -DSENSORY_KEY_WORD_DETECTOR=ON -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=$(AMAZON_AVS_LOCAL_DIR)/third-party/alexa-rpi/lib/libsnsr.a -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=$(AMAZON_AVS_LOCAL_DIR)/third-party/alexa-rpi/include -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON -DPORTAUDIO_LIB_PATH=$(AMAZON_AVS_LOCAL_DIR)/third-party/portaudio/lib/.libs/libportaudio.a -DPORTAUDIO_INCLUDE_DIR=$(AMAZON_AVS_LOCAL_DIR)/third-party/portaudio/include; \
	    make SampleApp -j2; \
	fi

# Create a folder to store the db files
	@if [ ! -d  $(AMAZON_AVS_LOCAL_DIR)/application-necessities ]; then \
	    mkdir $(AMAZON_AVS_LOCAL_DIR)/application-necessities; \
	    cd $(AMAZON_AVS_LOCAL_DIR)/application-necessities; \
	    wget https://images-na.ssl-images-amazon.com/images/G/01/mobile-apps/dex/alexa/alexa-voice-service/docs/audio/states/med_system_alerts_melodic_01_short._TTH_.wav; \
	fi

# Prompt the user for the Amazon tokens and update the Alexa JSON file
	@echo
	@echo
	@echo "--****************************************************************************--"
	@echo "--     Enter the device's tokens found on your Amazon developper account      --"
	@echo "--                https://developer.amazon.com/avs/home.html                  --"
	@echo "--****************************************************************************--"
	@echo

	@read -p "Product ID   : " productID; \
	read -p "Client ID    : " clientID; \
	read -p "Client secret: " clientSecret; \
	alertsDB=$(AMAZON_AVS_LOCAL_DIR)/application-necessities/alerts.db; \
	settingsDB=$(AMAZON_AVS_LOCAL_DIR)/application-necessities/settings.db; \
	certifDB=$(AMAZON_AVS_LOCAL_DIR)/application-necessities/certifiedSender.db; \
	cat $(AMAZON_AVS_JSON_CONFIG)|grep -v '\s*//'|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.authDelegate.clientSecret="'$$clientSecret'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.authDelegate.deviceSerialNumber="1"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.authDelegate.clientId="'$$clientID'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.authDelegate.productId="'$$productID'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.authDelegate.refreshToken="{SDK_CONFIG_REFRESH_TOKEN}"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.alertsCapabilityAgent.databaseFilePath="'$$alertsDB'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.settings.databaseFilePath="'$$settingsDB'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.settings.defaultAVSClientSettings.locale="en-US"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \
	jq '.certifiedSender.databaseFilePath="'$$certifDB'"' $(AMAZON_AVS_JSON_CONFIG)|sponge $(AMAZON_AVS_JSON_CONFIG); \

# Update the refresh token
	@echo
	@echo
	@echo "--****************************************************************************--"
	@echo "--     Open Chromium (on the Pi) and navigate to \"http://localhost:3000\"      --"
	@echo "--       Expected: \"refresh request failed with the response code 400\"        --"
	@echo "--****************************************************************************--"

	@cd $(AMAZON_AVS_LOCAL_DIR)/sdk-build; \
	python AuthServer/AuthServer.py

	@echo
	@echo
	@echo "--****************************************************************************--"
	@echo "--                 Alexa sample app autoboot (headless mode)                  --"
	@echo "--****************************************************************************--"
	@echo

	@ ( \
	    read -p "Do you want the Alexa sample app to autoboot [y/n]?" answer; \
	    if [ $$answer == "y" ]; then \
	        $(MAKE) enable_autostart; \
	    else \
	        $(MAKE) disable_autostart; \
	    fi \
	)

# Update the serial number
	@$(MAKE) set_serial

	@echo
	@echo
	@echo "--****************************************************************************--"
	@echo "--                   Alexa sample app installation complete                   --"
	@echo "--****************************************************************************--"
	@echo

# make sure the default sound card is always the microsemi card
.PHONY: samba_sh soundcfg update_sensory

soundcfg:
	@echo "--****************************************************************************--"
	@echo "--           Configuring the host ALSA related sound configuration            --"
	@echo "--****************************************************************************--"
	@if [ -f $(HOST_USER_HOME_DIR)/.asoundrc.backup ]; then \
	    exit 0; \
	fi

	@if [ -f /home/$(platformUser)/.asoundrc ]; then sudo cp $(HOST_USER_HOME_DIR)/.asoundrc $(HOST_USER_HOME_DIR)/.asoundrc.backup; rm /home/$(platformUser)/.asoundrc; \
	fi
	@printf "\npcm.dmixed {\n    ipc_key 1025\n    type dmix\n    slave {\n        pcm \"hw:sndmicrosemidac,0\"\n        channels 2\n        rate 48000\n    }\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\npcm.dsnooped {\n    ipc_key 1027\n    type dsnoop\n    slave {\n        pcm \"hw:sndmicrosemidac,0\"\n        channels 1\n        rate 48000\n    }\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\npcm.asymed {\n    type asym\n    playback.pcm \"dmixed\"\n    capture.pcm \"dsnooped\"\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\npcm.!default {\n    type plug\n    slave.pcm \"asymed\"\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@printf "\nctl.!default {\n    type hw\n    card sndmicrosemidac\n}\n" >> $(HOST_USER_HOME_DIR)/.asoundrc
	@sudo cp $(HOST_USER_HOME_DIR)/.asoundrc /etc/asound.conf

# Install and setup Samba
samba:
	sudo apt-get update
	sudo apt-get install samba samba-common-bin
	sudo cp /etc/samba/smb.conf /etc/samba/smb.conf.old
	sudo smbpasswd -a $(platformUser)
	@if [ ! -d $(HOST_SAMBA_SHARE_PATH) ]; then \
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

# Update the Sensory license
update_sensory:
	@cd $(AMAZON_AVS_LOCAL_DIR)/third-party/alexa-rpi; \
	git reset --hard; \
	git pull; \
	rm -f models/*.*; \
	cp ../../../apps/mscc_gr.lib models/spot-alexa-rpi-20500.snsr; \
	cp ../../../apps/mscc_gr.lib models/spot-alexa-rpi-21000.snsr; \
	cp ../../../apps/mscc_gr.lib models/spot-alexa-rpi-31000.snsr; \
	cd bin; \
	echo; \
	echo; \
	echo "--****************************************************************************--"; \
	echo "--             Read and accept Sensory's licence agreement (yes)              --"; \
	echo "--****************************************************************************--"; \
	echo; \
	./license.sh; \
	rm -rf $(AMAZON_AVS_LOCAL_DIR)/sdk-build
	$(MAKE) alexa_install

message:
	@echo "--*********************************************************************************--"
	@echo "--*********************************************************************************--"
	@echo "--                      System setup completed successfully...                     --"
	@echo "-- NOTE: For the changes made to the host to take effect Please do a:  sudo reboot --"
	@echo "--*********************************************************************************--"
	@echo "--*********************************************************************************--"

.PHONY: alexa_exec alexa_xterm

# Start the sample app in a separate terminal
alexa_xterm:
	@sudo cp /etc/asound.conf $(HOST_USER_HOME_DIR)/.asoundrc
	@xterm -hold -e 'bash -c "cd $(AMAZON_AVS_LOCAL_DIR)/sdk-build/SampleApp/src/; TZ=UTC ./SampleApp $(AMAZON_AVS_JSON_CONFIG) $(AMAZON_AVS_LOCAL_DIR)/third-party/alexa-rpi/models; bash"' &

# Start the sample app in the same terminal
alexa_exec:
	@sudo cp /etc/asound.conf $(HOST_USER_HOME_DIR)/.asoundrc
	@cd $(AMAZON_AVS_LOCAL_DIR)/sdk-build/SampleApp/src/; \
	TZ=UTC ./SampleApp $(AMAZON_AVS_JSON_CONFIG) $(AMAZON_AVS_LOCAL_DIR)/third-party/alexa-rpi/models
