#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/net/net_config.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(http_server, LOG_LEVEL_DBG);

static const uint8_t index_html_gz[] = {
#include "index.html.gz.inc"
};

static const uint8_t script_js_gz[] = {
#include "script.js.gz.inc"
};

static const uint8_t zkk_png[] = {
#include "zkk.png.inc"
};

static size_t data_received;
static uint8_t *recv_buf;
K_SEM_DEFINE(sem_http_data_uping, 0, 1);	/* starts off "available" */

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

static struct http_resource_detail_static zkk_png_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_STATIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		.content_type = "image/png",
	},
	.static_data = zkk_png,
	.static_data_len = sizeof(zkk_png),
};

static int data_up_handler(struct http_client_ctx *client, enum http_transaction_status status,
						   const struct http_request_ctx *request_ctx,
						   struct http_response_ctx *response_ctx, void *user_data)
{
	switch (status)
	{
	case HTTP_SERVER_TRANSACTION_ABORTED:
	{
		LOG_DBG("Transaction aborted after %zd bytes.", data_received);
		data_received = 0;
		return -1;
	}
	case HTTP_SERVER_TRANSACTION_COMPLETE: /* 响应也发送完成 */
	{
		LOG_DBG("Transmission completed, including response");
		data_received = 0;
		return 0;
	}
	default:
		break;
	}

	/* 未发送完成，存储数据*/
	if (recv_buf)
	{
		memcpy(recv_buf + data_received, request_ctx->data, request_ctx->data_len);
	}
	data_received += request_ctx->data_len;

	/* 每隔8192字节发送一次日志（太快的话log会丢包） */
	if (data_received / 8192 > (data_received - request_ctx->data_len) / 8192)
	{
		LOG_DBG("data has been received (%zd bytes)", data_received);
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL)
	{
		LOG_DBG("picture received succ (%zd bytes).", data_received);
		data_received = 0;

		// 传输并保存成功if ()
		static char *response_str = "Upload Succ";
		/* 填充response，status 和 header只会在第一次回调的时候被填充 */
		response_ctx->status = HTTP_200_OK;
		response_ctx->body = (const uint8_t *)response_str;
		response_ctx->body_len = sizeof(response_str) - 1;
		/* 只有当 final_chunk 为1的时候，才会发送response（结构和request相似，一次response只对应一次request，分包只是 TCP 的作用，和http没有关系） */
		response_ctx->final_chunk = true;

		k_sem_give(&sem_http_data_uping);
	}
	return 0;
}

static struct http_resource_detail_dynamic data_up_resource_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods = BIT(HTTP_POST),
	},
	.cb = data_up_handler,
	.user_data = NULL,
};

static uint16_t http_service_port = CONFIG_HTTP_SERVER_SERVICE_PORT;
HTTP_SERVICE_DEFINE(http_service, NULL, &http_service_port,
					CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL, NULL);

HTTP_RESOURCE_DEFINE(index_html_gz_resource, http_service, "/",
					 &index_html_gz_resource_detail);
HTTP_RESOURCE_DEFINE(script_js_gz_resource, http_service, "/script.js",
					 &script_js_gz_resource_detail);
HTTP_RESOURCE_DEFINE(zkk_png_resource, http_service, "/zkk.png", &zkk_png_resource_detail);
HTTP_RESOURCE_DEFINE(data_up_resource, http_service, "/dataUP", &data_up_resource_detail);

void http_set_revc_buf(uint8_t *bmp_buf)
{
	recv_buf = bmp_buf;
}