#include <common/buffer.h>

#include <xcodec/xcodec.h>
#include <xcodec/xcodec_cache.h>

std::map<UUID, XCodecCache *> XCodecCache::cache_map;

/*使用最久未使用算法淘汰
 *淘汰时机：当cache中段的个数大于max_count而且超时的段。
 * */
int HashTime::cnt = 0;

void waitsecond(int s)
{
	clock_t c = clock();
	while( (clock()-c)/CLOCKS_PER_SEC < s);
}

/*测试使用*/
void LRU::test_list()
{
	int i;
	HashTime *ht;

	std::cout<<"test push front:"<<std::endl;
	for(i = 0; i < 4; i++)
	{
		ht = new HashTime(i+1, clock());
		waitsecond(1);
		ht_push_front(ht);
	}
	show_list();

	std::cout<<"test push back:"<<std::endl;
	for(i = 10; i < 14; i++)
	{
		ht = new HashTime(i+1, clock());
		waitsecond(1);
		ht_push_back(ht);
	}
	show_list();

	
	std::cout<<"test remove front and remove back:"<<std::endl;
	for(i = 0; i < 2; i++)
	{
		HashTime *hfront = htlist.next;
		HashTime *hback  = htlist.prev;
		ht_remove(hfront);
		ht_remove(hback);
		std::cout<<"\tremove hash:"<<hfront->hash<<"\tTime:"<<hfront->time<<std::endl;
		std::cout<<"\tremove hash:"<<hback ->hash<<"\tTime:"<<hback ->time<<std::endl;
		delete hfront;
		delete hback;
	}
	show_list();
	
	std::cout<<"test remove front and insert to  back:"<<std::endl;
	for(i = 0; i < 2; i++)
	{
		HashTime *hfront = htlist.next;
		ht_remove(hfront);
		ht_push_back(hfront);
	}
	show_list();

	std::cout<<"test remove middle and insert to  back:"<<std::endl;
	
	for(i = 0; i < 2; i++)
	{
		HashTime *hfront = htlist.next->next;
		ht_remove(hfront);
		ht_push_back(hfront);
	}
	show_list();
}

LRU::LRU(int mc, int et)
{
	this->max_count = mc;
	this->expired_time = et;
	htlist.next = htlist.prev = &htlist;
	DEBUG("LRU construct...");
}

LRU::~LRU()
{

	DEBUG("destruct lru:");
	HashTime *ht = htlist.next;
	while(htlist.next != &htlist)
	{
		ht = htlist.next;
		ht_remove(ht);
		delete ht;
	}
	
	cache.clear();
}

int  LRU::cache_size()
{
	return cache.size();
}

/* 插入hash到map cahce,和链表htlist的尾部。*/	
void LRU::insert(uint64_t hash)
{
	ASSERT("lur insert", cache.find(hash) == cache.end());
	HashTime *ht = new HashTime(hash, clock()/CLOCKS_PER_SEC);
	ht_push_back(ht);
	cache[hash] = ht;
}

/* 查询hash值，并且把这个hash 值移动到htlist的尾部。*/
void LRU::lookup(uint64_t hash)
{
	HashTime *ht = cache[hash];
	ASSERT("lru lookup", ht!=NULL);
	ht->time = clock()/CLOCKS_PER_SEC;
	ht_remove(ht);
	ht_push_back(ht);
}

/* 循环淘汰队首的hash */
uint64_t LRU::eliminate()
{
	uint64_t hash;
	if(can_eliminate())
	{
		HashTime *ht = htlist.next;
		hash = ht->hash;
		ht_remove(ht);
	//	std::cout<<"Erase:"<<hash<<"size:"<<cache.size()<<std::endl;
		cache.erase(cache.find(hash));
		delete ht;
		return hash;
	}
	return 0;
}

bool LRU::can_eliminate()
{
	if(htlist.next!=NULL && cache.size() > (unsigned)max_count)
		if(clock() - htlist.next->time > expired_time)
			return true;
	return false;
}

void LRU::ht_push_front(struct HashTime *e)
{
	if(htlist.next == NULL)
	{
		htlist.next = htlist.prev = e;
		e->prev = e->next =&htlist;
	}
	else
	{
		e->next =  htlist.next;
		e->prev = &htlist;
		htlist.next->prev = e;
		htlist.next = e;
	}
}

void LRU::ht_push_back(struct HashTime *e)
{
	
	if(htlist.prev == NULL)
	{
		htlist.next = htlist.prev = e;
		e->prev = e->next =&htlist;
	}
	else
	{
		e->next = &htlist;
		e->prev = htlist.prev;
		htlist.prev->next = e;
		htlist.prev = e;
	}
}
void LRU::ht_remove(struct HashTime *e)
{
	e->prev->next = e->next;
	e->next->prev = e->prev;
}

void LRU::show_list()
{
	HashTime *ht = htlist.next;
	int cnt = 0;
	while(ht!=&htlist)
	{
		std::cout<<"Hash:"<<ht->hash<<"\tTime:"<<ht->time<<std::endl;
		ht = ht->next;
		cnt++;
	}

	std::cout<<"list have:"<<cnt<<"\t should be:"<<HashTime::cnt<<std::endl;
}

