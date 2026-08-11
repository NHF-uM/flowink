#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "rgb_strip.h"
#include "net.h"

LOG_MODULE_REGISTER(main);

#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
// #include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/net/net_config.h>

static uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static uint8_t script_js_gz[] = {
#include "script.js.gz.inc"
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

static struct http_resource_detail_static script_js_gz_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_encoding = "gzip",
		.content_type = "text/javascriptt",
	},
	.static_data = script_js_gz,
	.static_data_len = sizeof(script_js_gz),
};

// static size_t data_received;

// static int data_up_handler(struct http_client_ctx *client, enum http_transaction_status status,
// 						   const struct http_request_ctx *request_ctx,
// 						   struct http_response_ctx *response_ctx, void *user_data)
// {
// 	static size_t data_received;

// 	switch (status)
// 	{
// 	case HTTP_SERVER_TRANSACTION_ABORTED:
// 	{
// 		LOG_DBG("Transaction aborted after %zd bytes.", data_received);
// 		data_received = 0;
// 		return -1;
// 	}
// 	case HTTP_SERVER_TRANSACTION_COMPLETE:
// 	{
// 		data_received = 0;
// 		break;
// 	}
// 	default:
// 		break;
// 	}

// 	/* 未发送完成，存储数据*/
// 	data_received += request_ctx->data_len;

// 	if (status == HTTP_SERVER_REQUEST_DATA_FINAL)
// 	{
// 		LOG_DBG("bmp picture received (%zd bytes).", processed);
// 		processed = 0;

// 		// 传输并保存成功if ()
// 		char *response_str = "Upload Succ";
// 		/* 填充response */
		
// 		/* 只有当 final_chunk 为1的时候，才会发送response（结构和request相似，一次response只对应一次request，分包只是 TCP 的作用，和http没有关系） */
// 		response_ctx->final_chunk = true;
// 	}
// 	return 0;
// }

// static struct http_resource_detail_dynamic data_up_resource_detail = {
// 	.common = {
// 		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
// 		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
// 	},
// 	.cb = data_up_handler,
// 	.user_data = NULL,
// };

// static int zkk_handler(struct http_client_ctx *client, enum http_transaction_status status,
// 						   const struct http_request_ctx *request_ctx,
// 						   struct http_response_ctx *response_ctx, void *user_data)
{

}

static struct http_resource_detail_dynamic zkk_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
	},
	.cb = zkk_handler,
	.user_data = NULL,
};

static uint16_t http_service_port = CONFIG_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port,
					CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, http_service, "/",
					 &index_html_gz_resource_detail);
HTTP_RESOURCE_DEFINE(script_js_gz_resource, http_service, "/script.js",
					 &script_js_gz_resource_detail);
// HTTP_RESOURCE_DEFINE(data_up_resource, http_service, "/dataUP", &data_up_resource_detail);
// HTTP_RESOURCE_DEFINE(zkk_resource, http_service, "/dataUP", &data_up_resource_detail);



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
