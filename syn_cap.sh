#!/bin/bash
queuenum=1
dport=3333
toports=3300
iptables -t mangle -F
iptables -t nat -F
iptables -A PREROUTING -t mangle  -p tcp --dport $dport --syn -j NFQUEUE --queue-num $queuenum
iptables -A PREROUTING -t nat  -p tcp --dport $dport -j REDIRECT --to-ports 3300
#iptables -A PREROUTING -t nat -p tcp --dport 22 -j REDIRECT --to-ports 3303
#iptables -A OUTPUT -t mangle  -p tcp --syn  -j NFQUEUE --queue-num 0
iptables -t mangle -L -v -n
echo ""
echo "--------------------------------------------------------"
iptables -t nat -L -v -n
