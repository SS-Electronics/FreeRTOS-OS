# FreeRTOS-OS

FreeRTOS-OS project is os implementation using FreeRTOS kernel apis. It contains  services, io drivers management, network management, and handeling communication. 
A light weight OS on top of FreeRTOS kernel apis. 

* Please check the supprted devices
* Install the requited tools using the scripts
* Config the hardware requirement
* Build the OS seperately for minimal operation
* Or build the entire project by  adding this repo as submodule

## Prerequisite

#### Installing compiler

```http
sudo bash install-arm-gcc.sh
```

#### Installing Kconfig tools

```http
sudo bash install_kconfig.sh
```

## Build

### Config your OS
* Try to run in a shell [ Not from VS Code terminal ]
```http
make menuconfig
```

### Generate necessary files
```http
make config-outputs
```

### build
```http
make all
```

### clean
```http
make clean
```

### Run the gdb server before debug

```http
sudo bash run_gdb.sh
```


## Device List
### STM

* Please ensure the Config IDs are mentioned in Target MCU while config OS

| SERIES | Config ID     | Arch                |
| :-------- | :------- | :------------------------- |
| STM32F4 | STM32F405xx | Arm® Cortex®-M4  |
| STM32F4 | STM32F415xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F407xx | Arm® Cortex®-M4  |
| STM32F4 | STM32F417xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F427xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F437xx | Arm® Cortex®-M4  |
| STM32F4 | STM32F429xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F439xx | Arm® Cortex®-M4  |
| STM32F4 | STM32F401xC | Arm® Cortex®-M4 Same as STM32F401xB |
| STM32F4 | STM32F401xE | Arm® Cortex®-M4 Same as STM32F401xD |
| STM32F4 | STM32F410Tx |  |
| STM32F4 | STM32F410Cx |  |
| STM32F4 | STM32F410Rx |  |
| STM32F4 | STM32F411xE | Arm® Cortex®-M4 |
| STM32F4 | STM32F446xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F469xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F479xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F412Cx | Arm® Cortex®-M4 |
| STM32F4 | STM32F412Zx | Arm® Cortex®-M4 |
| STM32F4 | STM32F412Rx | Arm® Cortex®-M4 |
| STM32F4 | STM32F412Vx | Arm® Cortex®-M4 |
| STM32F4 | STM32F413xx | Arm® Cortex®-M4 |
| STM32F4 | STM32F423xx | Arm® Cortex®-M4 |


