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
LOG_MODULE_REGISTER(turtle_http_server, LOG_LEVEL_WRN);
int turtle_web::webserver::service_fd = -1;
turtle_web::WebserverDataInterface* turtle_web::webserver::mDataInterface = nullptr;
struct http_service_runtime_data turtle_web::webserver::service_data = {0};
struct http_resource_detail_static turtle_web::webserver::index_html_gz_resource_detail = {};
struct http_resource_detail_static turtle_web::webserver::main_js_gz_resource_detail = {};
struct http_resource_detail_dynamic turtle_web::webserver::data_resource_detail = {}; 
struct http_resource_detail_dynamic turtle_web::webserver::update_resource_detail = {}; 
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

const struct http_resource_desc  turtle_web::webserver::update_resource = {
    .resource = "/update",
    .detail = (void *)(&update_resource_detail),
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
            http_server_stop();
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

        update_resource_detail.common = {
            .bitmask_of_supported_http_methods = BIT(HTTP_POST),
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		},
	    update_resource_detail.cb = update_handler;
	    update_resource_detail.user_data = NULL;

        mInit = true;
    }

    void webserver::set_temp(struct sensor_value iTemp)
    {
        mCurrentTemp = iTemp;
    }

    void webserver::set_data_interface(WebserverDataInterface *iDataInterface)
    {
        mDataInterface = iDataInterface;
    }

    int webserver::data_handler(http_client_ctx *client, http_data_status status,
			  const http_request_ctx *request_ctx,
			  http_response_ctx *response_ctx, void *user_data)
    {
        static uint8_t uptime_buf[160];

        LOG_DBG("Uptime handler status %d", status);

        /* A payload is not expected with the GET request. Ignore any data and wait until
        * final callback before sending response
        */
        if (status == HTTP_SERVER_DATA_FINAL) 
        {
            
            size_t ret = fill_data_buffer(uptime_buf, sizeof(uptime_buf));
            response_ctx->body = uptime_buf;
            response_ctx->body_len = ret;
            response_ctx->final_chunk = true;
        }

        return 0;
    }

    int webserver::update_handler(http_client_ctx *client, http_data_status status, const http_request_ctx *request_ctx, http_response_ctx *response_ctx, void *user_data)
    {
        static uint8_t uptime_buf[160];
        static size_t cursor;

        LOG_DBG("Uptime handler status %d, size %zu", status, request_ctx->data_len);

        if (status == HTTP_SERVER_DATA_ABORTED) {
            cursor = 0;
            return 0;
        }

        if (request_ctx->data_len + cursor > sizeof(uptime_buf)) {
            cursor = 0;
            return -ENOMEM;
        }

        /* Copy payload to our buffer. Note that even for a small payload, it may arrive split into
        * chunks (e.g. if the header size was such that the whole HTTP request exceeds the size of
        * the client buffer).
        */
        memcpy(uptime_buf + cursor, request_ctx->data, request_ctx->data_len);
        cursor += request_ctx->data_len;

        if (status == HTTP_SERVER_DATA_FINAL) {
            parse_update_post(uptime_buf, cursor);
            cursor = 0;

            size_t ret = fill_data_buffer(uptime_buf, sizeof(uptime_buf));
            response_ctx->body = uptime_buf;
            response_ctx->body_len = ret;
            response_ctx->final_chunk = true;
        }
        return 0;
    }


    size_t webserver::fill_data_buffer(uint8_t *iBuffer, size_t iSize)
    {
        int ret = 0;
        if(mDataInterface)
        {
            struct rtc_time endtime = mDataInterface->get_end_time();
            struct rtc_time starttime = mDataInterface->get_start_time();
            int relay_state = mDataInterface->get_relay_state();
            struct sensor_value switchTemp = mDataInterface->get_switching_temp();
            struct sensor_value curTemp = mDataInterface->get_temp();

            ret = snprintf(reinterpret_cast<char*>(iBuffer), iSize,
            "{\n"
            "  \"end_time\": \"%02d:%02d\",\n"
            "  \"relay\": %s,\n"
            "  \"start_time\": \"%02d:%02d\",\n"
            "  \"switch_temp\": %d.%02d,\n"
            "  \"temp\": %d.%02d,\n"
            "  \"uptime\": %llu\n"
            "}",
            endtime.tm_hour, endtime.tm_min,
            relay_state > 0 ? "true" : "false",
            starttime.tm_hour, starttime.tm_min,
            switchTemp.val1, (switchTemp.val2 / 10) % 100,
            curTemp.val1, (curTemp.val2 / 10) % 100,
            k_uptime_get()
            );
            //snprintf(reinterpret_cast<char*>(&uptime_buf[0]), sizeof(uptime_buf), "{ \"uptime\": %lld, \"temp\": %d.%06d }", k_uptime_get(), mCurrentTemp.val1, mCurrentTemp.val2);
            if (ret < 0) 
            {
                LOG_ERR("Failed to snprintf uptime, err %d", ret);
                return 0;
            }

        }
        else
        {
            ret = snprintf(reinterpret_cast<char*>(iBuffer), iSize, "{ \"uptime\": %lld, \"temp\": %d.%06d }", k_uptime_get(), mCurrentTemp.val1, mCurrentTemp.val2);
            if (ret < 0) {
                LOG_ERR("Failed to snprintf uptime, err %d", ret);
                return 0;
            }
        }
        return static_cast<size_t>(ret);
    }
    
    void webserver::parse_update_post(uint8_t *buf, size_t len)
    {
        if(!mDataInterface)
        {
            return;
        }
        struct rtc_time endtime = mDataInterface->get_end_time();
        struct rtc_time starttime = mDataInterface->get_start_time();
        struct sensor_value switchTemp = mDataInterface->get_switching_temp();
        int relay_state = mDataInterface->get_relay_state();
        buf[len] = 0;
        JsonData data = parseJson(const_cast<const char*>(reinterpret_cast<char*>(buf)));
        
        if(data.relay != (relay_state > 0))
        {
            mDataInterface->set_relay_state(data.relay ? 1 : 0);
        }
        bool doUpdate = false;
        
        if((data.end_hour >= 0) && (endtime.tm_hour != data.end_hour || endtime.tm_min != data.end_min))
        {
            endtime.tm_hour = data.end_hour;
            endtime.tm_min = data.end_min;
            LOG_ERR("End Time changed");
            doUpdate = true;
        }

        if((data.start_hour >= 0) && (starttime.tm_hour != data.start_hour || starttime.tm_min != data.start_min))
        {
            starttime.tm_hour = data.start_hour;
            starttime.tm_min = data.start_min;
            LOG_ERR("Start Time changed");
            doUpdate = true;
        }

        if((data.switch_dec >= 0) && (switchTemp.val1 != data.switch_int || switchTemp.val2 != data.switch_dec))
        {
            switchTemp.val1 = data.switch_int;
            switchTemp.val2 = data.switch_dec;
            LOG_ERR("Switching Temp changed");
            doUpdate = true;
        }

        if(doUpdate)
        {
            LOG_ERR("Do Update");
            mDataInterface->update_settings(starttime, endtime, switchTemp);
        }

    }

    int webserver::toInt(const char* &p) 
    {
        int val = 0;
        bool neg = false;
        if (*p == '-') { neg = true; p++; }
        while (*p >= '0' && *p <= '9') {
            val = val * 10 + (*p - '0');
            p++;
        }
        return neg ? -val : val;
    }

    void webserver::skipSpaces(const char* &p)
    {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    }

    webserver::JsonData webserver::parseJson(const char* json) 
    {
        JsonData data{};
        data.start_hour = -1;
        data.end_hour = -1;
        data.switch_dec = -1;
        const char* p = json;
        LOG_DBG("parseJson %s", json);
        while (*p) 
        {
            skipSpaces(p);
            // Ende erreicht?
            if (*p == 0) break;

            if (*p == '"') 
            {
                p++;
                LOG_DBG("remaining string %s", p);
                // Key auslesen
                const char* key = p;
                while (*p && *p != '"') p++;
                int keyLen = p - key;
                p++; 
                skipSpaces(p);
                if (*p == ':') p++;
                skipSpaces(p);
                // --- Werte auslesen ---
                if (keyLen == 8 && strncmp(key, "end_time", 8) == 0) 
                {
                    if (*p == '"') {
                        p++;
                        data.end_hour = toInt(p);
                        if (*p == ':') p++;
                        data.end_min = toInt(p);
                        if (*p == '"') p++;
                    }
                    LOG_DBG("end_time %02d:%02d", data.end_hour, data.end_min);
                }
                else if (keyLen == 10 && strncmp(key, "start_time", 10) == 0) 
                {
                    if (*p == '"') {
                        p++;
                        data.start_hour = toInt(p);
                        if (*p == ':') p++;
                        data.start_min = toInt(p);
                        if (*p == '"') p++;
                    }
                    LOG_DBG("start_time %02d:%02d", data.start_hour, data.start_min);
                }
                else if (keyLen == 5 && strncmp(key, "relay", 5) == 0) 
                {
                    if (strncmp(p, "true", 4) == 0)
                    { 
                        data.relay = true; p += 4; 
                    }
                    else if (strncmp(p, "false", 5) == 0)
                    { 
                        data.relay = false; p += 5;
                    }
                    LOG_DBG("relay %s", data.relay ? "true" : "false");
                }
                else if (keyLen == 11 && strncmp(key, "switch_temp", 11) == 0)
                {
                    data.switch_int = toInt(p);
                    if (*p == '.') 
                    { 
                        p++; data.switch_dec = toInt(p); 
                    }
                    else
                    {
                        data.switch_dec = 0;
                    }
                    LOG_DBG("switch_temp %03d.%03d", data.switch_int, data.switch_dec);
                }
                else
                {
                    while(*p != '"') p++;
                    p++;
                }
            }
            else if (*p == '}')
            {
                break;
            }
            else
            {
                p++;
            }
        }
        return data;
    }

}
