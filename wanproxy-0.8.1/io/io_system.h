#ifndef	IO_IO_SYSTEM_H
#define	IO_IO_SYSTEM_H

#include <map>

#include <io/io_system.h>

class Channel;

class IOSystem {
	struct Handle {
		LogHandle log_;

		int fd_;
		Channel *owner_;

		SimpleCallback *close_callback_;
		Action *close_action_;

		off_t read_offset_;
		size_t read_amount_;
		Buffer read_buffer_;
		EventCallback *read_callback_;
		Action *read_action_;

		off_t write_offset_;
		Buffer write_buffer_;
		EventCallback *write_callback_;
		Action *write_action_;

		Handle(int, Channel *);
		~Handle();

		void close_callback(void);
		void close_cancel(void);
		Action *close_schedule(void);

		void read_callback(Event);
		void read_cancel(void);
		Action *read_do(void);
		Action *read_schedule(void);

		void write_callback(Event);
		void write_cancel(void);
		Action *write_do(void);
		Action *write_schedule(void);
	};

	/*
	 * The map is keyed by fd and channel to prevent collision
	 * on the fd with a common race on close.  This is also why
	 * we pass the fd and channel to all functions.  There is
	 * no interaction, it's just a disambiguator.
	 */
	typedef std::pair<int, Channel *> handle_key_t;
	typedef std::map<handle_key_t, Handle *> handle_map_t;

	LogHandle log_;
	handle_map_t handle_map_;

	IOSystem(void);
	~IOSystem();

public:
	void attach(int, Channel *);
	void detach(int, Channel *);

	Action *close(int, Channel *, SimpleCallback *);
	Action *read(int, Channel *, off_t, size_t, EventCallback *);
	Action *write(int, Channel *, off_t, Buffer *, EventCallback *);

	static IOSystem *instance(void)
	{
		static IOSystem *instance_;

		if (instance_ == NULL)
			instance_ = new IOSystem();
		return (instance_);
	}
};

#endif /* !IO_IO_SYSTEM_H */
