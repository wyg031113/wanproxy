echo "420960	327680	23214200"  > /proc/sys/net/ipv4/tcp_mem
echo "4096	32768	23214200" >  /proc/sys/net/ipv4/tcp_rmem
echo "4096	32768	23214200" > /proc/sys/net/ipv4/tcp_wmem
echo "hybla" > /proc/sys/net/ipv4/tcp_congestion_control
