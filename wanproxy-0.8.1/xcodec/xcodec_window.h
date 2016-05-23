#ifndef	XCODEC_XCODEC_WINDOW_H
#define	XCODEC_XCODEC_WINDOW_H

#include <map>

#define	XCODEC_WINDOW_MAX		(0xff)
#define	XCODEC_WINDOW_COUNT		(XCODEC_WINDOW_MAX + 1)

/*
 * XXX
 * Make more like an LRU and make present() bump up in the window.
 *
 * Maybe add an explicit use() mechanism?
 */
class XCodecWindow {
	uint64_t window_[XCODEC_WINDOW_COUNT];
	unsigned cursor_;
	std::map<uint64_t, unsigned> present_;
	std::map<uint64_t, BufferSegment *> segments_;
public:
	XCodecWindow(void)
	: window_(),
	  cursor_(0),
	  present_(),
	  segments_()
	{
		unsigned b;

		for (b = 0; b < XCODEC_WINDOW_COUNT; b++) {
			window_[b] = 0;
		}
	}

	~XCodecWindow()
	{
		std::map<uint64_t, BufferSegment *>::iterator it;

		for (it = segments_.begin(); it != segments_.end(); ++it)
			it->second->unref();
		segments_.clear();
	}

	void declare(uint64_t hash, BufferSegment *seg)
	{
		if (hash == 0)
			return;

		if (present_.find(hash) != present_.end())
			return;

		uint64_t old = window_[cursor_];
		if (old != 0) {
			ASSERT("/xcodec/window", present_[old] == cursor_);
			present_.erase(old);

			std::map<uint64_t, BufferSegment *>::iterator it;
			it = segments_.find(old);
			ASSERT("/xcodec/window", it != segments_.end());
			BufferSegment *oseg = it->second;
			oseg->unref();
			segments_.erase(it);
		}

		window_[cursor_] = hash;
		present_[hash] = cursor_;
		seg->ref();
		segments_[hash] = seg;
		cursor_ = (cursor_ + 1) % XCODEC_WINDOW_COUNT;
	}

	void undeclare(const uint64_t hash)
	{
		if(hash == 0)
			return;
		std::map<uint64_t, unsigned>::iterator it;
		it = present_.find(hash);
		if (it == present_.end())
			return;
		int cur = it->second;
		window_[cur] = 0;
		present_.erase(it);

		std::map<uint64_t, BufferSegment *>::iterator ite;
		ite = segments_.find(hash);
		ASSERT("/xcodec/window", ite != segments_.end());
		BufferSegment *oseg = ite->second;
			ite->second->unref();
		oseg->unref();
		segments_.erase(ite);

	}


	BufferSegment *dereference(unsigned c) const
	{
		if (window_[c] == 0)
			return (NULL);
		std::map<uint64_t, BufferSegment *>::const_iterator it;
		it = segments_.find(window_[c]);
		ASSERT("/xcodec/window", it != segments_.end());
		BufferSegment *seg = it->second;
		seg->ref();
		return (seg);
	}

	bool present(uint64_t hash, uint8_t *c) const
	{
		std::map<uint64_t, unsigned>::const_iterator it = present_.find(hash);
		if (it == present_.end())
			return (false);
		ASSERT("/xcodec/window", window_[it->second] == hash);
		*c = it->second;
		return (true);
	}
};

#endif /* !XCODEC_XCODEC_WINDOW_H */
