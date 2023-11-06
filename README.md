# Xilinx-XDMA-Host-Software
## Intro
This repo provides program running on host PC for writing/reading data to/from Xilinx FPGA device which connected with host PC through [PCIe-XDMA](https://docs.xilinx.com/r/en-US/pg195-pcie-dma/Introduction). The provided program for writing /reading supports both AXI-Memory-Map and AXI-Stream DMA interface configurations.

## Get Started
This part introduces how to set up host PC and run program to write/read data to/from FPGA devices.
### Get Source Codes
Clone this repo to your host PC:
```shell
git clone --recursive https://github.com/wengwz/Xilinx-XDMA-Host-Software.git
```
The command above also downloads offical XDMA driver from [dma_ip_drivers](https://github.com/Xilinx/dma_ip_drivers).

Compile XDMA driver and host program:
```
# Compile host program
$ cd Xilinx-XDMA-Host-Software/
make xdma
# The generated executable is located in ./bin/

# Compile XDMA driver
$ cd ./dma_ip_drivers/XDMA/linux-kernel/xdma
make
```

### Load XDMA Driver
Before loading XDMA driver, you have to check that your FPGA with PCIe-XDMA has been recognized by the host PC as a PCIe device. You can run `lspci` command and check if there is:
```
...                                                           # other devices
01:00.0 Memory controller: Xilinx Corporation Device 7021     # Xilinx PCIe-XDMA(AXI-MM)
...                                                           # other devices
```
Or:
```
...                                                           # other devices
01:00.0 Serial controller: Xilinx Corporation Device 7021     # Xilinx PCIe-XDMA(AXI-Stream)
...                                                           # other devices
```
In most cases, once FPGA is programed by a new bitstream file, you need to reboot your host PC so that it can rescan PCIe devices and recognize your FPGA. But if your FPGA has been recognized successfully before and you don't change the **PCIe:Bars** configuration of XDMA IP in the new bitstream, you can run following commands to rescan PCIe devices:
```
$ sudo -i
$ echo 1 > /sys/class/pci_bus/0000:02/device/remove
$ echo 1 > /sys/bus/pci/rescan
$ exit
```
After executing commands above, you need to run `lspci` again and if you can't find your FPGA, you'd better try to reboot your host PC. 
**Note**: you may need to change `0000:02` to the number corresponding to your FPGA device. You can find these number from the output of `lspci`. For example, you need to use `0000:01` if you get the output of `lspci` as shown above.

Once you have check that your FPGA is recognized successsfully after programming, you can load XDMA driver by:
```
# Suppose that you are in the root directory of this repo now:
$ sudo ./dma_ip_drivers/XDMA/linux-kernel/tests/load_driver.sh
```
If successfully, you can see the following output:
```
interrupt_selection .
xdma                  110592  0
Loading driver...insmod xdma.ko interrupt_mode=2 ...

The Kernel module installed correctly and the xmda devices were recognized.
DONE
```
And when you can also find following files under `/dev` driectory, which are written/read by our host program to communicate with FPGA:
```
$ ls /dev/xdma*
...
/dev/xdma0_c2h_0
...
/dev/xdma0_h2c_0
```

### Run Read/Write Software
After loading XDMA driver successfully, you can run our host program to read/write data from/to our FPGA deivce. The executable is located in `./bin` directory and its detailed usage is as follows:

```
Usage: ./xdma_rw [OPTIONS]

The options are as follows:
   -r/w   Specify read/write from/to XDMA (mandatory option).
   -h     Display help information.
   -d device_name
          Specify the device name, default device name:
          Write - /dev/xdma0_h2c_0
          Read - /dev/xdma0_c2h_0
   -f file_name
          Specify the name of file to be read/write.
   -s byte_num
          Specify the size of one transaction(default: 32).
   -c trans_num
          Specify the total number of transactions(default: 1).
   -a address
          Specify the start address on the AXI bus(Only for AXI-MM)
```
**Note**: the size of specified file should be equal to (byte_num*trans_num) bytes.

Example:
```
$ sudo ./xdma_rw -r -f ./output_data.bin -s 1024 -c 4
# Read 4KB data from XDMA(AXI-Stream) to output_data.bin through 4 1KB-transactions.

$ sudo ./xdma_rw -w -f ./input_data.bin -s 2048 -c 512
# Write 1MB data from input_data.bin to XDMA(AXI-Stream) through 512 2KB-transactions.
```
### Simple Loopback Test
A script that automatically performs loopback tests on XDMA is also provided in this repo. It assumes that XDMA is configured in the AXI-Stream mode, and what user logic in FPGA does is just to pass data received from h2c channel back to c2h channel. In this script, you can specify the size and number of transactions to be written/read to/from FPGA. And the script first generates binary file containing random data of the specified size, writes these data to XDMA h2c channel, receives loopback data from c2h channel, and finally check if data read from XDMA is consistent with data written to it before.
```shell
$ sudo ./loopback_test.sh
```

## Related Links
The implementation of this repo refers to codes in the following links. And you can visit them to find more tutorials or guides about how to use Xilinx XDMA IP.
- Xilinx-FPGA-PCIe-XDMA-Tutorial: https://github.com/WangXuan95/Xilinx-FPGA-PCIe-XDMA-Tutorial/tree/main
- dma_ip_drivers: https://github.com/Xilinx/dma_ip_drivers
