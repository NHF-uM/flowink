#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "rgb_strip.h"

LOG_MODULE_REGISTER(main);

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
// #include <zephyr/sys/util.h>
// #include <zephyr/data/json.h>
// #include <zephyr/sys/util_macro.h>
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

static uint16_t http_service_port = CONFIG_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, http_service, "/",
		     &index_html_gz_resource_detail);


#include "net.h"

int main(void)
{
	/* 等待ic初始化 */
	k_sleep(K_SECONDS(5));

	wifi_init();
	// http_server_start();
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
