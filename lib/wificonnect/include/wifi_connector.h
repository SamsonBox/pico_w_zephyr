#pragma once
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include "status_led.h"

LOG_MODULE_REGISTER(wificonnector, LOG_LEVEL_DBG);
#define MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"
namespace wificonnector
{
class IConnecttionCallback
{
public:
    enum class State
    {
        DISCONNECTED,
        CONNECTED,
    };
    virtual void connection_st_state_changed(IConnecttionCallback::State iNewState) = 0;
    virtual void connection_ap_state_changed(IConnecttionCallback::State iNewState) = 0;
};
class WifiAutoConnect
{
    public:
        static WifiAutoConnect* get_instance();
        void connect();
        enum class ConnectionState
        {
            IDLE,
            CONNECTING,
            CONNECTED,
        };
        ConnectionState get_state();
        void set_st_state(ConnectionState iConnectionState);
        void set_ap_state(ConnectionState iConnectionState);
        void set_callbackifc(IConnecttionCallback* iCallback);
    private:
        int enable_ap_mode(void);
        void enable_dhcpv4_server(void);
        void disable_dhcpv4_server(void);
        ConnectionState mState = ConnectionState::IDLE;
        static void net_mgmt_event_handler(net_mgmt_event_callback *cb,
					 long long unsigned int mgmt_event, net_if *iface);
        static constexpr long long unsigned int MGMT_EVENTS = (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT |
	                                             NET_EVENT_WIFI_AP_ENABLE_RESULT | NET_EVENT_WIFI_AP_DISABLE_RESULT |
	                                             NET_EVENT_WIFI_AP_STA_CONNECTED | NET_EVENT_WIFI_AP_STA_DISCONNECTED );
        static void handle_wifi_connect_result(net_mgmt_event_callback *cb);
        static void handle_wifi_disconnect_result(net_mgmt_event_callback *cb);
        WifiAutoConnect();
        static WifiAutoConnect* mInstance;
        struct net_mgmt_event_callback mWifiMgmtCb;
        static constexpr const char* SSID = {"HITRON-9090"};
        static constexpr const char* PW = {"8KI530DQXELS"};
        static constexpr const char* WIFI_AP_SSID = {"KROETE-AP"};
        static constexpr const char* WIFI_AP_PSK = {"012345678912"};
        static constexpr const char* WIFI_AP_IP_ADDRESS = {"192.168.4.1"};
        static constexpr const char* WIFI_AP_NETMASK = {"255.255.255.0"};
        net_if * mApIfc = nullptr;
        net_if * mStaIfc = nullptr;
        StatusLed::StatusLed mLed;
        IConnecttionCallback* mCallback = nullptr;

};

WifiAutoConnect::WifiAutoConnect() 
{
    net_mgmt_init_event_callback(&mWifiMgmtCb,
				     net_mgmt_event_handler,
				     MGMT_EVENTS);

	net_mgmt_add_event_callback(&mWifiMgmtCb);
    mLed.start();
	/* Get STA interface in AP-STA mode. */
	mStaIfc = net_if_get_wifi_sta();
    mApIfc = net_if_get_wifi_sap();
    enable_ap_mode();
};

WifiAutoConnect* WifiAutoConnect::get_instance()
{
    if(!mInstance)
    {
        mInstance = new WifiAutoConnect();
    }
    return mInstance;
}

void WifiAutoConnect::handle_wifi_connect_result(net_mgmt_event_callback *cb)
{
    const struct wifi_status *status =
		(const struct wifi_status *) cb->info;
    WifiAutoConnect* instance = get_instance();

	if (status->status)
    {
		LOG_INF("Connection request failed (%d)\n", status->status);
        instance->set_st_state(ConnectionState::IDLE);
        instance->connect();
	} else {
        instance->set_st_state(ConnectionState::CONNECTED);
		LOG_INF("Connected to %s\n", SSID);
	}
}

void WifiAutoConnect::handle_wifi_disconnect_result(net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *) cb->info;
    if (status->status)
    {
	    LOG_INF("Disconnected Failed\n");
    }
    else
    {
        LOG_INF("Disconnected\n");
    }
    WifiAutoConnect* instance = get_instance();
    instance->set_st_state(ConnectionState::IDLE);
    instance->connect();
}

