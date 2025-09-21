#include "pushover.h"
#include <zephyr/logging/log.h>


LOG_MODULE_REGISTER(PushOver, LOG_LEVEL_DBG);

namespace PushOver
{
PushOver::PushOver(const char *iToken, const char *iUser)
: mToken(iToken)
, mUser(iUser)
{
}

int PushOver::open_socket()
{
    sockaddr* addr = reinterpret_cast<sockaddr*>(&mAddr);
    memset(&mAddr, 0, sizeof(mAddr));
    net_sin(addr)->sin_family = AF_INET;
    net_sin(addr)->sin_port = htons(mPort);
    inet_pton(AF_INET, mServerIp, &net_sin(addr)->sin_addr);
    mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mSocket < 0) {
		LOG_DBG("Failed to create HTTP socket (%d)", -errno);
        return -1;
	}
    int ret = connect(mSocket, addr, sizeof(mAddr));
	if (ret < 0) {
		LOG_DBG("Cannot connect to remote (%d)", -errno);
		close(mSocket);
		mSocket = -1;
		ret = -errno;
	}

	return ret; 
}

int PushOver::response_cb(http_response *rsp, http_final_call final_data)
{
	if (final_data == HTTP_DATA_MORE) {
		LOG_DBG("Partial data received (%zd bytes)", rsp->data_len);
	} else if (final_data == HTTP_DATA_FINAL) {
		LOG_DBG("All the data received (%zd bytes)", rsp->data_len);
	}
	LOG_DBG("Response status %s", rsp->http_status);

	return 0;
}

int PushOver::send_http_post(const char* iMessage)
{
    int32_t timeout = -1;
	int ret = 0;
    static const char *headers[] = {
			"Transfer-Encoding: chunked\r\n",
            "Content-Type: application/json\r\n",
			NULL
		};
    memset(&mReq, 0, sizeof(mReq));
    mReq.method = HTTP_POST;
    mReq.url = "/echo";
    mReq.host = mServerIp;
    mReq.header_fields = headers;
    mReq.response = gloabal_response_cb;
    mReq.protocol = "HTTP/1.1";
    mReq.recv_buf = mReceveBuffer;
    mReq.recv_buf_len = sizeof(mReceveBuffer);
    mReq.payload = "HALLO";
    mReq.payload_len = strlen(mReq.payload);
    ret = http_client_req(mSocket, &mReq, timeout, this);
    if (ret < 0) {
        LOG_DBG("Client error %d", ret);
    }
    return ret;
}

void PushOver::close_socket()
{
    close(mSocket);
	mSocket = -1;
}

bool PushOver::send_message(const char *iMessage)
{
    if(!iMessage)
    {
        return false;
    }
    int ret = open_socket();
    if(ret != 0)
    {
        LOG_ERR("open_socket failed (%d)", ret);
        return ret;
    }
    ret = send_http_post(iMessage);
    if(ret != 0)
    {
        LOG_ERR("send_http_post failed (%d)", ret);
    }
    close_socket();
    return ret;
}
int PushOver::gloabal_response_cb(http_response *rsp, http_final_call final_data, void *userData)
{
    PushOver* pInstance = reinterpret_cast<PushOver*>(userData);
    return pInstance->response_cb(rsp, final_data);
}
} // namespace PushOver