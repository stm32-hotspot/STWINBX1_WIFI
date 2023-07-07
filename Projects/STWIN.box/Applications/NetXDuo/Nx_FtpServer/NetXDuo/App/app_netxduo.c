/* USER CODE BEGIN Header */
/**
  ******************************************************************************
* @file    app_netxduo.c
* @author  MCD Application Team
* @brief   NetXDuo applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_netxduo.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include   "app_azure_rtos.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* Define the ThreadX , NetX and FileX object control blocks. */

/* Define Threadx global data structures. */
TX_THREAD AppMainThread;
TX_THREAD AppFtpServerThread;
TX_SEMAPHORE Semaphore;

/* Define NetX global data structures. */

NX_PACKET_POOL IpPool;
NX_PACKET_POOL FtpServerPool;

NX_IP   IpInstance;
NX_DHCP DHCPClient;

ULONG IpAddress;
ULONG NetMask;

/* App memory pointer. */
UCHAR   *pointer;

NX_FTP_SERVER ftp_server;

/* Define FileX global data structures. */

/* the web server reads the web content from the uSD, a FX_MEDIA instance is required */
//FX_MEDIA   nor_flash_disk;
//BSP_OSPI_NOR_Info_t ospi_info;
//UINT *media_memory;
///* Buffer for FileX FX_MEDIA sector cache. this should be 32-Bytes aligned to avoid
//cache maintenance issues */
//ALIGN_32BYTES (uint32_t DataBuffer[512]);

ALIGN_32BYTES (uint32_t media_memory[FX_STM32_SD_DEFAULT_SECTOR_SIZE / sizeof(uint32_t)]);

/* Define FileX global data structures.  */
FX_MEDIA        sdio_disk;
FX_FILE         fx_file;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
TX_THREAD LedThread;
void LedThread_Entry(ULONG thread_input);

