## <b>Nx_FtpServer Application Description</b>

This application provides an example of Azure RTOS NetX/NetXDuo stack usage.

It shows how to develop an FTP server application to remotely browse the content from SDCard.

The main entry function tx_application_define() is called by ThreadX during kernel start, at this stage, all NetX resources are created.

 + A <i>NX_PACKET_POOL "AppPool" </i>is allocated

 + A <i>NX_PACKET_POOL "FtpServerPool" </i>is allocated

 + A <i>NX_IP</i> instance using AppPool is initialized

 + A <i>FTPServer</i> instance using FtpServerPool is initialized

 + The <i>ARP</i>, <i>ICMP</i>, <i>UDP</i> and <i>TCP</i> protocols are enabled for the <i>NX_IP</i> instance

 + A <i>DHCP client is created.</i>

The application then creates 2 threads with the same priorities:

 + **AppMainThread** (priority 10, PreemtionThreashold 10) : created with the <i>TX_AUTO_START</i> flag to start automatically.

 + **AppFtpServerThread** (priority 5, PreemtionThreashold 5) : created with the <i>TX_DONT_START</i> flag to be started later.

The **AppMainThread** starts and perform the following actions:

  + Starts the DHCP client

  + Waits for the IP address resolution

  + Resumes the **FtpWebServerThread**

The **FtpWebServerThread**, once started:

  + Opens the SD Card disk driver

  + Starts FTP server.

####  <b>Expected success behavior</b>

 + The board IP address "IP@" is printed on the HyperTerminal

 + If the SD Card is missing, an error is shown on the HyperTerminal: "FX media opening failed". Insert an SDCard with FAT32 File System and reset the board.

 + The user can now connect to that IP using an FTP Client application (e.g. FileZilla)

#### <b>ThreadX usage hints</b>

 - ThreadX uses the Systick as time base, thus it is mandatory that the HAL uses a separate time base through the TIM IPs.

 - ThreadX is configured with 100 ticks/sec by default, this should be taken into account when using delays or timeouts at application. It is always possible to reconfigure it in the "tx_user.h", the "TX_TIMER_TICKS_PER_SECOND" define,but this should be reflected in "tx_initialize_low_level.s" file too.

 - ThreadX is disabling all interrupts during kernel start-up to avoid any unexpected behavior, therefore all system related calls (HAL, BSP) should be done either at the beginning of the application or inside the thread entry functions.

 - ThreadX offers the "tx_application_define()" function, that is automatically called by the tx_kernel_enter() API.
   It is highly recommended to use it to create all applications ThreadX related resources (threads, semaphores, memory pools...)  but it should not in any way contain a system API call (HAL or BSP).
 
 - Using dynamic memory allocation requires to apply some changes to the linker file.

   ThreadX needs to pass a pointer to the first free memory location in RAM to the tx_application_define() function,
   using the "first_unused_memory" argument.
   This require changes in the linker files to expose this memory location.
   
    + For EWARM add the following section into the .icf file:
     ```
	 place in RAM_region    { last section FREE_MEM };
	 ```
    + For MDK-ARM:
	```
    either define the RW_IRAM1 region in the ".sct" file
    or modify the line below in "tx_low_level_initilize.s to match the memory region being used
        LDR r1, =|Image$$RW_IRAM1$$ZI$$Limit|
	```
    + For STM32CubeIDE add the following section into the .ld file:
	``` 
    ._threadx_heap :
      {
         . = ALIGN(8);
         __RAM_segment_used_end__ = .;
         . = . + 64K;
         . = ALIGN(8);
       } >RAM_D1 AT> RAM_D1
	``` 
	
       The simplest way to provide memory for ThreadX is to define a new section, see ._threadx_heap above.
       In the example above the ThreadX heap size is set to 64KBytes.
       The ._threadx_heap must be located between the .bss and the ._user_heap_stack sections in the linker script.	 
       Caution: Make sure that ThreadX does not need more than the provided heap memory (64KBytes in this example).	 
       Read more in STM32CubeIDE User Guide, chapter: "Linker script".
	  
    + The "tx_initialize_low_level.s" should be also modified to enable the "USE_DYNAMIC_MEMORY_ALLOCATION" flag.
         
### <b>Keywords</b>

RTOS, Network, ThreadX, NetXDuo, FileX, WIFI, TCP, MXCHIP, UART


### <b>Hardware and Software environment</b>

  - This example runs on STM32U585xx devices with a WiFi module (MXCHIP:EMW3080) with this configuration :

    + MXCHIP Firmware 2.3.4

    + Bypass mode 

    + SPI mode used as interface

    Before using this project, you shall update your STEVAL-STWINBX1 board with the EMW3080B firmware version 2.3.4.

    To achieve this, use the EMW3080updateV2.3.4STWINbox.bin flasher under the EMW3080update/V2.3.4 folder.

  - This application has been tested with STEVAL-STWINBX1 board and can be easily tailored to any other supported device and development board.

  - This application uses USART2 to display logs, the hyperterminal configuration is as follows:
      - BaudRate = 115200 baud
      - Word Length = 8 Bits
      - Stop Bit = 1
      - Parity = None
      - Flow control = None

###  <b>How to use it ?</b>

In order to make the program work, you must do the following :

 - Open your preferred toolchain

 - On <code> Core/Inc/mx_wifi_conf.h </code> , Edit your Wifi Settings (WIFI_SSID,WIFI_PASSWORD)  

 - Rebuild all files and load your image into target memory

 - Run the application
