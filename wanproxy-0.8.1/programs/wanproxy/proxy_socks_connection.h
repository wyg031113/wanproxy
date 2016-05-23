#ifndef	PROGRAMS_WANPROXY_PROXY_SOCKS_CONNECTION_H
#define	PROGRAMS_WANPROXY_PROXY_SOCKS_CONNECTION_H

class ProxySocksConnection {
	enum State {
		GetSOCKSVersion,

		GetSOCKS4Command,
		GetSOCKS4Port,
		GetSOCKS4Address,
		GetSOCKS4User,

		GetSOCKS5AuthLength,
		GetSOCKS5Auth,
		GetSOCKS5Command,
		GetSOCKS5Reserved,
		GetSOCKS5AddressType,
		GetSOCKS5Address,
		GetSOCKS5NameLength,
		GetSOCKS5Name,
		GetSOCKS5Port,
	};

	LogHandle log_;
	std::string name_;
	Socket *client_;
	Action *action_;
	State state_;
	uint16_t network_port_;
	uint32_t network_address_;
	bool socks5_authenticated_;
	std::string socks5_remote_name_;

public:
	ProxySocksConnection(const std::string&, Socket *);
private:
	~ProxySocksConnection();

private:
	void read_complete(Event);
	void write_complete(Event);
	void close_complete(void);

	void schedule_read(size_t);
	void schedule_write(void);
	void schedule_close(void);
};

#endif /* !PROGRAMS_WANPROXY_PROXY_SOCKS_CONNECTION_H */
