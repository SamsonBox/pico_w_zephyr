#include <inttypes.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/sntp.h>
#include <arpa/inet.h>
#include <zephyr/net/net_if.h>
#include "dns_resolver.h"
 
namespace sntp_client
{
class sntp_client
{
public:
    sntp_client() = default;
    ~sntp_client() = default;
    int do_sntp_request(struct sntp_time& oTime);
private:
    int do_sntp(int family, struct sntp_time& oTime);
    dns_resolver::DnsResolver mResolver;

};
} // namespace sntp_client