#pragma once
#include <inttypes.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/sensor.h>

extern struct http_resource_desc _http_resource_desc_test_http_service_list_start[];
extern struct http_resource_desc _http_resource_desc_test_http_service_list_end[];
namespace turtle_web
{
class WebserverDataInterface
{
public:
    // getter
    virtual struct sensor_value get_temp() = 0;
    virtual struct sensor_value get_switching_temp() = 0;
    virtual struct rtc_time get_start_time() = 0;
    virtual struct rtc_time get_end_time() = 0;
    virtual int get_relay_state() = 0;

    // setter
    virtual void set_switching_temp(struct sensor_value& iSwitchingTemp) = 0;
    virtual void set_start_time(struct rtc_time& iStartTime) = 0;
    virtual void set_end_time(struct rtc_time& iEndTime) = 0;
    virtual void update_settings(struct rtc_time& iStartTime, struct rtc_time& iEndTime, struct sensor_value& iSwitchingTemp) = 0;
    virtual void set_relay_state(int iRelayState) = 0;
};

class webserver
{
public:
    webserver();
    ~webserver() = default;
    void start();
    void stop();
    void init();
public:
    static constexpr uint8_t index_html_gz[] = {
        #include "index.html.gz.inc"
    };

    static constexpr uint8_t main_js_gz[] = {
        #include "main.js.gz.inc"
    };
    static constexpr uint16_t service_port = CONFIG_NET_SAMPLE_HTTP_SERVER_SERVICE_PORT;
    static int service_fd;
    void set_temp(struct sensor_value iTemp);
    void set_data_interface(WebserverDataInterface* iDataInterface);
private:
    struct JsonData {
        int start_hour;
        int start_min;
        int end_hour;
        int end_min;
        bool relay;
        int switch_int;
        int switch_dec;
    };
    static int data_handler(http_client_ctx *client, http_data_status status,
			  const http_request_ctx *request_ctx,
			  http_response_ctx *response_ctx, void *user_data);
    static int update_handler(http_client_ctx *client, http_data_status status,
			  const http_request_ctx *request_ctx,
			  http_response_ctx *response_ctx, void *user_data);
    static size_t fill_data_buffer(uint8_t* iBuffer, size_t iSize);
    static void parse_update_post(uint8_t *buf, size_t len);
    static JsonData parseJson(const char* json);
    static int toInt(const char* &p);
    static void skipSpaces(const char* &p);
    static struct http_service_runtime_data service_data;
    static struct http_resource_detail_static index_html_gz_resource_detail;
    static struct http_resource_detail_static main_js_gz_resource_detail;
    static struct http_resource_detail_dynamic data_resource_detail;
    static struct http_resource_detail_dynamic update_resource_detail;
    
    const static Z_DECL_ALIGN(struct http_service_desc) test_http_service __in_section(_http_service_desc, static, _CONCAT(test_http_service, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) index_html_gz_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(index_html_gz_resource, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) index_html_gz_resource2 __in_section(_http_resource_desc_test_http_service, static, _CONCAT(index_html_gz_resource2, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) main_js_gz_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(main_js_gz_resource, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) data_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(uptime_resource, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) update_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(update_resource, _)) __used __noasan;
    bool server_running = false;
    bool mInit = false;
    static struct sensor_value mCurrentTemp;
    static WebserverDataInterface* mDataInterface;
};

}