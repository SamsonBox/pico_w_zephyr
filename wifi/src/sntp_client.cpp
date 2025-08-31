#include "sntp_client.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sntp_client, LOG_LEVEL_ERR);


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
    if(!addr_resolved)
    {
	    rv = mResolver.resolve(CONFIG_NET_SAMPLE_SNTP_SERVER_ADDRESS, family, &addrInfo);
        if (rv != 0) {
            LOG_ERR("Failed to lookup %s SNTP server (%d)", family_str, rv);
            return -1;
        }
        addr_resolved = true;
    }
    struct sockaddr addr;
    socklen_t addr_len;
    addr = addrInfo.ai_addr;
    addr_len =  addrInfo.ai_addrlen;
    char addr_str[INET6_ADDRSTRLEN] = {0};
    /* Store the port */
	net_sin(&addr)->sin_port = htons(CONFIG_NET_SAMPLE_SNTP_SERVER_PORT);
	/* Print the found address */
    const void *src = &(net_sin(&addr)->sin_addr);
	zsock_inet_ntop(addr.sa_family, src , addr_str, sizeof(addr_str));
	LOG_INF("%s -> %s", CONFIG_NET_SAMPLE_SNTP_SERVER_ADDRESS, addr_str);
	rv = sntp_init(&ctx, &addr, addr_len);
	if (rv < 0) {
		LOG_ERR("Failed to init SNTP %s ctx: %d", family_str, rv);
		goto end;
	}

	LOG_DBG("Sending SNTP %s request...", family_str);
    
    rv = sntp_query(&ctx, 4 * MSEC_PER_SEC, &oTime);
    if (rv < 0) 
    {
        LOG_ERR("SNTP %s request failed: %d", family_str, rv);
        goto end;
    }

	LOG_DBG("SNTP Time: %llu", oTime.seconds);

end:
	sntp_close(&ctx);
    return rv;
}
} // namespace sntp_client