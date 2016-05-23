#ifndef	IO_PIPE_SPLICE_H
#define	IO_PIPE_SPLICE_H

class StreamChannel;
class Pipe;

class Splice {
	LogHandle log_;
	StreamChannel *source_;
	Pipe *pipe_;
	StreamChannel *sink_;

	EventCallback *callback_;
	Action *callback_action_;

	bool read_eos_;
	Action *read_action_;
	Action *input_action_;
	bool output_eos_;
	Action *output_action_;
	Action *write_action_;
	Action *shutdown_action_;

public:
	Splice(const LogHandle&, StreamChannel *, Pipe *, StreamChannel *);
	~Splice();

	Action *start(EventCallback *);

private:
	void cancel(void);
	void complete(Event);

	void read_complete(Event);
	void input_complete(Event);

	void output_complete(Event);
	void write_complete(Event);

	void shutdown_complete(Event);
public:
	static Action *create(Splice **, StreamChannel *, Pipe *, StreamChannel *);
};

#endif /* !IO_PIPE_SPLICE_H */
