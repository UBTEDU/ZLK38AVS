include $(ROOTDIR)/Makefile.globals
include $(ROOTDIR)/config.mk

LOCAL_PWD=$(ROOTDIR)/platform/$(PLATFORM)/kernel/dts
ifneq	 ($(CROSS_COMPILE),)
DTC:=$(KSRC)/scripts/dtc/dtc
else
DTC:=dtc
endif

all:
	$(DTC) -W no-unit_address_vs_reg -@ -I dts -O dtb -o microsemi-spi-overlay.dtbo microsemi-spi-overlay.dts
	$(DTC) -W no-unit_address_vs_reg -@ -I dts -O dtb -o microsemi-dac-overlay.dtbo microsemi-dac-overlay.dts
	$(DTC) -W no-unit_address_vs_reg -@ -I dts -O dtb -o microsemi-spi-multi-tw-overlay.dtbo microsemi-spi-multi-tw-overlay.dts
	$(DTC) -W no-unit_address_vs_reg -@ -I dts -O dtb -o microsemi-spi-multi-tw-dt-overlay.dtbo microsemi-spi-multi-tw-dt-overlay.dts
	$(DTC) -W no-unit_address_vs_reg -@ -I dts -O dtb -o microsemi-i2c-multi-tw-dt-overlay.dtbo microsemi-i2c-multi-tw-dt-overlay.dts
	if [ -z "$(CROSS_COMPILE)" ]; then \
	sudo cp *.dtbo /boot/overlays; \
	fi
	mv *.dtb* $(INSTALL_LIB_PATH)

clean:
	rm -rf   *.dtb* 
