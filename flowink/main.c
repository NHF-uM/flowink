#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/dhcpv4_server.h>
#include "rgb_strip.h"

LOG_MODULE_REGISTER(main);

/* 还是有一个警告：
 * [00:00:05.695,000] <wrn> net_if: iface 2 pkt 0x3fcaf654 send failure status -5
 */

#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define NET_EVENT_WIFI_MASK                                               \
	(NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT | \
	 NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED)

static struct net_if *ap_iface;

static struct wifi_connect_req_params ap_config;

static struct net_mgmt_event_callback net_mgmt_cb;

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

	/* 为网络接口设置 IPv4 网关。 */
	net_if_ipv4_set_gw(ap_iface, &addr);

	if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL)
	{
		LOG_ERR("unable to set IP address for AP interface");
	}

	if (!net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &netmaskAddr))
	{
		LOG_ERR("Unable to set netmask for AP interface: %s",
				CONFIG_WIFI_AP_NETMASK);
	}

	addr.s4_addr[3] += 10; /* Starting IPv4 address for DHCPv4 address pool. */

	if (net_dhcpv4_server_start(ap_iface, &addr) != 0)
	{
		LOG_ERR("DHCP server is not started for desired IP");
		return;
	}

	LOG_INF("DHCPv4 server started...\n");
}

static int enable_ap_mode(void)
{
	if (!ap_iface)
	{
		LOG_INF("AP: is not initialized");
		return -EIO;
	}

	LOG_INF("Turning on AP Mode");
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

	enable_dhcpv4_server();

	int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &ap_config,
					   sizeof(struct wifi_connect_req_params));
	if (ret)
	{
		LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
	}

	return ret;
}

void rgb_strip_thread_entry(void)
{
	while (1)
	{
		rgb_strip_on(BLUE);
		k_sleep(K_SECONDS(1));
		rgb_strip_on(RED);
		k_sleep(K_SECONDS(1));
	}
}

K_THREAD_DEFINE(rgb_strip_thread, 1024, rgb_strip_thread_entry, NULL, NULL, NULL,
				7, 0, 0);

// #include <zephyr/net/http/server.h>
// #include <zephyr/net/http/service.h>
// #include <zephyr/net/net_ip.h>
// #include <zephyr/net/socket.h>
// #include <zephyr/net/net_config.h>
// #include <zephyr/sys/util.h>
// #include <zephyr/data/json.h>
// #include <zephyr/sys/util_macro.h>
// #include <zephyr/net/net_config.h>

// #include <zephyr/net/tls_credentials.h>

#include <stdio.h>
#include <inttypes.h>

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/led.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/net/net_config.h>

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.content_encoding = "gzip",
			.content_type = "text/html",
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

static const uint16_t http_service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, http_service, "/",
		     &index_html_gz_resource_detail);

int main(void)
{
	/* 等待ic初始化 */
	k_sleep(K_SECONDS(5));

	net_mgmt_init_event_callback(&net_mgmt_cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
	net_mgmt_add_event_callback(&net_mgmt_cb);

	ap_iface = net_if_get_wifi_sap();

	enable_ap_mode();

	http_server_start();
	return 0;
}

/*
6. _detail 用户私有自定义数据
任意指针，会挂载在 http_service 结构体里。
业务场景：
传入设备配置结构体、状态指针；
在路由回调函数中通过 service->detail 拿到业务上下文，不用全局变量。
7. _res_fallback 兜底资源（404 页面）
当访问的 URL 没有匹配任何已注册路由时，就返回这个资源。
一般用 HTTP_RESOURCE_STATIC() 定义一个返回 404 html 的静态资源。
*/
