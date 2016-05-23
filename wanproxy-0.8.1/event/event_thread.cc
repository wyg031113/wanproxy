#include <signal.h>
#include <unistd.h>

#include <event/event_callback.h>
#include <event/event_thread.h>

namespace {
	static void signal_reload(int);
}

EventThread::EventThread(void)
: Thread("EventThread"),
  log_("/event/thread"),
  queue_(),
  reload_(),
  interest_queue_(),
  timeout_queue_(),
  poll_()
{
	INFO(log_) << "Starting event thread.";

	signal(SIGHUP, signal_reload);
}

void
EventThread::main(void)
{
	for (;;) {
		/*
		 * If we have been told to stop, fire all shutdown events.
		 */
		if (stop_ && !interest_queue_[EventInterestStop].empty()) {
			INFO(log_) << "Running stop handlers.";
			if (interest_queue_[EventInterestStop].drain())
				ERROR(log_) << "Stop handlers added other stop handlers.";
			INFO(log_) << "Stop handlers have been run.";
		}

		/*
		 * If we have been told to reload, fire all shutdown events.
		 */
		if (reload_ && !interest_queue_[EventInterestReload].empty()) {
			INFO(log_) << "Running reload handlers.";
			interest_queue_[EventInterestReload].drain();
			INFO(log_) << "Reload handlers have been run.";

			reload_ = false;
			signal(SIGHUP, signal_reload);
		}

		/*
		 * If there are time-triggered events whose time has come,
		 * run them.
		 */
		if (!timeout_queue_.empty()) {
			while (timeout_queue_.ready())
				timeout_queue_.perform();
		}

		/*
		 * And then run a pending callback.
		 */
		//int cnt = 0;
		while (!queue_.empty() && !timeout_queue_.ready()) {
			queue_.perform();
			if (stop_ || reload_)
				break;
		//	if(cnt++ > 100)
		//		break;
		}
			/*
			 * Quickly poll in between callbacks in case there are
			 * a lot of callbacks or an infinite number of
			 * callbacks, so that we can still handle I/O.
			 */
			if (!poll_.idle())
				poll_.poll();

		/*
		 * If there are any pending callbacks, go back to the start
		 * of the loop.
		 */
		if (!queue_.empty() || timeout_queue_.ready())
			continue;

		/*
		 * But if there are no pending callbacks, and no timers or
		 * file descriptors being polled, stop.
		 */
		if (timeout_queue_.empty() && poll_.idle())
			break;
		/*
		 * If there are timers ticking down, then let the poll module
		 * block until a timer will be ready.
		 * XXX Maybe block 1 second less, just in case?  We'll never
		 * be a realtime thread though, so any attempt at pretending
		 * to be one, beyond giving time events priority at the top of
		 * this loop, is probably a mistake.
		 */
		if (!timeout_queue_.empty()) {
			poll_.wait(timeout_queue_.interval());
			continue;
		}
		/*
		 * If, however, there's nothing sensitive to time, but someone
		 * has a file descriptor that can be blocked on, just block on it
		 * indefinitely.
		 */
		poll_.wait();
	}
}

void
EventThread::reload(void)
{
	signal(SIGHUP, SIG_IGN);
	INFO(log_) << "Running reload events.";
	reload_ = true;
}

namespace {
	static void
	signal_reload(int)
	{
		EventThread::self()->reload();
		/* XXX Forward signal to all threads.  */
	}
}
