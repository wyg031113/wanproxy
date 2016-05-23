tc qdisc del dev eth0 root
tc qdisc del dev eth1 root
tc qdisc add dev eth0 root handle 1:0 tbf rate 170Mbit burst 15k mtu 8000 latency 50ms
tc qdisc add dev eth0 parent 1:0 handle 10: netem limit 60000 delay 300ms loss 0.001%

tc qdisc add dev eth1 root handle 1:0 tbf rate 170Mbit burst 15k mtu 8000 latency 50ms
tc qdisc add dev eth1 parent 1:0 handle 10: netem limit 60000 delay 300ms loss 0.001%
