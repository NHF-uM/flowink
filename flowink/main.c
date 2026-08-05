#define LOG_LEVEL 4
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "rgb_strip.h"

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK                                               \
    (NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
     NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static struct net_if *ap_iface;
static struct net_mgmt_event_callback net_event_cb;

static struct wifi_connect_req_params ap_config;

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
                               struct net_if *iface)
{
    switch (mgmt_event)
    {
    case NET_EVENT_WIFI_AP_ENABLE_RESULT:
    {
        LOG_INF("AP Mode is enabled. Waiting for station to connect");
        break;
    }
    case NET_EVENT_WIFI_AP_DISABLE_RESULT:
    {
        LOG_INF("AP Mode is disabled.");
        break;
    }
    case NET_EVENT_WIFI_AP_STA_CONNECTED:
    {
        struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

        LOG_INF("station: " MACSTR " joined ", sta_info->mac[0], sta_info->mac[1],
                sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
        break;
    }
    case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
    {
        struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

        LOG_INF("station: " MACSTR " leave ", sta_info->mac[0], sta_info->mac[1],
                sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
        break;
    }
    default:
        break;
    }
}

static void enable_dhcpv4_server(void)
{
    static struct net_in_addr addr;
    static struct net_in_addr netmaskAddr;

    /* 字符串转地址 */
    if (net_addr_pton(NET_AF_INET, CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS, &addr))
    {
        LOG_ERR("Invalid address: %s", CONFIG_WIFI_SAMPLE_AP_IP_ADDRESS);
        return;
    }

    if (net_addr_pton(NET_AF_INET, CONFIG_WIFI_SAMPLE_AP_NETMASK, &netmaskAddr))
    {
        LOG_ERR("Invalid netmask: %s", CONFIG_WIFI_SAMPLE_AP_NETMASK);
        return;
    }

    /* 为网络接口设置 IPv4 网关。 */
    net_if_ipv4_set_gw(ap_iface, &addr);

    if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL)
    {
        LOG_ERR("unable to set IP address for AP interface");
    }

    if (!net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &netmaskAddr))
    {
        LOG_ERR("Unable to set netmask for AP interface: %s",
                CONFIG_WIFI_SAMPLE_AP_NETMASK);
    }

    addr.s4_addr[3] += 10; /* Starting IPv4 address for DHCPv4 address pool. */

    if (net_dhcpv4_server_start(ap_iface, &addr) != 0)
    {
        LOG_ERR("DHCP server is not started for desired IP");
        return;
    }

    LOG_INF("DHCPv4 server started...\n");
}

int main(void)
{
    /* 等待ic初始化 */
    k_sleep(K_SECONDS(5));

    net_mgmt_init_event_callback(&net_event_cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&net_event_cb);

    ap_iface = net_if_get_wifi_sap();

    if (!ap_iface)
    {
        LOG_INF("AP: is not initialized");
        return -EIO;
    }

    LOG_INF("Turning on AP Mode");
    ap_config.ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_AP_SSID;
    ap_config.ssid_length = sizeof(CONFIG_WIFI_SAMPLE_AP_SSID) - 1;
    ap_config.psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_AP_PSK;
    ap_config.psk_length = sizeof(CONFIG_WIFI_SAMPLE_AP_PSK) - 1;
    ap_config.channel = WIFI_CHANNEL_ANY;
    ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;

    if (sizeof(CONFIG_WIFI_SAMPLE_AP_PSK) == 1)
    {
        ap_config.security = WIFI_SECURITY_TYPE_NONE;
    }
    else
    {

        ap_config.security = WIFI_SECURITY_TYPE_PSK;
    }

    enable_dhcpv4_server();

    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &ap_config,
                       sizeof(struct wifi_connect_req_params));
    if (ret)
    {
        LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
    }








    // http_server_start();

    while (1)
    {
        rgb_strip_on(BLUE);
        k_sleep(K_SECONDS(1));
        rgb_strip_off();
        k_sleep(K_SECONDS(1));
    }
    return 0;
}

/*

连接一次之后就卡死了，ap热点也找不到了：

Build:Mar 27 2021
rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x40378eba
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fc91f30,len:0x5324
load:0x40374000,len:0xdf20
load:0x50000000,len:0x24
SHA-256 comparison failed:
Calculated: 0b898f4b5f7721db68f9b71429d06b68849758634e72c93667dd9095aeac9ed0
Expected: 0000000050cd0000000000000000000000000000000000000000000000000000
Attempting to boot anyway...
entry 0x4037c32c
I (soc_init): ESP Simple boot
I (soc_init): compile time Aug  5 2026 12:27:32
W (soc_init): Unicore bootloader
I (soc_init): chip revision: v0.2
I (flash_init): Boot SPI Speed : 80MHz
I (flash_init): SPI Mode       : DIO
I (flash_init): SPI Flash Size : 16MB
I (boot): DRAM	: lma=00000020h vma=3fc91f30h size=05324h ( 21284)
I (boot): IRAM	: lma=0000534ch vma=40374000h size=0df20h ( 57120)
I (boot): RTC_DATA	: lma=00013274h vma=50000000h size=00024h (    36)
I (boot): IROM	: lma=00020000h vma=42000000h size=5ecb8h (388280)
I (boot): DROM	: lma=00080000h vma=3c060000h size=0da28h ( 55848)
I (boot): libc heap size 200 kB.
I (cache): Instruction cache: size 16KB, 8Ways, cache line size 32Byte
I (spi_flash): detected chip: boya
I (spi_flash): flash io: dio
I (pp): pp rom version: e7ae62f
I (net80211): net80211 rom version: e7ae62f
I (wifi_init): rx ba win: 6
*** Booting Zephyr OS build v4.4.0-645-ga0372ca149a6 ***
[00:00:05.087,000] <inf> main: Turning on AP Mode
[00:00:05.088,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start: Started DHCPv4 server, address pool:
[00:00:05.088,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start: 	 0: 192.168.4.11
[00:00:05.088,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start: 	 1: 192.168.4.12
[00:00:05.088,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start: 	 2: 192.168.4.13
[00:00:05.088,000] <dbg> net_dhcpv4_server: net_dhcpv4_server_start: 	 3: 192.168.4.14
[00:00:05.088,000] <inf> main: DHCPv4 server started...

[00:00:05.696,000] <wrn> net_if: iface 2 pkt 0x3fcaf144 send failure status -5
[00:00:05.697,000] <inf> main: AP Mode is enabled. Waiting for station to connect
[00:01:36.998,000] <inf> main: station: 42:DA:1A:EF:18:A8 joined 
[00:01:37.161,000] <dbg> net_dhcpv4_server: dhcpv4_handle_discover: DHCPv4 processing Discover - reserved 192.168.4.11


*/