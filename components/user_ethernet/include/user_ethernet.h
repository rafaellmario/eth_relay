#ifndef USER_ETHERNET_H
#define USER_ETHERNET_H

#define ETH_PHY_ADDR             1
#define ETH_PHY_RST_GPIO        -1       // Reset pin is not connected
#define ETH_OSC_ENAB        GPIO_NUM_16  // External oscillator pulled down at boot to allow IO0 strapping
#define ETH_MDC_GPIO        GPIO_NUM_23  // Management Data clock
#define ETH_MDIO_GPIO       GPIO_NUM_18  // Management Data Input/output
#define ETH_RMII_TX_EN      GPIO_NUM_21 
#define ETH_RMII_TX0        GPIO_NUM_19
#define ETH_RMII_TX1        GPIO_NUM_22
#define ETH_RMII_RX0        GPIO_NUM_25
#define ETH_RMII_RX1_EN     GPIO_NUM_26
#define ETH_RMII_CRS_DV     GPIO_NUM_27

#define USER_ETH_HOSTNAME "REMOTE-RELAY"

#define STATIC_IP               0 // Change for 1 to use static IP configuration 
#if STATIC_IP
    #define S_IP        "192.168.1.5"     
    #define GATEWAY     "192.168.1.1"    
    #define NETMASK     "255.255.255.0"
#endif /* STATIC_IP */


esp_err_t ethernet_setup(void);
bool user_eth_con_status(void);

#endif