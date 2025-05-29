#include "turtle_http_server.h"

#include <stdio.h>
#include <inttypes.h>

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include "zephyr/device.h"
#include "zephyr/sys/util.h"
#include <zephyr/data/json.h>
#include <zephyr/sys/util_macro.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(turtle_http_server, LOG_LEVEL_DBG);
int turtle_web::webserver::service_fd = -1;
struct http_service_runtime_data turtle_web::webserver::service_data = {0};
struct http_resource_detail_static turtle_web::webserver::index_html_gz_resource_detail = {};
struct http_resource_detail_static turtle_web::webserver::main_js_gz_resource_detail = {};
struct http_resource_detail_dynamic turtle_web::webserver::data_resource_detail = {}; 
struct sensor_value turtle_web::webserver::mCurrentTemp = {0};

const struct http_service_desc turtle_web::webserver::test_http_service =  
{
    .host = NULL,
    .port = (uint16_t *)(&turtle_web::webserver::service_port),
    .fd = &turtle_web::webserver::service_fd,
    .detail = (void *)(NULL),
    .concurrent = (CONFIG_HTTP_SERVER_MAX_CLIENTS),
    .backlog = (10),
    .data = &turtle_web::webserver::service_data,
    .res_begin = (_http_resource_desc_test_http_service_list_start),
    .res_end = (_http_resource_desc_test_http_service_list_end),
    .res_fallback = (NULL)
};

const struct http_resource_desc turtle_web::webserver::index_html_gz_resource = {
    .resource = "/",
    .detail = (void *)(&turtle_web::webserver::index_html_gz_resource_detail),
};

const struct http_resource_desc turtle_web::webserver::index_html_gz_resource2 = {
    .resource = "/index.html",
    .detail = (void *)(&turtle_web::webserver::index_html_gz_resource_detail),
};


const struct http_resource_desc turtle_web::webserver::main_js_gz_resource = {
    .resource = "/main.js",
    .detail = (void *)(&main_js_gz_resource_detail),
};

const struct http_resource_desc  turtle_web::webserver::data_resource = {
    .resource = "/data",
    .detail = (void *)(&data_resource_detail),
};

namespace turtle_web
{
    webserver::webserver()
    {
    }

    void webserver::start()
    {
        if(!mInit)
        {
            return;
        }
        if(!server_running)
        {
            http_server_start();
            server_running = true;
        }
    }

    void webserver::stop()
    {
        if(server_running)
        {
            http_server_start();
            server_running=false;
        }
    }

    void webserver::init()
    {
        if(mInit)
        {
            return;
        }
        index_html_gz_resource_detail.common = {
            .bitmask_of_supported_http_methods = (uint32_t) BIT(HTTP_GET),
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.content_encoding = "gzip",
			.content_type = "text/html",
		};
        index_html_gz_resource_detail.static_data = index_html_gz;
        index_html_gz_resource_detail.static_data_len = sizeof(index_html_gz);

        main_js_gz_resource_detail.common = {
            .bitmask_of_supported_http_methods = (uint32_t) BIT(HTTP_GET),
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.content_encoding = "gzip",
			.content_type = "text/javascript",
		};
        main_js_gz_resource_detail.static_data = main_js_gz;
        main_js_gz_resource_detail.static_data_len = sizeof(main_js_gz);

	    data_resource_detail.common = {
            .bitmask_of_supported_http_methods = BIT(HTTP_GET),
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		},
	    data_resource_detail.cb = data_handler;
	    data_resource_detail.user_data = NULL;

        mInit = true;
    }

    void webserver::set_temp(struct sensor_value iTemp)
    {
        mCurrentTemp = iTemp;
    }

    int webserver::data_handler(http_client_ctx *client, http_data_status status,
			  const http_request_ctx *request_ctx,
			  http_response_ctx *response_ctx, void *user_data)
    {
        int ret;
        static uint8_t uptime_buf[64 + sizeof(STRINGIFY(INT64_MAX))];

        LOG_DBG("Uptime handler status %d", status);

        /* A payload is not expected with the GET request. Ignore any data and wait until
        * final callback before sending response
        */
        if (status == HTTP_SERVER_DATA_FINAL) {
            ret = snprintf(reinterpret_cast<char*>(&uptime_buf[0]), sizeof(uptime_buf), "{ \"uptime\": %lld, \"temp\": %d.%06d }", k_uptime_get(), mCurrentTemp.val1, mCurrentTemp.val2);
            if (ret < 0) {
                LOG_ERR("Failed to snprintf uptime, err %d", ret);
                return ret;
            }

            response_ctx->body = uptime_buf;
            response_ctx->body_len = ret;
            response_ctx->final_chunk = true;
        }

        return 0;
    }
}