include $(ROOTDIR)/Makefile.globals
include $(ROOTDIR)/config.mk
LOCAL_PWD=$(ROOTDIR)/platform/$(PLATFORM)/driver/sound/lnxalsa/soc/codec
KBUILD_EXTRA_SYMBOLS := $(ROOTDIR)/lnxdrivers/lnxhbi/lnxkernel/Module.symvers
ccflags-y += -I$(ROOTDIR)/include
ccflags-y += -D$(BUILD_TYPE) 
ccflags-y += -zmuldefs
ccflags-y += -I$(LOCAL_PWD)/inc
ccflags-y += -I$(ROOTDIR)/drivers/hbi/inc

LIB_PATH := ../../../libs

snd-soc-zl380xx-objs := zl380xx_codec.o
 

obj-m	+= snd-soc-zl380xx.o


all:
	make -C $(KSRC) M=$(LOCAL_PWD)
	mv *.ko $(INSTALL_LIB_PATH)/

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions modules.order Module.symvers *.a ../*.o ../*.mod.* ../.*cmd
