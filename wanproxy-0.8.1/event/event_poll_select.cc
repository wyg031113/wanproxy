#include <sys/errno.h>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_poll.h>

EventPoll::EventPoll(void)
: log_("/event/poll"),
  read_poll_(),
  write_poll_(),
  state_(NULL)
{
}

EventPoll::~EventPoll()
{
	ASSERT(log_, read_poll_.empty());
	ASSERT(log_, write_poll_.empty());
}

Action *
EventPoll::poll(const Type& type, int fd, EventCallback *cb)
{
	ASSERT(log_, fd != -1);

	EventPoll::PollHandler *poll_handler;
	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) == read_poll_.end());
		poll_handler = &read_poll_[fd];
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) == write_poll_.end());
		poll_handler = &write_poll_[fd];
		break;
	default:
		NOTREACHED(log_);
	}
	ASSERT(log_, poll_handler->action_ == NULL);
	poll_handler->callback_ = cb;
	Action *a = new EventPoll::PollAction(this, type, fd);
	return (a);
}

void
EventPoll::cancel(const Type& type, int fd)
{
	EventPoll::PollHandler *poll_handler;

	switch (type) {
	case EventPoll::Readable:
		ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
		poll_handler = &read_poll_[fd];
		poll_handler->cancel();
		read_poll_.erase(fd);
		break;
	case EventPoll::Writable:
		ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
		poll_handler = &write_poll_[fd];
		poll_handler->cancel();
		write_poll_.erase(fd);
		break;
	}
}

void
EventPoll::wait(int ms)
{
	poll_handler_map_t::iterator it;
	fd_set read_set, write_set;
	int maxfd;

	/*
	 * XXX
	 * Track minfd, too.
	 */
	maxfd = -1;

	FD_ZERO(&read_set);
	for (it = read_poll_.begin(); it != read_poll_.end(); ++it) {
		FD_SET(it->first, &read_set);
		if (maxfd < it->first)
			maxfd = it->first;
	}

	FD_ZERO(&write_set);
	for (it = write_poll_.begin(); it != write_poll_.end(); ++it) {
		FD_SET(it->first, &write_set);
		if (maxfd < it->first)
			maxfd = it->first;
	}

	/*
	 * XXX Why wait to call idle() until here?
	 */
	if (idle()) {
		if (ms != -1) {
			int rv;

			rv = usleep(ms * 1000);
			ASSERT(log_, rv != -1);
		}
		return;
	}
	ASSERT(log_, maxfd != -1);

	struct timeval tv, *tvp;

	if (ms == -1) {
		tvp = NULL;
	} else {
		tv.tv_sec = ms / 1000;
		tv.tv_usec = ms % 1000;
		tvp = &tv;
	}

	int fdcnt = ::select(maxfd + 1, &read_set, &write_set, NULL, tvp);
	if (fdcnt == -1) {
		if (errno == EINTR) {
			INFO(log_) << "Received interrupt, ceasing polling until stop handlers have run.";
			return;
		}
		HALT(log_) << "Could not select.";
	}

	int fd;
	for (fd = 0; fdcnt != 0 && fd <= maxfd; fd++) {
		if (FD_ISSET(fd, &read_set)) {
			ASSERT(log_, read_poll_.find(fd) != read_poll_.end());
			read_poll_[fd].callback(Event::Done);

			ASSERT(log_, fdcnt != 0);
			fdcnt--;
		}

		if (FD_ISSET(fd, &write_set)) {
			ASSERT(log_, write_poll_.find(fd) != write_poll_.end());
			write_poll_[fd].callback(Event::Done);

			ASSERT(log_, fdcnt != 0);
			fdcnt--;
		}
	}
}
