include $(ROOTDIR)/Makefile.globals
include $(ROOTDIR)/config.mk

LOCAL_PWD=$(ROOTDIR)/platform/$(PLATFORM)/driver/ssl

obj-y := libssl_port.o

libssl_port-y := hal_port.o

libssl_port-y += ssl_port.o

all:help
	make -C $(KSRC) M=$(LOCAL_PWD)
	mv libssl_port.o $(INSTALL_LIB_PATH)/ssl_port.o_shipped

help:
	@echo "$(VPROC_DEV_BUS)="$(VPROC_DEV_BUS)

clean:
	rm -rf   *.o *.ko .*cmd *.mod.c .tmp_versions modules.order Module.symvers *.a ../*.o ../*.mod.* ../.*cmd
