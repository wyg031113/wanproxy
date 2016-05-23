#ifndef __SYNCAP_H__
#define __SYNCAP_H__
#include <map>
typedef unsigned int u_int32;
typedef unsigned short u_int16;

/*IP 端口
 */
struct IpPort
{
	u_int32 IP;
	u_int16 port;	

};

/**连接状态
 */
//enum ConnStatus{Pedding, Connected, Closed};

/**SYN中目标 IP端口
 */
struct Dest
{
	struct IpPort dest;
	long time_stamp;     //维护时间戳
	//ConnStatus state;	
};

/** SYNTable维护
 * 整个进程中维护了一个对象
 * syn_map存放了连接的源IP‘PORT和目标IP PORT
 * 截获SYN有另一个线程完成。
 * 锁syn_lock就是用于线程互斥访问syn_table的。
 *
 */
class SYNTable
{
	public:
		 SYNTable();
		~SYNTable();

		/* 在syn_map中查找，并且删除IP_PORT对。*/
		 Dest SearchDest(const IpPort &src);  

		 /* 插入IP_PORT对 */
		 bool InsertDest(IpPort &src, Dest &dest);

		 /* 检查有无超时的IP_PORT对*/
		 void CheckTimeStamp();
	private:
	std::map<IpPort, Dest> syn_map;
	pthread_mutex_t syn_lock;
};

/* 初始化syn截获线程，定时检测线程。*/
void init_syn_handle();

/* 全局syn表*/
extern SYNTable syn;
#endif /* __SYNCAP_H__ */
