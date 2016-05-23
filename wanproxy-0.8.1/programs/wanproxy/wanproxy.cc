#include <common/buffer.h>
#include <common/endian.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_main.h>

#include "wanproxy_config.h"
#include "syncap.h"

static void usage(void);

int
main(int argc, char *argv[])
{
	std::string configfile("");
	bool quiet, verbose;
	int ch;

	quiet = false;
	verbose = false;

	INFO("/tcp-pepd") << "tcp-pepd 1.0";
	INFO("/tcp-pepd") << "Copyright (c) 2015.";
	INFO("/tcp-pepd") << "All rights reserved.";

	while ((ch = getopt(argc, argv, "c:qv")) != -1) {
		switch (ch) {
		case 'c':
			configfile = optarg;
			break;
		case 'q':
			quiet = true;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
		}
	}

	if (configfile == "")
		usage();

	if (quiet && verbose)
		usage();

	if (verbose) {
		Log::mask(".?", Log::Debug);
	} else if (quiet) {
		Log::mask(".?", Log::Error);
	} else {
		Log::mask(".?", Log::Info);
	}

	WANProxyConfig config;
	if (!config.configure(configfile)) {
		ERROR("/tcp_pepd") << "Could not configure proxies.";
		return (1);
	}
	init_syn_handle();
	event_main();
}

static void
usage(void)
{
	INFO("/tcp_pepd/usage") << "tcp_pepd [-q | -v] -c configfile";
	exit(1);
}