void WifiAutoConnect::net_mgmt_event_handler(net_mgmt_event_callback *cb,
				    long long unsigned int mgmt_event, net_if *iface)
{
	switch (mgmt_event) 
    {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
    case NET_EVENT_WIFI_AP_ENABLE_RESULT: 
    {
		LOG_INF("AP Mode is enabled. Waiting for station to connect");
		break;
	}
	case NET_EVENT_WIFI_AP_DISABLE_RESULT: 
    {
		LOG_INF("AP Mode is disabled.");
		break;
	}
	case NET_EVENT_WIFI_AP_STA_CONNECTED: 
    {
		struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

		LOG_INF("station: " MACSTR " joined ", sta_info->mac[0], sta_info->mac[1],
			sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
        WifiAutoConnect* instance = get_instance();
        instance->set_ap_state(ConnectionState::CONNECTED);
        instance->enable_dhcpv4_server();
		break;
	}
	case NET_EVENT_WIFI_AP_STA_DISCONNECTED: 
    {
		struct wifi_ap_sta_info *sta_info = (struct wifi_ap_sta_info *)cb->info;

		LOG_INF("station: " MACSTR " leave ", sta_info->mac[0], sta_info->mac[1],
			sta_info->mac[2], sta_info->mac[3], sta_info->mac[4], sta_info->mac[5]);
        WifiAutoConnect* instance = get_instance();
        instance->set_ap_state(ConnectionState::IDLE);
        instance->disable_dhcpv4_server();
		break;
	}
	default:
		break;
	}
}

void WifiAutoConnect::connect()
{
   if(mState == ConnectionState::IDLE && mStaIfc != nullptr)
   {
        mState = ConnectionState::CONNECTING;
        mLed.set_state(StatusLed::StatusLed::State::CONNECTING);
        struct wifi_connect_req_params cnx_params = { 0 };
        LOG_INF("waiting for disconnected state");
        struct wifi_iface_status status = { 0 };
        status.state = static_cast<int>(WIFI_STATE_SCANNING);
        while(status.state != static_cast<int>(WIFI_STATE_INACTIVE) && status.state != static_cast<int>(WIFI_STATE_DISCONNECTED))
        {
            k_msleep(200);
            if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, mStaIfc, &status,
                    sizeof(struct wifi_iface_status))) {
                LOG_INF("Status request failed\n");
            }

            LOG_DBG("Status: successful");
            LOG_DBG("==================");
            LOG_DBG("State: %s", wifi_state_txt(static_cast<wifi_iface_state>(status.state)));
            
        }

        LOG_INF("Establish connection to : %s", SSID);
        cnx_params.band = WIFI_FREQ_BAND_UNKNOWN;
        cnx_params.channel = WIFI_CHANNEL_ANY;
        cnx_params.ssid = reinterpret_cast<const uint8_t*>(SSID);
        cnx_params.ssid_length = strlen(reinterpret_cast<const char*>(cnx_params.ssid));
        cnx_params.security = WIFI_SECURITY_TYPE_PSK;
        cnx_params.psk = reinterpret_cast<const uint8_t*>(PW);
        cnx_params.psk_length = strlen(reinterpret_cast<const char*>(cnx_params.psk));

        int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, mStaIfc,
                &cnx_params, sizeof(struct wifi_connect_req_params));

        if (ret)
        {
            LOG_INF("Connection request failed with error: %d", ret);
            mState = ConnectionState::IDLE;
            mLed.set_state(StatusLed::StatusLed::State::NONE);
            return;
        }
    }   

}

WifiAutoConnect::ConnectionState WifiAutoConnect::get_state()
{
    return mState;
}

void WifiAutoConnect::set_callbackifc(IConnecttionCallback* iCallback)
{
    mCallback = iCallback;
}

void WifiAutoConnect::set_ap_state(ConnectionState iConnectionState)
{
    switch (iConnectionState)
    {
    case WifiAutoConnect::ConnectionState::CONNECTED:
    {
        mCallback->connection_ap_state_changed(IConnecttionCallback::State::CONNECTED);
        break;
    }
    default:
    {
        mCallback->connection_ap_state_changed(IConnecttionCallback::State::DISCONNECTED);
        break;
    }
    }
}

