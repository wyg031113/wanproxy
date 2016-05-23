#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H

#include <config/config_type_pointer.h>

#include "wanproxy_config_type_proxy_type.h"

class ProxyListener;

class WANProxyConfigClassProxy : public ConfigClass {
	std::map<ConfigObject *, ProxyListener *> object_listener_map_;
public:
	WANProxyConfigClassProxy(void)
	: ConfigClass("proxy"),
	  object_listener_map_()
	{
		add_member("type", &wanproxy_config_type_proxy_type);
		add_member("interface", &config_type_pointer);
		add_member("interface_codec", &config_type_pointer);
		add_member("peer", &config_type_pointer);
		add_member("peer_codec", &config_type_pointer);
		add_member("uuid", &config_type_pointer);
	}

	/* XXX So wrong.  */
	~WANProxyConfigClassProxy()
	{ }

	bool activate(ConfigObject *);
};

extern WANProxyConfigClassProxy wanproxy_config_class_proxy;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_PROXY_H */
