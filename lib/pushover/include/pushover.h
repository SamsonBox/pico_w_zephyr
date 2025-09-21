#include <string>
#include <zephyr/net/socket.h>
#include <zephyr/net/http/client.h>

namespace PushOver
{
class PushOver
{
public:
PushOver(const char* iToken, const char* iUser);
bool send_message(const char* iMessage);
static int gloabal_response_cb(http_response *rsp, http_final_call final_data, void* userData);
private:
int open_socket();
int send_http_post(const char* iMessage);
void close_socket();
int response_cb(http_response *rsp, http_final_call final_data);
const char* mServer = {"api.pushover.net"};
const char* mServerIp = "10.76.67.95";
static constexpr int mPort = 8080;
std::string mToken;
std::string mUser;
struct sockaddr_in mAddr;
struct http_request mReq;
int mSocket = -1;
uint8_t mReceveBuffer[512];
};
} // namespace PushOver