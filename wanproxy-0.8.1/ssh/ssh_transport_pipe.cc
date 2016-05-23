#include <common/endian.h>

#include <event/event_callback.h>

#include <http/http_protocol.h>

#include <io/pipe/pipe.h>
#include <io/pipe/pipe_producer.h>

#include <ssh/ssh_algorithm_negotiation.h>
#include <ssh/ssh_protocol.h>
#include <ssh/ssh_transport_pipe.h>

namespace {
	static uint8_t zero_padding[255];
}

SSH::TransportPipe::TransportPipe(AlgorithmNegotiation *algorithm_negotiation)
: PipeProducer("/ssh/transport/pipe"),
  state_(GetIdentificationString),
  block_size_(8),
  mac_length_(0),
  algorithm_negotiation_(algorithm_negotiation),
  receive_callback_(NULL),
  receive_action_(NULL)
{
	Buffer identification_string("SSH-2.0-WANProxy " + (std::string)log_ + "\r\n");
	produce(&identification_string);
}

SSH::TransportPipe::~TransportPipe()
{
	ASSERT(log_, receive_callback_ == NULL);
	ASSERT(log_, receive_action_ == NULL);
}

Action *
SSH::TransportPipe::receive(EventCallback *cb)
{
	ASSERT(log_, receive_callback_ == NULL);
	ASSERT(log_, receive_action_ == NULL);

	/*
	 * XXX
	 * This pattern is implemented about three different
	 * ways in the WANProxy codebase, and they are all kind
	 * of awful.  Need a better way to handle code that
	 * needs executed either on the request of the caller
	 * or when data comes in that satisfies a deferred
	 * callback to a caller.
	 *
	 * Would an EventDropbox suffice?  Or something like?
	 */
	receive_callback_ = cb;
	receive_do();

	if (receive_callback_ != NULL)
		return (cancellation(this, &SSH::TransportPipe::receive_cancel));

	ASSERT(log_, receive_action_ != NULL);
	Action *a = receive_action_;
	receive_action_ = NULL;
	return (a);
}

/*
 * Because this is primarily for WAN optimization we always use minimal
 * padding and zero padding.  Quick and dirty.  Perhaps revisit later,
 * although it makes send() asynchronous unless we add a blocking
 * RNG interface.
 */
void
SSH::TransportPipe::send(Buffer *payload)
{
	Buffer packet;
	uint8_t padding_len;
	uint32_t packet_len;

	ASSERT(log_, state_ == GetPacket);

	packet_len = sizeof padding_len + payload->length();
	padding_len = 4 + (block_size_ - ((sizeof packet_len + packet_len + 4) % block_size_));
	packet_len += padding_len;

	packet_len = BigEndian::encode(packet_len);
	packet.append(&packet_len);
	packet.append(padding_len);
	payload->moveout(&packet);
	packet.append(zero_padding, padding_len);

	if (mac_length_ != 0)
		NOTREACHED(log_);

	produce(&packet);
}

void
SSH::TransportPipe::consume(Buffer *in)
{
	/* XXX XXX XXX */
	if (in->empty()) {
		if (!input_buffer_.empty())
			DEBUG(log_) << "Received EOS with data outstanding.";
		produce_eos();
		return;
	}

	in->moveout(&input_buffer_);

	if (state_ == GetIdentificationString) {
		HTTPProtocol::ParseStatus status;

		while (!input_buffer_.empty()) {
			Buffer line;
			status = HTTPProtocol::ExtractLine(&line, &input_buffer_);
			switch (status) {
			case HTTPProtocol::ParseSuccess:
				break;
			case HTTPProtocol::ParseFailure:
				ERROR(log_) << "Invalid line while waiting for identification string.";
				produce_error();
				return;
			case HTTPProtocol::ParseIncomplete:
				/* Wait for more.  */
				return;
			}

			if (!line.prefix("SSH-"))
				continue; /* Next line.  */

			if (!line.prefix("SSH-2.0")) {
				ERROR(log_) << "Unsupported version.";
				produce_error();
				return;
			}

			state_ = GetPacket;
			/*
			 * XXX
			 * Should have a callback here?
			 */
			if (algorithm_negotiation_ != NULL) {
				Buffer packet;
				if (algorithm_negotiation_->output(&packet))
					send(&packet);
			}
			break;
		}
	}

	if (state_ == GetPacket && receive_callback_ != NULL)
		receive_do();
}

void
SSH::TransportPipe::receive_cancel(void)
{
	if (receive_action_ != NULL) {
		receive_action_->cancel();
		receive_action_ = NULL;
	}

	if (receive_callback_ != NULL) {
		delete receive_callback_;
		receive_callback_ = NULL;
	}
}

