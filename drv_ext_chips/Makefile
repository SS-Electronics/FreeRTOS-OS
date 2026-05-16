# File:   drv_ext_chips/Makefile
# Module: drv_ext_chips — external chip drivers (I2C, SPI, ...)

include autoconf.mk

ifeq ($(CONFIG_INC_SERVICE_IIC_MGMT),1)
obj-y += drv_ext_chips/pca9685_iic.o
obj-y += drv_ext_chips/lsm303dlhc.o
endif
