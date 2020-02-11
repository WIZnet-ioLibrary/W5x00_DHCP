#include <stdio.h>
#include <stdlib.h>
#include "HAL_Config.h"
#include "HALInit.h"
#include "wizchip_conf.h"
#include "inttypes.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "W5x00RelFunctions.h"
#include "serialCommand.h"
#include "Internet\DHCP\dhcp.h"

wiz_NetInfo gWIZNETINFO = { .mac = {0x00,0x08,0xdc,0x78,0x91,0x71},
							.ip = {192,168,0,15},
							.sn = {255, 255, 255, 0},
							.gw = {192, 168, 0, 1},
							.dns = {168, 126, 63, 1},
							.dhcp = NETINFO_STATIC};

#define ETH_MAX_BUF_SIZE	2048

unsigned char ethBuf0[ETH_MAX_BUF_SIZE];
unsigned char ethBuf1[ETH_MAX_BUF_SIZE];
unsigned char ethBuf2[ETH_MAX_BUF_SIZE];
unsigned char ethBuf3[ETH_MAX_BUF_SIZE];


uint8_t bLoopback = 1;
uint8_t bRandomPacket = 0;
uint8_t bAnyPacket = 0;
uint16_t pack_size = 0;
volatile uint32_t msTicks; 		/* counts 1ms timeTicks */
uint8_t ret = 0;
uint8_t dhcp_retry = 0;


void print_network_information(void);
int process_dhcp(void);
int main(void)
{
	uint8_t dnsclient_ip[16] = {0,};
	//datetime time;
	volatile int i;
	volatile int j,k;
	RCCInitialize();
	gpioInitialize();
	usartInitialize();
	timerInitialize();
	//SysTick_Config((SystemCoreClock/1000));
	printf("System start.\r\n");




#if _WIZCHIP_IO_MODE_ & _WIZCHIP_IO_MODE_SPI_
	// SPI method callback registration
	reg_wizchip_spi_cbfunc(spiReadByte, spiWriteByte);
	// CS function register
	reg_wizchip_cs_cbfunc(csEnable,csDisable);

#else
	// Indirect bus method callback registration
	reg_wizchip_bus_cbfunc(busReadByte, busWriteByte);
#endif

#if _WIZCHIP_IO_MODE_ == _WIZCHIP_IO_MODE_BUS_INDIR_
	FSMCInitialize();
#else
	spiInitailize();
#endif
	resetAssert();
	delay(10);
	resetDeassert();
	delay(50);
	W5x00Initialze();
	

#if _WIZCHIP_ != W5200
	printf("\r\nCHIP Version: %02x\r\n", getVER());
#endif
	wizchip_setnetinfo(&gWIZNETINFO);
    //print_network_information();
	DHCP_init(0, ethBuf0);

	while(1){
		if(process_dhcp() ==DHCP_IP_LEASED){
			printf("DHCP Success\r\n");
			break;
		}
		else
			printf("Try.....\r\n");
	}

	while(1);

}

void delay(unsigned int count)
{
	int temp;
	temp = count + TIM2_gettimer();
	while(temp > TIM2_gettimer()){}
}

void print_network_information(void)
{
	memset(&gWIZNETINFO,0,sizeof(gWIZNETINFO));

	wizchip_getnetinfo(&gWIZNETINFO);
	printf("MAC Address : %02x:%02x:%02x:%02x:%02x:%02x\n\r",gWIZNETINFO.mac[0],gWIZNETINFO.mac[1],gWIZNETINFO.mac[2],gWIZNETINFO.mac[3],gWIZNETINFO.mac[4],gWIZNETINFO.mac[5]);
	printf("IP  Address : %d.%d.%d.%d\n\r",gWIZNETINFO.ip[0],gWIZNETINFO.ip[1],gWIZNETINFO.ip[2],gWIZNETINFO.ip[3]);
	printf("Subnet Mask : %d.%d.%d.%d\n\r",gWIZNETINFO.sn[0],gWIZNETINFO.sn[1],gWIZNETINFO.sn[2],gWIZNETINFO.sn[3]);
	printf("Gateway     : %d.%d.%d.%d\n\r",gWIZNETINFO.gw[0],gWIZNETINFO.gw[1],gWIZNETINFO.gw[2],gWIZNETINFO.gw[3]);
	printf("DNS Server  : %d.%d.%d.%d\n\r",gWIZNETINFO.dns[0],gWIZNETINFO.dns[1],gWIZNETINFO.dns[2],gWIZNETINFO.dns[3]);
}

int process_dhcp(void){
	int8_t ret =0;
	while(1){
		ret = DHCP_run();
		if(ret == DHCP_IP_LEASED)
		{
			#ifdef _MAIN_DEBUG_
				printf(" - DHCP Success\r\n");
			#endif
				printf("=============================\r\n");
				print_network_information();
				printf("=============================\r\n");
				break;
		}
		else if(ret == DHCP_FAILED)
		{
			dhcp_retry++;
			#ifdef _MAIN_DEBUG_
				if(dhcp_retry <= 3) printf(" - DHCP Timeout occurred and retry [%d]\r\n", dhcp_retry);
			#endif
		}


		if(dhcp_retry > 3)
		{
			#ifdef _MAIN_DEBUG_
				printf(" - DHCP Failed\r\n\r\n");
			#endif
			DHCP_stop();
			break;
		}
	}
	return ret;
}
