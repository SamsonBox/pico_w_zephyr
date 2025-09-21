#include "dns_resolver.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(my_dns_resolver, LOG_LEVEL_DBG);

namespace dns_resolver
{
DnsResolver::DnsResolver()
{
    k_sem_init (&mSemaphor, 0, 1);
}

void DnsResolver::dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	const char *hr_family;
	void *addr;
    DnsResolver* pInstance = reinterpret_cast<DnsResolver*>(user_data);

	switch (status) {
	case DNS_EAI_CANCELED:
		LOG_INF("DNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		LOG_INF("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		LOG_INF("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		LOG_INF("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		LOG_INF("DNS resolving error (%d)", status);
		return;
	}

	if (!info || !pInstance) {
		return;
	}

    pInstance->set_address_info(*info);

	if (info->ai_family == AF_INET) {
		hr_family = "IPv4";
		addr = &net_sin(&info->ai_addr)->sin_addr;
	} else if (info->ai_family == AF_INET6) {
		hr_family = "IPv6";
		addr = &net_sin6(&info->ai_addr)->sin6_addr;
	} else {
		LOG_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}

	LOG_INF("%s %s address: %s", info->ai_canonname,
		hr_family,
		net_addr_ntop(info->ai_family, addr,
					 hr_addr, sizeof(hr_addr)));
}

void DnsResolver::set_address_info(struct dns_addrinfo iAdrInfo)
{
    if(!mAddressResolved)
    {
        if(mAdrInfo)
        {
            *mAdrInfo = iAdrInfo;
			mAddressResolved = true;
			set_finish();
        }
    }
    
}

void DnsResolver::set_finish()
{
	/* Notify test thread */
	k_sem_give(&mSemaphor);
}

int DnsResolver::resolve(const char *host, int family, struct dns_addrinfo* oResolveInfo)
{
    if(!oResolveInfo)
    {
        return -1;
    }
    mAdrInfo = oResolveInfo;
    return dns_query(host, family, SOCK_DGRAM);
}

int DnsResolver::dns_query(const char *host, int family, int socktype)
{
	int rv;
    static uint16_t dns_id;
	/* Perform DNS query */
    mAddressResolved = false;
    k_sem_reset(&mSemaphor);
	rv = dns_get_addr_info(host,
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				this,
				2 * MSEC_PER_SEC);
	if (rv < 0) {
		LOG_ERR("getaddrinfo failed (%d, errno %d)", rv, errno);
		return rv;
	}

    rv = k_sem_take(&mSemaphor, K_MSEC(CONFIG_NET_SAMPLE_SNTP_SERVER_TIMEOUT_MS));
	if (rv < 0) {
		LOG_INF("dns_get_addr_info response timed out (%d)", rv);
        return rv;
	}
	return 0;
}

} //dns_resolver