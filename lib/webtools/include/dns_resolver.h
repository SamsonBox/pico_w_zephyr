#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>

namespace dns_resolver
{
class DnsResolver
{
public:
    DnsResolver();
    ~DnsResolver() = default;
    int resolve(const char *host, int family, struct dns_addrinfo* oResolveInfo);
private:
    void set_finish();
    void set_address_info(struct dns_addrinfo mAdrInfo);
    static void dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data);
    int dns_query(const char *host, int family, int socktype);
    struct k_sem mSemaphor;
    struct dns_addrinfo* mAdrInfo = nullptr;
    bool mAddressResolved = false;
};
} //dns_resolver