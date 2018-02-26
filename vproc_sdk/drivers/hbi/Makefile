include $(ROOTDIR)/Makefile.globals
include $(ROOTDIR)/config.mk

LOCAL_PWD = $(ROOTDIR)/drivers/hbi

EXTRA_CFLAGS += -I$(LOCAL_PWD)/inc

obj-y := libhbi.o

ifeq ($(TARGET),TW)
	libhbi-y := hbi_tw.o 	
endif
libhbi-y += hbi_ssl.o
libhbi-y += hbi.o

all:
	make -C $(KSRC) M=$(LOCAL_PWD) 
	mv libhbi.o $(INSTALL_LIB_PATH)/hbi.o_shipped

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions modules.order Module.symvers *.a ../*.o ../*.mod.* ../.*cmd
