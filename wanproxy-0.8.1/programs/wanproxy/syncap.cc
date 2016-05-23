#include <iostream>
#include <map>
#define IPQUEUE_OLD 0
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/user.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netdb.h>
#include <getopt.h>
#include <linux/netfilter.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <arpa/inet.h>

#if (IPQUEUE_OLD)
#include <libipq/libipq.h>
#else
#include <libnetfilter_queue/libnetfilter_queue.h>
#endif
#define QUEUER_BUF_SIZE PAGE_SIZE

/* 检测间隔 */
#define CHECK_INTV 100000

/* 生存时间 */
#define LIFE_TIME 50000

#include "syncap.h"

/* NFQUEUE序号*/
int queuenum = 1;			
struct nfnl_handle *nh;    
int fd;
char buf[QUEUER_BUF_SIZE];

/* IP PORT 比较函数*/
bool operator<(const IpPort &a, const IpPort &b)
{
	if(a.IP == b.IP)
		return a.port < b.port;
	else
		return a.IP < b.IP;
}

SYNTable::SYNTable()
{
	pthread_mutex_init(&syn_lock, NULL);
}

SYNTable::~SYNTable()
{
	 pthread_mutex_destroy(&syn_lock);
}

/* 在SYN表中，依据源IP端口查找目标IP端口。
 * 找到后就删除该项
 * 找不到，则目标IP 端口为全0
 * */
Dest SYNTable::SearchDest(const IpPort &src)
{
	Dest d;
	d.dest.port = d.dest.IP = 0;

	pthread_mutex_lock(&syn_lock);
	std::map<IpPort, Dest>::iterator ite = syn_map.find(src);
	if(ite != syn_map.end())
	{
		d = ite->second;
		syn_map.erase(ite);
		DEBUG("Search and delete")<<"src["<<src.IP<<":"<<src.port<<"] dest["<<d.dest.IP<<":"<<d.dest.port<<"]";
	}
	pthread_mutex_unlock(&syn_lock);
	return d;

}
bool SYNTable::InsertDest(IpPort &src,Dest &dest)
{
	pthread_mutex_lock(&syn_lock);
	dest.time_stamp = time(NULL);
	//dest.state = Pedding;
	syn_map[src] = dest;
	pthread_mutex_unlock(&syn_lock);
	DEBUG("insert")<<"src["<<src.IP<<":"<<src.port<<"] dest["<<dest.dest.IP<<":"<<dest.dest.port<<"]";
	return true;
}

void SYNTable::CheckTimeStamp()
{
	pthread_mutex_lock(&syn_lock);
	
	std::map<IpPort, Dest>::iterator ite;
	for(ite = syn_map.begin(); ite != syn_map.end();)
	{
		//if(ite->second.state != Connected)
		if(time(NULL) - ite->second.time_stamp > LIFE_TIME)
		{
			syn_map.erase(ite++);
			DEBUG("GC erase")<<"src["<<ite->first.IP<<":"<<ite->first.port<<"] dest["<<ite->second.dest.IP<<":"<<ite->second.dest.port<<"]";
		}
		else
			ite++;	
		
	}
	pthread_mutex_unlock(&syn_lock);

}


SYNTable syn;


//回调函数
int nfqueue_get_syn(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                           struct nfq_data *nfa, void *data)
{
	struct nfqnl_msg_packet_hdr *ph;
	unsigned char *buffer;

	ph = nfq_get_msg_packet_hdr(nfa);

	int id = ntohl(ph->packet_id);
	int ret = nfq_get_payload(nfa, &buffer);
	struct in_addr srcx, dstx;
	struct iphdr *iph = ((struct iphdr *) buffer);
	struct tcphdr *tcp = ((struct tcphdr *) (buffer + (iph->ihl << 2)));

	srcx.s_addr = iph->saddr;
	dstx.s_addr = iph->daddr;

	// because compile unused erro
	nfmsg = nfmsg;
	data =data;

	/* 超出源IP端口和目标IP端口 */
	IpPort src;
	Dest dst;
	src.IP = ntohl(iph->saddr);
	src.port = ntohs(tcp->source);
	
	dst.dest.IP = ntohl(iph->daddr);
	dst.dest.port = ntohs(tcp->dest);
	
	syn.InsertDest(src, dst);
	
	DEBUG("capture")<<"src["<<inet_ntoa(srcx)<<":"<< ntohs(tcp->source)<<"] dest["<<inet_ntoa(dstx)<<":"<<ntohs(tcp->dest)<<"]";
	ret = nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	return 0;
}

/* time check thread */
void *syn_collector(void *args)
{
	args = NULL;

	pthread_detach(pthread_self());
	while(1)
	{
		DEBUG("SYN collector running...");
		syn.CheckTimeStamp();
		sleep(CHECK_INTV);
	}
	return NULL;
}

/* syn add thread */
void * syn_add(void *args)
{
	args = NULL;
 
	struct nfq_handle *h;
	struct nfq_q_handle *qh;

	int ret;
	pthread_detach(pthread_self());

	h = nfq_open();
	if(h == NULL)
		printf("nfq_open failed\n");

	ret = nfq_unbind_pf(h, AF_INET);
	if(ret < 0)
		printf("nfq_unbind_pf failed\n");

	ret = nfq_bind_pf(h, AF_INET);
	if(ret < 0)
		printf("nfq_bind_pf failed\n");

	qh = nfq_create_queue(h, queuenum, &nfqueue_get_syn, NULL);
	if(!qh)
		printf("create queue failed!\n");
	
	ret = nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xFFFF);
	if(ret < 0)
		printf("failed to set mode\n");

	nh = nfq_nfnlh(h);
      fd = nfnl_fd(nh);
	int rc;
      while ((rc = recv(fd, buf, QUEUER_BUF_SIZE, 0)) >= 0)
        nfq_handle_packet(h, buf, rc);

    	nfq_close(h);	
	return NULL;
}

void init_syn_handle()
{
	int ret;
	pthread_t tid;
	
	/* SYN 截获线程 */
	ret = pthread_create(&tid, NULL, syn_add, NULL);
	if (ret)
	{
		printf("create thread syn add failed\n");
		exit(-1);
	}

	/* SYN 回收线程*/
	ret = pthread_create(&tid, NULL, syn_collector, NULL);
	if (ret)
	{
		printf("create thread syn collector failed\n");
		exit(-1);
	}
}
/*
int main()
{
	std::cout<<"Hello world"<<std::endl;
	init_syn_handle();
	sleep(10000);
	return 0;
}
*/