void WifiAutoConnect::set_st_state(WifiAutoConnect::ConnectionState iConnectionState)
{
    mState = iConnectionState;
    switch (mState)
    {
    case WifiAutoConnect::ConnectionState::IDLE:
        mLed.set_state(StatusLed::StatusLed::State::NONE);
        if(mCallback)
        {
            mCallback->connection_st_state_changed(IConnecttionCallback::State::DISCONNECTED);
        }
        break;
    case WifiAutoConnect::ConnectionState::CONNECTING:
        mLed.set_state(StatusLed::StatusLed::State::CONNECTING);
        if(mCallback)
        {
            mCallback->connection_st_state_changed(IConnecttionCallback::State::DISCONNECTED);
        }
        break;
    case WifiAutoConnect::ConnectionState::CONNECTED:
        mLed.set_state(StatusLed::StatusLed::State::CONNECTED);
        if(mCallback)
        {
            mCallback->connection_st_state_changed(IConnecttionCallback::State::CONNECTED);
        }
        break;
    default:
        mLed.set_state(StatusLed::StatusLed::State::NONE);
        if(mCallback)
        {
            mCallback->connection_st_state_changed(IConnecttionCallback::State::DISCONNECTED);
        }
        break;
    }
}

void WifiAutoConnect::disable_dhcpv4_server(void)
{
    net_dhcpv4_server_stop(mApIfc);
    static struct in_addr addr;
	static struct in_addr netmaskAddr;

	if (net_addr_pton(AF_INET, WIFI_AP_IP_ADDRESS, &addr)) {
		LOG_ERR("Invalid address: %s", WIFI_AP_IP_ADDRESS);
		return;
	}

	if (net_addr_pton(AF_INET, WIFI_AP_NETMASK, &netmaskAddr)) {
		LOG_ERR("Invalid netmask: %s", WIFI_AP_NETMASK);
		return;
	
    }

    if (!net_if_ipv4_addr_rm(mApIfc, &addr))
    {
            LOG_ERR("unable to rm IP address for AP interface");
    }
}

void WifiAutoConnect::enable_dhcpv4_server(void)
{
	static struct in_addr addr;
	static struct in_addr netmaskAddr;

	if (net_addr_pton(AF_INET, WIFI_AP_IP_ADDRESS, &addr)) {
		LOG_ERR("Invalid address: %s", WIFI_AP_IP_ADDRESS);
		return;
	}

	if (net_addr_pton(AF_INET, WIFI_AP_NETMASK, &netmaskAddr)) {
		LOG_ERR("Invalid netmask: %s", WIFI_AP_NETMASK);
		return;
	}

	net_if_ipv4_set_gw(mApIfc, &addr);

	if (net_if_ipv4_addr_add(mApIfc, &addr, NET_ADDR_MANUAL, 0) == NULL) {
		LOG_ERR("unable to set IP address for AP interface");
	}

	if (!net_if_ipv4_set_netmask_by_addr(mApIfc, &addr, &netmaskAddr)) {
		LOG_ERR("Unable to set netmask for AP interface: %s", WIFI_AP_NETMASK);
	}
 
    addr.s4_addr[3] += 10; /* Starting IPv4 address for DHCPv4 address pool. */
    
	if (net_dhcpv4_server_start(mApIfc, &addr) != 0) {
		LOG_ERR("DHCP server is not started for desired IP");
		return;
	} 
    

	LOG_INF("DHCPv4 server started...");
}

int WifiAutoConnect::enable_ap_mode(void)
{
	if (!mApIfc) {
		LOG_INF("AP: is not initialized");
		return -EIO;
	}

	LOG_INF("Turning on AP Mode");
    struct wifi_connect_req_params ap_config = {0};
	ap_config.ssid = (const uint8_t *)WIFI_AP_SSID;
	ap_config.ssid_length = strlen(WIFI_AP_SSID);
	ap_config.psk = (const uint8_t *)WIFI_AP_PSK;
	ap_config.psk_length = strlen(WIFI_AP_PSK);
	ap_config.channel = WIFI_CHANNEL_ANY;
	ap_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	if (strlen(WIFI_AP_PSK) == 0) {
		ap_config.security = WIFI_SECURITY_TYPE_NONE;
	} else {

		ap_config.security = WIFI_SECURITY_TYPE_PSK;
	}

	//enable_dhcpv4_server();

	int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, mApIfc, &ap_config,
			   sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("NET_REQUEST_WIFI_AP_ENABLE failed, err: %d", ret);
	}

	return ret;
}
}
