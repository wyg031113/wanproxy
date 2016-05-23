#ifndef	CRYPTO_CRYPTO_MAC_H
#define	CRYPTO_CRYPTO_MAC_H

#include <set>

#include <event/event_callback.h>

namespace CryptoMAC {
	class Method;

	enum Algorithm {
		MD5,
	};

	class Instance {
	protected:
		Instance(void)
		{ }

	public:
		virtual ~Instance()
		{ }

		virtual bool initialize(const Buffer *) = 0;
		virtual Action *submit(Buffer *, EventCallback *) = 0;
	};

	class Method {
		std::string name_;
	protected:
		Method(const std::string&);

		virtual ~Method()
		{ }
	public:
		virtual std::set<Algorithm> algorithms(void) const = 0;
		virtual Instance *instance(Algorithm) const = 0;

		static const Method *method(Algorithm);
	};
}

std::ostream& operator<< (std::ostream&, CryptoMAC::Algorithm);

#endif /* !CRYPTO_CRYPTO_MAC_H */
