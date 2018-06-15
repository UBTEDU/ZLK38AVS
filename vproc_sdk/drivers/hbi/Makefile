include $(ROOTDIR)/Makefile.globals
include $(ROOTDIR)/config.mk

LOCAL_PWD = $(ROOTDIR)/drivers/hbi

EXTRA_CFLAGS += -I$(LOCAL_PWD)/inc

obj-y := hbi.o

ifeq ($(TARGET),TW)
	lib-y := hbi_tw.o 	
endif
lib-y += hbi_ssl.o
lib-y += hbi.o

all:
	make -C $(KSRC) M=$(LOCAL_PWD) 
	mv hbi.o $(INSTALL_LIB_PATH)/hbi.o_shipped

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions modules.order Module.symvers *.a ../*.o ../*.mod.* ../.*cmd
