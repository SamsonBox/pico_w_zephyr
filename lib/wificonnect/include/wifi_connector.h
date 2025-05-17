#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include "status_led.h"


namespace wificonnector
{
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
        void set_state(ConnectionState iConnectionState);
    private:
        
        ConnectionState mState = ConnectionState::IDLE;
        static void net_mgmt_event_handler(net_mgmt_event_callback *cb,
					 uint32_t mgmt_event, net_if *iface);
        static constexpr uint32_t MGMT_EVENTS = (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
        static void handle_wifi_connect_result(net_mgmt_event_callback *cb);
        static void handle_wifi_disconnect_result(net_mgmt_event_callback *cb);
        WifiAutoConnect();
        static WifiAutoConnect* mInstance;
        struct net_mgmt_event_callback mWifiMgmtCb;
        static constexpr char* SSID = "HITRON-9090";
        static constexpr char* PW = "8KI530DQXELS";
        StatusLed::StatusLed mLed;

};

WifiAutoConnect::WifiAutoConnect() 
{
    net_mgmt_init_event_callback(&mWifiMgmtCb,
				     net_mgmt_event_handler,
				     MGMT_EVENTS);

	net_mgmt_add_event_callback(&mWifiMgmtCb);
    mLed.start();
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

	if (status->status) {
		printk("Connection request failed (%d)\n", status->status);
        instance->set_state(ConnectionState::IDLE);
        instance->connect();
	} else {
        instance->set_state(ConnectionState::CONNECTED);
		printk("Connected to %s\n", SSID);
	}
}

void WifiAutoConnect::handle_wifi_disconnect_result(net_mgmt_event_callback *cb)
{
    const struct wifi_status *status =
		(const struct wifi_status *) cb->info;

	printk("Disconnected\n");
    WifiAutoConnect* instance = get_instance();
    instance->set_state(ConnectionState::IDLE);
    instance->connect();

}

void WifiAutoConnect::net_mgmt_event_handler(net_mgmt_event_callback *cb,
				    uint32_t mgmt_event, net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}
}

void WifiAutoConnect::connect()
{
   if(mState == ConnectionState::IDLE)
   {
    printk("Establish connection to : %s\n", SSID);
    mState = ConnectionState::CONNECTING;
    mLed.set_state(StatusLed::StatusLed::State::CONNECTING);
    struct net_if *iface = net_if_get_first_wifi();
    struct wifi_connect_req_params cnx_params = { 0 }; 
    cnx_params.band = WIFI_FREQ_BAND_UNKNOWN;
    cnx_params.channel = WIFI_CHANNEL_ANY;
    cnx_params.ssid = reinterpret_cast<const uint8_t*>(SSID);
    cnx_params.ssid_length = strlen(reinterpret_cast<const char*>(cnx_params.ssid));
    cnx_params.security = WIFI_SECURITY_TYPE_PSK;
    cnx_params.psk = reinterpret_cast<const uint8_t*>(PW);
    cnx_params.psk_length = strlen(reinterpret_cast<const char*>(cnx_params.psk));

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                &cnx_params, sizeof(struct wifi_connect_req_params));

        if (ret)
        {
            printk("Connection request failed with error: %d\n", ret);
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

void WifiAutoConnect::set_state(WifiAutoConnect::ConnectionState iConnectionState)
{
    mState = iConnectionState;
    switch (mState)
    {
    case WifiAutoConnect::ConnectionState::IDLE:
        mLed.set_state(StatusLed::StatusLed::State::NONE);
        break;
    case WifiAutoConnect::ConnectionState::CONNECTING:
        mLed.set_state(StatusLed::StatusLed::State::CONNECTING);
        break;
    case WifiAutoConnect::ConnectionState::CONNECTED:
        mLed.set_state(StatusLed::StatusLed::State::CONNECTED);
        break;
    default:
        mLed.set_state(StatusLed::StatusLed::State::NONE);
        break;
    }
}
}
