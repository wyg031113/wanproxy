#include <common/endian.h>
#include <event/event_callback.h>
#include <event/event_system.h>
#include <io/socket/socket.h>
#include <io/net/tcp_server.h>
#include <iostream>
#include <arpa/inet.h>
#include <stdio.h>

#include "syncap.h"
#include "proxy_connector.h"
#include "proxy_listener.h"

#include "wanproxy_codec_pipe_pair.h"

ProxyListener::ProxyListener(const std::string& name,
			     WANProxyCodec *interface_codec,
			     WANProxyCodec *remote_codec,
			     SocketAddressFamily interface_family,
			     const std::string& interface,
			     SocketAddressFamily remote_family,
			     const std::string& remote_name)
: SimpleServer<TCPServer>("/tcp_pepd/proxy/" + name + "/listener", interface_family, interface),
  name_(name),
  interface_codec_(interface_codec),
  remote_codec_(remote_codec),
  remote_family_(remote_family),
  remote_name_(remote_name)
{ }

ProxyListener::~ProxyListener()
{ }
void
ProxyListener::client_connected(Socket *socket)
{
	//std::cout<<socket->dest<<"  remote_name: "<<remote_name_<<std::endl;
	PipePair *pipe_pair = new WANProxyCodecPipePair(interface_codec_, remote_codec_);
	struct IpPort src;
	src.IP = ntohl(socket->src_addr.sin_addr.s_addr);
	src.port = ntohs(socket->src_addr.sin_port);
	struct Dest dest = syn.SearchDest(src);
	struct sockaddr_in remote_addr;
	remote_addr.sin_addr.s_addr = htonl(dest.dest.IP);
	char addr[32];
	snprintf(addr, 32, "[%s]:%u", inet_ntoa(remote_addr.sin_addr), dest.dest.port);
	//std::cout<<"remote addr"<<std::string(addr)<<std::endl;
	new ProxyConnector(name_, pipe_pair, socket, remote_family_, std::string(addr)/*remote_name_*/);
}
