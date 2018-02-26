export SHELL ?=/bin/bash
export ROOTDIR ?=$(PWD)
include Makefile.globals
include config.mk

TARGET=

.PHONY: all platform hbilnx ssl hbi apps tools vproc_sdk drivers fwr header header_empty cleanall

PROJECTS = platform/$(PLATFORM) drivers lnxdrivers 

all: platform ssl hbi hbilnx 

vproc_sdk: hbilnx

tools:
	@echo "Building Toolchain...."
	$(MAKE) -C $(PLATFORM_DIR)/tools 

fwr:
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/images/
	
fwr_clean:
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/images/ clean

platform:
	@echo "Building Platform...."
	mkdir -p $(INSTALL_LIB_PATH)
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/ 

platform_clean:
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/ clean
	
hbilnx:header platform hbi drivers codec 

drivers: 
	$(MAKE) -C $(ROOTDIR)/lnxdrivers/

hbilnx_clean:platform_clean ssl_clean hbi_clean
	$(MAKE) -C $(ROOTDIR)/lnxdrivers/ clean

apps:
	$(MAKE) -C $(ROOTDIR)/apps/C/ $(TARGET)
	
apps_clean:
	$(MAKE) -C $(ROOTDIR)/apps/C/ clean
	
apps_help:
	$(MAKE) -C $(ROOTDIR)/apps/C/ help

hbi: 
	mkdir -p $(INSTALL_LIB_PATH)
	$(MAKE) -C $(ROOTDIR)/drivers/hbi/

hbi_clean:
	$(MAKE) -C $(ROOTDIR)/drivers/hbi/ clean

swig:
	$(MAKE) -C $(ROOTDIR)/apps/Python/wrapper/

swig_clean:
	$(MAKE) -C $(ROOTDIR)/apps/Python/wrapper/ clean

ssl: 
	mkdir -p $(INSTALL_LIB_PATH)
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/driver/ssl
codec :
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/driver/sound
	
mixer :
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/driver/sound

codec_clean:
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/driver/sound clean

ssl_clean:
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM)/driver/ssl clean

doxygen:
	$(MAKE) -f doxygen.mk

help:
	@echo "ROOTDIR="$(ROOTDIR)
	@echo "PLATFORM="$(PLATFORM)
	@echo "ARCH="$(ARCH)
	@echo "VPROC_DEV_CLASS="$(VPROC_DEV_CLASS)
	@echo "KSRC="$(KSRC)
	@echo "SHELL="$(SHELL)

cleanall: platform_clean ssl_clean hbi_clean hbilnx_clean codec_clean
	rm -rf $(INSTALL_LIB_PATH)
	
# make clean causes a script to go into each of the 
# project folders and run the make clean command
clean:
	rm -f *.symvers *.orders *.o 
	@ (										\
		for dir in $(PROJECTS) ; do						\
		DIR=$${dir}/;						\
		$(MAKE) -C $$DIR clean ;					\
		done                                                                    \
	)


# make install causes a script to go into each of the
# project folders and run the make install command
install_sdk:
	$(MAKE) -C $(ROOTDIR)/platform/$(PLATFORM) install_sdk

HEADER=$(ROOTDIR)/include/fwr_image_headers.h
header:
	@if [ -f $(HEADER) ]; then rm $(HEADER) ; \
	fi
	@printf "\n /*This file is generated - DO NOT EDIT*/\n" >> $(HEADER)
	echo "#ifndef __FWR_IMAGE_HEADERS__" >> $(HEADER)
	echo "#define __FWR_IMAGE_HEADERS__" >> $(HEADER)
	echo "//#define HBI_BOOTLOAD_FWR_STATIC" >> $(HEADER)
	echo "//#define HBI_BOOTLOAD_CFGREC_STATIC" >> $(HEADER)
	files=$(ls $(ZL380XX_FIRMWARE_IMAGES_PATH)/*.h 2> /dev/null | wc -l)
	for file in $(ZL380XX_FIRMWARE_IMAGES_PATH)/*.h; do \
	if [ -f "$$file" ]; then \
    echo "#include <$$file>" >> $(HEADER); \
	fi \
	done; \
	echo "#endif" >> $(HEADER)


header_empty:
	@if [ -f $(HEADER) ]; then rm $(HEADER) ; \
	fi
	@printf "\n /*This file is generated - DO NOT EDIT*/\n" >> $(HEADER)
	echo "#ifndef __FWR_IMAGE_HEADERS__" >> $(HEADER)
	echo "#define __FWR_IMAGE_HEADERS__" >> $(HEADER)

	echo "#endif" >> $(HEADER)

