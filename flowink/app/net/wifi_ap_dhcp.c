#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wifi_ap_dhcp, LOG_LEVEL_DBG);

#define STR_TO_MAC "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK                                               \
    (NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
     NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static struct net_if *ap_iface;

static struct net_mgmt_event_callback net_mgmt_cb;

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
                               struct net_if *iface)
{
    switch (mgmt_event)
    {
    case NET_EVENT_WIFI_AP_ENABLE_RESULT:
    {
        LOG_INF("AP Mode is enabled.");
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

        LOG_DBG("station: " STR_TO_MAC " joined ", sta_info->mac[0], sta_info->mac[1],
                sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
        break;
    }
    case NET_EVENT_WIFI_AP_STA_DISCONNECTED:
    {
        struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

        LOG_DBG("station: " STR_TO_MAC " leave ", sta_info->mac[0], sta_info->mac[1],
                sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
        break;
    }
    default:
        break;
    }
}

/// @brief 设置完成本机地址和掩码才能打开 dhcp
/// @param 无
static void enable_dhcpv4_server(void)
{
    static struct net_in_addr addr;
    static struct net_in_addr netmaskAddr;

    /* 字符串转地址 */
    if (net_addr_pton(NET_AF_INET, CONFIG_WIFI_AP_IP_ADDRESS, &addr))
    {
        LOG_ERR("Invalid address: %s", CONFIG_WIFI_AP_IP_ADDRESS);
        return;
    }

    if (net_addr_pton(NET_AF_INET, CONFIG_WIFI_AP_NETMASK, &netmaskAddr))
    {
        LOG_ERR("Invalid netmask: %s", CONFIG_WIFI_AP_NETMASK);
        return;
    }

    /* 为网络接口设置 IPv4 网关，在 ap 模式下不需要设置 */
    // net_if_ipv4_set_gw(ap_iface, &addr);

    if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL)
    {
        LOG_ERR("unable to set IP address for AP interface");
    }

    if (!net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &netmaskAddr))
    {
        LOG_ERR("Unable to set netmask for AP interface: %s",
                CONFIG_WIFI_AP_NETMASK);
    }

    /* 从 本机ip + 1 开始分配 */
    addr.s4_addr[3] += 1;

    if (net_dhcpv4_server_start(ap_iface, &addr) != 0)
    {
        LOG_ERR("DHCP server is not started for desired IP");
        return;
    }

    LOG_INF("DHCPv4 server started...\n");
}

static int enable_ap_mode(void)
{
    struct wifi_connect_req_params ap_config = {0};

    ap_config.ssid = (const uint8_t *)CONFIG_WIFI_AP_SSID;
    ap_config.ssid_length = sizeof(CONFIG_WIFI_AP_SSID) - 1;
    ap_config.psk = (const uint8_t *)CONFIG_WIFI_AP_PSK;
    ap_config.psk_length = sizeof(CONFIG_WIFI_AP_PSK) - 1;
    ap_config.channel = WIFI_CHANNEL_ANY;
    ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;

    if (sizeof(CONFIG_WIFI_AP_PSK) == 1)
    {
        ap_config.security = WIFI_SECURITY_TYPE_NONE;
    }
    else
    {
        ap_config.security = WIFI_SECURITY_TYPE_PSK;
    }

    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &ap_config,
                       sizeof(struct wifi_connect_req_params));
    if (ret)
    {
        LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
    }

    return ret;
}

void wifi_init(void)
{
    net_mgmt_init_event_callback(&net_mgmt_cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&net_mgmt_cb);

    ap_iface = net_if_get_wifi_sap();
    if (!ap_iface)
    {
        LOG_ERR("AP interface not found");
        return;
    }

    enable_dhcpv4_server();
    enable_ap_mode();
}

void wifi_deinit1(void)
{
    net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, ap_iface, NULL, 0);
    net_dhcpv4_server_stop(ap_iface);
}