void
SSH::TransportPipe::receive_do(void)
{
	ASSERT(log_, receive_action_ == NULL);
	ASSERT(log_, receive_callback_ != NULL);

	if (state_ != GetPacket)
		return;

	while (!input_buffer_.empty()) {
		Buffer packet;
		Buffer mac;
		uint32_t packet_len;
		uint8_t padding_len;
		uint8_t msg;

		if (input_buffer_.length() <= sizeof packet_len) {
			DEBUG(log_) << "Waiting for packet length.";
			return;
		}

		input_buffer_.extract(&packet_len);
		packet_len = BigEndian::decode(packet_len);

		if (packet_len == 0) {
			ERROR(log_) << "Need to handle 0-length packet.";
			produce_error();
			return;
		}

		if (input_buffer_.length() < sizeof packet_len + packet_len + mac_length_) {
			DEBUG(log_) << "Need " << sizeof packet_len + packet_len + mac_length_ << "bytes; have " << input_buffer_.length() << ".";
			return;
		}

		input_buffer_.moveout(&packet, sizeof packet_len, packet_len);
		if (mac_length_ != 0)
			input_buffer_.moveout(&mac, 0, mac_length_);

		padding_len = packet.pop();
		if (padding_len != 0) {
			if (packet.length() < padding_len) {
				ERROR(log_) << "Padding too large for packet.";
				produce_error();
				return;
			}
			packet.trim(padding_len);
		}

		if (packet.empty()) {
			ERROR(log_) << "Need to handle empty packet.";
			produce_error();
			return;
		}

		/*
		 * Pass by range to registered handlers for each range.
		 * Unhandled messages go to the receive_callback_, and
		 * the caller can register key exchange mechanisms,
		 * and handle (or discard) whatever they don't handle.
		 *
		 * NB: The caller could do all this, but it's assumed
		 *     that they usually have better things to do.  If
		 *     they register no handlers, they can certainly do
		 *     so by hand.
		 */
		msg = packet.peek();
		if (msg >= SSH::Message::TransportRangeBegin &&
		    msg <= SSH::Message::TransportRangeEnd) {
			DEBUG(log_) << "Using default handler for transport message.";
		} else if (msg >= SSH::Message::AlgorithmNegotiationRangeBegin &&
			   msg <= SSH::Message::AlgorithmNegotiationRangeEnd) {
			if (algorithm_negotiation_ != NULL) {
				if (algorithm_negotiation_->input(&packet))
					continue;
			}
			DEBUG(log_) << "Using default handler for algorithm negotiation message.";
		} else if (msg >= SSH::Message::KeyExchangeMethodRangeBegin &&
			   msg <= SSH::Message::KeyExchangeMethodRangeEnd) {
			DEBUG(log_) << "Using default handler for key exchange method message.";
		} else if (msg >= SSH::Message::UserAuthenticationGenericRangeBegin &&
			   msg <= SSH::Message::UserAuthenticationGenericRangeEnd) {
			DEBUG(log_) << "Using default handler for generic user authentication message.";
		} else if (msg >= SSH::Message::UserAuthenticationMethodRangeBegin &&
			   msg <= SSH::Message::UserAuthenticationMethodRangeEnd) {
			DEBUG(log_) << "Using default handler for user authentication method message.";
		} else if (msg >= SSH::Message::ConnectionProtocolGlobalRangeBegin &&
			   msg <= SSH::Message::ConnectionProtocolGlobalRangeEnd) {
			DEBUG(log_) << "Using default handler for generic connection protocol message.";
		} else if (msg >= SSH::Message::ConnectionChannelRangeBegin &&
			   msg <= SSH::Message::ConnectionChannelRangeEnd) {
			DEBUG(log_) << "Using default handler for connection channel message.";
		} else if (msg >= SSH::Message::ClientProtocolReservedRangeBegin &&
			   msg <= SSH::Message::ClientProtocolReservedRangeEnd) {
			DEBUG(log_) << "Using default handler for client protocol message.";
		} else if (msg >= SSH::Message::LocalExtensionRangeBegin) {
			/* Because msg is a uint8_t, it will always be <= SSH::Message::LocalExtensionRangeEnd.  */
			DEBUG(log_) << "Using default handler for local extension message.";
		} else {
			ASSERT(log_, msg == 0);
			ERROR(log_) << "Message outside of protocol range received.  Passing to default handler, but not expecting much.";
		}

		receive_callback_->param(Event(Event::Done, packet));
		receive_action_ = receive_callback_->schedule();
		receive_callback_ = NULL;

		return;
	}
}
