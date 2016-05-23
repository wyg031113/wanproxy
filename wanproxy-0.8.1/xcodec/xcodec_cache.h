#ifndef	XCODEC_XCODEC_CACHE_H
#define	XCODEC_XCODEC_CACHE_H

#include <ext/hash_map>
#include <map>

#include <common/uuid/uuid.h>
#include <iostream>
#include <time.h>
#include <stdint.h>
#include <cstdlib>
#include <stdio.h>
#include "xcodec_window.h"
//#define ASSERT(p) {if(!(p)) abort();}


#include <common/uuid/uuid.h>


/* 对每个hash维护时间戳*/
struct HashTime
{
	HashTime(){cnt++;/*std::cout<<"construct a HT\n";*/}
	HashTime(uint64_t h, unsigned long t):hash(h), time(t){cnt++;/* std::cout<<"construct a HT\n";*/}
	~HashTime(){cnt--;/*std::cout<<"destroy a HT\n";*/}
	uint64_t hash;
	unsigned long time;
	struct HashTime *next, *prev;
	static int cnt;
};


/* 淘汰最久未使用的hash和对应的段*/
class LRU
{
public:	
	LRU(int max_count, int expired_time);
	~LRU();
	void insert(uint64_t hash); 				/* 插入hash到map cahce,和链表htlist的尾部。*/			
	void lookup(uint64_t hash);					/* 查询hash值，并且把这个hash 值移动到htlist的尾部。*/
	uint64_t eliminate();						/* 循环淘汰队首的hash */
	bool can_eliminate();
	void show_list();
	void test_list();
	int  cache_size();
private:
	/*双向循环链表操作*/
	void ht_push_front(struct HashTime *e);
	void ht_push_back(struct HashTime *e);
	void ht_remove(struct HashTime *e);
	
private:
	int max_count;								/* 缓存中允许的最多的段数 */
	unsigned long expired_time;					/* 可以淘汰的最短时间 */
	std::map<uint64_t, HashTime*> cache;
	HashTime htlist;
};


/*
 * XXX
 * GCC supports hash<unsigned long> but not hash<unsigned long long>.  On some
 * of our platforms, the former is 64-bits, on some the latter.  As a result,
 * we need this wrapper structure to throw our hashes into so that GCC's hash
 * function can be reliably defined to use them.
 */
struct Hash64 {
	uint64_t hash_;

	Hash64(const uint64_t& hash)
	: hash_(hash)
	{ }

	bool operator== (const Hash64& hash) const
	{
		return (hash_ == hash.hash_);
	}

	bool operator< (const Hash64& hash) const
	{
		return (hash_ < hash.hash_);
	}
};

namespace __gnu_cxx {
	template<>
	struct hash<Hash64> {
		size_t operator() (const Hash64& x) const
		{
			return (x.hash_);
		}
	};
}

class XCodecCache {
protected:
	

	XCodecCache(const UUID& uuid)
	: uuid_(uuid)
	{ }

public:
	UUID uuid_;
	virtual ~XCodecCache()
	{ }

	virtual void enter(const uint64_t&, BufferSegment *) = 0;
	virtual BufferSegment *lookup(const uint64_t&) const = 0;
	virtual bool out_of_band(void) const = 0;
	virtual void eliminate(XCodecWindow *window) = 0;

	bool uuid_encode(Buffer *buf) const
	{
		return (uuid_.encode(buf));
	}

	static void enter(const UUID& uuid, XCodecCache *cache)
	{
		ASSERT("/xcodec/cache", cache_map.find(uuid) == cache_map.end());
		cache_map[uuid] = cache;
	}

	static XCodecCache *lookup(const UUID& uuid)
	{
		std::map<UUID, XCodecCache *>::const_iterator it;

		it = cache_map.find(uuid);
		if (it == cache_map.end())
			return (NULL);

		return (it->second);
	}

private:
	static std::map<UUID, XCodecCache *> cache_map;
};

class XCodecMemoryCache : public XCodecCache {
	typedef __gnu_cxx::hash_map<Hash64, BufferSegment *> segment_hash_map_t;

	LogHandle log_;
	segment_hash_map_t segment_hash_map_;
	LRU lru;
public:
	XCodecMemoryCache(const UUID& uuid)
	: XCodecCache(uuid),
	  log_("/xcodec/cache/memory"),
	  segment_hash_map_(),
	  lru(1024*250, 10)
	{ DEBUG("Create a Memory cache...\n");}

	~XCodecMemoryCache()
	{
		segment_hash_map_t::const_iterator it;
		for (it = segment_hash_map_.begin();
		     it != segment_hash_map_.end(); ++it)
			it->second->unref();
		segment_hash_map_.clear();
	DEBUG("MemoryCache destroyed\n");
	}

	void enter(const uint64_t& hash, BufferSegment *seg)
	{
		ASSERT(log_, seg->length() == XCODEC_SEGMENT_LENGTH);
		ASSERT(log_, segment_hash_map_.find(hash) == segment_hash_map_.end());
		seg->ref();
		segment_hash_map_[hash] = seg;
		lru.insert(hash);
		//printf("lru cache:%d\n",lru.cache.size());
	}

	void eliminate(XCodecWindow *window)
	{
		
		while(lru.can_eliminate())
		{
			uint64_t hash = lru.eliminate();
			if(window != NULL)
				window->undeclare(hash);
			//printf("ref=%d\n", it->second->ref_);

			segment_hash_map_t::iterator it;
			it = segment_hash_map_.find(hash);
			//std::cout<<"ref_="<<it->second->ref_<<std::endl;
			it->second->unref();
		//	while(it->second->ref_>0)
		//		it->second->unref();
			
			segment_hash_map_.erase(it);
               // printf("in while: lru cache:%d\n",lru.cache_size());
		
		//printf("in while: segment_hash_mat size:%d\n", segment_hash_map_.size());
		}	
               // printf("out while: lru cache:%d\n",lru.cache_size());

		//printf("out while: segment_hash_mat size:%d\n", segment_hash_map_.size());
	}

	bool out_of_band(void) const
	{
		/*
		 * Memory caches are not exchanged out-of-band; references
		 * must be extracted in-stream.
		 */
		return (false);
	}

	BufferSegment *lookup(const uint64_t& hash) const
	{
		segment_hash_map_t::const_iterator it;
		it = segment_hash_map_.find(hash);
		if (it == segment_hash_map_.end())
			return (NULL);

		BufferSegment *seg;

		seg = it->second;
		seg->ref();
		return (seg);
	}
};

#endif /* !XCODEC_XCODEC_CACHE_H */
