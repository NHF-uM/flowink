#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "rgb_strip.h"
#include "net.h"

LOG_MODULE_REGISTER(main);

/*

手机关掉wifi之后，正常leave，但是之后内存池没有释放，新设备拿不到ip
[00:00:05.817,000] <inf> wifi_ap_dhcp: AP Mode is enabled.
[00:00:16.030,000] <dbg> wifi_ap_dhcp: wifi_event_handler: station: 42:DA:1A:EF:18:A8 joined 
[00:03:14.448,000] <dbg> wifi_ap_dhcp: wifi_event_handler: station: 42:DA:1A:EF:18:A8 leave 
[00:03:23.957,000] <dbg> wifi_ap_dhcp: wifi_event_handler: station: 28:D0:43:96:A6:2A joined 
[00:03:24.295,000] <err> net_dhcpv4_server: No free address found in address pool
[00:03:29.297,000] <err> net_dhcpv4_server: No free address found in address pool
[00:03:33.655,000] <dbg> wifi_ap_dhcp: wifi_event_handler: station: 28:D0:43:96:A6:2A leave 

[00:01:12.327,000] <dbg> http_server: data_up_handler: picture received succ (1152054 bytes).
[00:01:12.329,000] <dbg> http_server: data_up_handler: Transmission completed, including response

*/
int main(void)
{
	/* 等待ic初始化 */
	k_sleep(K_SECONDS(5));
	/* 手机直接关掉WiFi，不算断联？？ */
	wifi_init();
	http_server_start();
	return 0;
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