/* USER CODE END PV */
#ifdef USE_IPV6
UINT    server_login6(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ipduo_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
UINT    server_logout6(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ipduo_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
#else
UINT    server_login(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
UINT    server_logout(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info);
#endif
/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* WEB HTTP server thread entry */
static void  App_Main_Thread_Entry(ULONG thread_input);
static void  nx_server_thread_entry(ULONG thread_input);

/* DHCP state change notify callback */
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr);

/* USER CODE END PFP */
/**
  * @brief  Application NetXDuo Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_NetXDuo_Init(VOID *memory_ptr)
{
  UINT ret = NX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

   /* USER CODE BEGIN App_NetXDuo_MEM_POOL */

  /* USER CODE END App_NetXDuo_MEM_POOL */

  /* USER CODE BEGIN MX_NetXDuo_Init */
#if (USE_MEMORY_POOL_ALLOCATION == 1)  
  printf("Nx_FtpServer_Application_Started..\n");
  
  /* Initialize the NetX system. */
  nx_system_initialize();

  /* Allocate the memory for packet_pool.  */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,  NX_PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }

  /* Create the Packet pool to be used for packet allocation */
  ret = nx_packet_pool_create(&IpPool, "Main Packet Pool", PAYLOAD_SIZE, pointer, NX_PACKET_POOL_SIZE);

  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the server packet pool. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, FTP_SERVER_POOL_SIZE, TX_NO_WAIT);
  
  /* Check server packet pool memory allocation. */
  if (ret != NX_SUCCESS)
  {
    printf("Packed pool memory allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }
  
  /* Create the server packet pool. */
  ret = nx_packet_pool_create(&FtpServerPool, "FTP Server Packet Pool", 512, pointer, FTP_SERVER_POOL_SIZE);

  /* Check for server pool creation status. */
  if (ret != NX_SUCCESS)
  {
    printf("Server pool creation failed : 0x%02x\n", ret);
    Error_Handler();
  }
  
  /* Allocate the memory for Ip_Instance */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer,   2 * DEFAULT_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* Create the main NX_IP instance */
  ret = nx_ip_create(&IpInstance, "Main Ip instance", NULL_ADDRESS, NULL_ADDRESS, &IpPool, nx_driver_emw3080_entry,
                     pointer, 2 * DEFAULT_MEMORY_SIZE, DEFAULT_PRIORITY);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the memory for ARP */
  if (tx_byte_allocate(byte_pool, (VOID **) &pointer, ARP_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  
  /* Enable the ARP protocol and provide the ARP cache size for the IP instance */
  ret = nx_arp_enable(&IpInstance, (VOID *)pointer, ARP_MEMORY_SIZE);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  } 
  
  /* Enable the ICMP */
  ret = nx_icmp_enable(&IpInstance);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Enable the UDP protocol required for  DHCP communication */
  ret = nx_udp_enable(&IpInstance);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }  
  
  /* Enable the TCP protocol */
  ret = nx_tcp_enable(&IpInstance);
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the server stack. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, FTP_SERVER_STACK, TX_NO_WAIT);
  
  /* Check server stack memory allocation. */
  if (ret != NX_SUCCESS)
  {
    printf("Server stack memory allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }
  
  /* Create the FTP server.  */
  ret = nxd_ftp_server_create(&ftp_server, "FTP Server Instance", &IpInstance, &sdio_disk, pointer, FTP_SERVER_STACK, &FtpServerPool,
                                    server_login, server_logout);
  if (ret != NX_SUCCESS)
  {
    printf("FTP Server creation failed: 0x%02x\n", ret);
    Error_Handler();
  }
  
  /* Allocate the main thread. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, 2 * DEFAULT_MEMORY_SIZE, TX_NO_WAIT);
  
  /* Check main thread memory allocation. */
  if (ret != NX_SUCCESS)
  {
    printf("Main thread memory allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }
  
  /* Create the main thread */
  ret = tx_thread_create(&AppMainThread, "App Main thread", App_Main_Thread_Entry, 0, pointer, 2 * DEFAULT_MEMORY_SIZE,
                         DEFAULT_MAIN_PRIORITY, DEFAULT_MAIN_PRIORITY, TX_NO_TIME_SLICE, TX_AUTO_START);
  
  if (ret != TX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* Allocate the Web Server Thread stack. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, 2 * DEFAULT_MEMORY_SIZE, TX_NO_WAIT);
  
  /* Check server thread memory allocation. */
  if (ret != NX_SUCCESS)
  {
    printf("Server thread memory allocation failed : 0x%02x\n", ret);
    Error_Handler();
  }
  
  /* create the Web Server Thread */
  ret = tx_thread_create(&AppFtpServerThread, "App Ftp Server Thread", nx_server_thread_entry, 0, pointer, 2 * DEFAULT_MEMORY_SIZE,
                         DEFAULT_PRIORITY, DEFAULT_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START);
  
  if (ret != TX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* create the DHCP client */
  ret = nx_dhcp_create(&DHCPClient, &IpInstance, "DHCP Client");
  
  if (ret != NX_SUCCESS)
  {
    return NX_NOT_ENABLED;
  }
  
  /* set DHCP notification callback  */
  tx_semaphore_create(&Semaphore, "App Semaphore", 0);
  
    /* Allocate the LED thread stack. */
  ret = tx_byte_allocate(byte_pool, (VOID **) &pointer, DEFAULT_MEMORY_SIZE, TX_NO_WAIT);

  /* Check LED thread memory allocation. */
  if (ret != NX_SUCCESS)
  {
    printf("led thread allocation failed: 0x%02x\n", ret);
    Error_Handler();
  }

  /* Create Led Thread.  */
  if (tx_thread_create(&LedThread, "Led Thread", LedThread_Entry, 0, pointer, DEFAULT_MEMORY_SIZE,
                       DEFAULT_PRIORITY, DEFAULT_PRIORITY, TX_NO_TIME_SLICE, TX_DONT_START) != TX_SUCCESS)
  {
    Error_Handler();
  }
  
#endif
  /* USER CODE END MX_NetXDuo_Init */

  return ret;
}

/* USER CODE BEGIN 1 */
/**
* @brief  ip address change callback
* @param  ip_instance : NX_IP instance registered for this callback
* @param   ptr : VOID * optional data pointer
* @retval None
*/
static VOID ip_address_change_notify_callback(NX_IP *ip_instance, VOID *ptr)
{
  /* as soon as the IP address is ready, the semaphore is released to let the web server start */
  tx_semaphore_put(&Semaphore);
}


static VOID App_Main_Thread_Entry(ULONG thread_input)
{
  UINT ret;
  
  ret = nx_ip_address_change_notify(&IpInstance, ip_address_change_notify_callback, NULL);
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  ret = nx_dhcp_start(&DHCPClient);
  if (ret != NX_SUCCESS)
  {
    Error_Handler();
  }
  
  /* wait until an IP address is ready */
  if(tx_semaphore_get(&Semaphore, TX_WAIT_FOREVER) != TX_SUCCESS)
  {
    Error_Handler();
  }
  /* get IP address */
  ret = nx_ip_address_get(&IpInstance, &IpAddress, &NetMask);
  
  PRINT_IP_ADDRESS(IpAddress);
  
  if (ret != TX_SUCCESS)
  {
    Error_Handler();
  }
  /* the network is correctly initialized, start the TCP server thread */
  tx_thread_resume(&AppFtpServerThread);
  
  /* this thread is not needed any more, we relinquish it */
  tx_thread_relinquish();
  
  return;
}


void nx_server_thread_entry(ULONG thread_input)
{
  /* HTTP WEB SERVER THREAD Entry */
  UINT    status;
  NX_PARAMETER_NOT_USED(thread_input);

  /* Open the SD disk driver.  */
  status =  fx_media_open(&sdio_disk, "STM32_SDIO_DISK", fx_stm32_sd_driver, 0,(VOID *) media_memory, sizeof(media_memory));

  /* Check the media opening status. */
  if (status != FX_SUCCESS)
  {
    /*Print Media Opening error. */
    printf("FX media opening failed : 0x%02x\n", status);
    /* Error, call error handler.*/
    Error_Handler();
  }
  else
  {
    /* Print Media Opening Success. */
    printf("Fx media successfully opened.\n");
  }
  
  /* OK to start the FTP Server.   */
  status = nx_ftp_server_start(&ftp_server);
  
//  status = nx_web_http_server_mime_maps_additional_set(&HTTPServer,&my_mime_maps[0], 4);
//
//  /* Start the WEB HTTP Server. */
//  status = nx_web_http_server_start(&HTTPServer);
  
  /* Check the WEB HTTP Server starting status. */
  if (status != NX_SUCCESS)
  {
    /* Print FTP Server starting error. */
    printf("FTP Server Starting Failed, error: 0x%02x\n", status);
    /* Error, call error handler.*/
    Error_Handler();
  }
  else
  {
    /* Print FTP Server Starting success. */
    printf("FTP Server successfully started.\n");
    /* LED_GREEN On. */
    tx_thread_resume(&LedThread);
  }
}

void LedThread_Entry(ULONG thread_input)
{
  (void) thread_input;
  /* Infinite loop */
  while (1)
  {
    BSP_LED_On(LED_GREEN);
    DELAY_MS(50);
//    tx_thread_sleep(50);
    BSP_LED_Off(LED_GREEN);
    DELAY_MS(1000);
//    tx_thread_sleep(1000);
  }
}

#ifdef USE_IPV6
UINT  server_login6(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ipduo_address, UINT client_port,
                   CHAR *name, CHAR *password, CHAR *extra_info)
{
    NX_PARAMETER_NOT_USED(ftp_server_ptr);
    NX_PARAMETER_NOT_USED(client_ipduo_address);
    NX_PARAMETER_NOT_USED(client_port);
    NX_PARAMETER_NOT_USED(name);
    NX_PARAMETER_NOT_USED(password);
    NX_PARAMETER_NOT_USED(extra_info);

    printf("Logged in6!\n");

    /* Always return success.  */
    return(NX_SUCCESS);
}

UINT  server_logout6(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ipduo_address, UINT client_port,
                    CHAR *name, CHAR *password, CHAR *extra_info)
{
    NX_PARAMETER_NOT_USED(ftp_server_ptr);
    NX_PARAMETER_NOT_USED(client_ipduo_address);
    NX_PARAMETER_NOT_USED(client_port);
    NX_PARAMETER_NOT_USED(name);
    NX_PARAMETER_NOT_USED(password);
    NX_PARAMETER_NOT_USED(extra_info);

    printf("Logged out6!\n");

    /* Always return success.  */
    return(NX_SUCCESS);
}
#else
UINT  server_login(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info)
{
    NX_PARAMETER_NOT_USED(ftp_server_ptr);
    NX_PARAMETER_NOT_USED(client_ip_address);
    NX_PARAMETER_NOT_USED(client_port);
    NX_PARAMETER_NOT_USED(name);
    NX_PARAMETER_NOT_USED(password);
    NX_PARAMETER_NOT_USED(extra_info);

//#define FTP_USERNAME  "sonoio"
//#define FTP_PASSWORD  "sette"
//
//    printf("name: %s\n", name);
//    printf("password: %s\n", password);
//
//    if(strcmp(name, FTP_USERNAME) != 0 || strcmp(password, FTP_PASSWORD) != 0)
//    {
//      printf("Wrong username or password!\n");
//      return(NX_INVALID_PARAMETERS);
//    }
    printf("Logged in!\n");
    /* Return success.  */
    return(NX_SUCCESS);
}

UINT  server_logout(struct NX_FTP_SERVER_STRUCT *ftp_server_ptr, NXD_ADDRESS *client_ip_address, UINT client_port, CHAR *name, CHAR *password, CHAR *extra_info)
{
    NX_PARAMETER_NOT_USED(ftp_server_ptr);
    NX_PARAMETER_NOT_USED(client_ip_address);
    NX_PARAMETER_NOT_USED(client_port);
    NX_PARAMETER_NOT_USED(name);
    NX_PARAMETER_NOT_USED(password);
    NX_PARAMETER_NOT_USED(extra_info);

    printf("Logged out!\n");

    /* Always return success.  */
    return(NX_SUCCESS);
}
#endif  /* USE_IPV6 */
/* USER CODE END 1 */
