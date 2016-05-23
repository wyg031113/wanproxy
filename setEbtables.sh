#!/bin/sh

###Date: 12-Oct-2007

###tekbdrlimbu@hotmail.com####

#!/bin/bash
/sbin/ebtables -t broute -A BROUTING -p IPv4 --ip-protocol tcp -j redirect --redirect-target ACCEPT
