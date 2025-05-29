#include "sntp_client.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sntp_client, LOG_LEVEL_DBG);


namespace sntp_client
{
int sntp_client::do_sntp_request(struct sntp_time& oTime)
{
    return do_sntp(AF_INET, oTime);
}

int sntp_client::do_sntp(int family, struct sntp_time& oTime)
{
	struct sntp_ctx ctx;
	int rv;
    const char *family_str = family == AF_INET ? "IPv4" : "IPv6";

	/* Get SNTP server */
    struct dns_addrinfo addrInfo;
	rv = mResolver.resolve(CONFIG_NET_SAMPLE_SNTP_SERVER_ADDRESS, CONFIG_NET_SAMPLE_SNTP_SERVER_PORT, family, &addrInfo);
	if (rv != 0) {
		LOG_ERR("Failed to lookup %s SNTP server (%d)", family_str, rv);
		return -1;
	}
	rv = sntp_init(&ctx, &addrInfo.ai_addr, addrInfo.ai_addrlen);
	if (rv < 0) {
		LOG_ERR("Failed to init SNTP %s ctx: %d", family_str, rv);
		goto end;
	}

	LOG_DBG("Sending SNTP %s request...", family_str);
    for(int i =0; i < 10; i++)
    {
        rv = sntp_query(&ctx, 4 * MSEC_PER_SEC, &oTime);
        if(rv == 0)
        {
            break;
        }
        k_msleep(200);
    }
    if (rv < 0) 
    {
        LOG_ERR("SNTP %s request failed: %d", family_str, rv);
        goto end;
    }

	LOG_DBG("SNTP Time: %llu", oTime.seconds);

end:
	sntp_close(&ctx);
    return 0;
}
} // namespace sntp_client