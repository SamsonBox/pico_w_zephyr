#include <inttypes.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/drivers/sensor.h>

extern struct http_resource_desc _http_resource_desc_test_http_service_list_start[];
extern struct http_resource_desc _http_resource_desc_test_http_service_list_end[];
namespace turtle_web
{
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
private:
    static int data_handler(http_client_ctx *client, http_data_status status,
			  const http_request_ctx *request_ctx,
			  http_response_ctx *response_ctx, void *user_data);
    static struct http_service_runtime_data service_data;
    static struct http_resource_detail_static index_html_gz_resource_detail;
    static struct http_resource_detail_static main_js_gz_resource_detail;
    static struct http_resource_detail_dynamic data_resource_detail;
    
    const static Z_DECL_ALIGN(struct http_service_desc) test_http_service __in_section(_http_service_desc, static, _CONCAT(test_http_service, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) index_html_gz_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(index_html_gz_resource, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) index_html_gz_resource2 __in_section(_http_resource_desc_test_http_service, static, _CONCAT(index_html_gz_resource2, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) main_js_gz_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(main_js_gz_resource, _)) __used __noasan;
    const static Z_DECL_ALIGN(struct http_resource_desc) data_resource __in_section(_http_resource_desc_test_http_service, static, _CONCAT(uptime_resource, _)) __used __noasan;
    bool server_running = false;
    bool mInit = false;
    static struct sensor_value mCurrentTemp;
};

}