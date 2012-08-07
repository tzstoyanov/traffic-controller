#ifndef __OS_LINUX__
#define __OS_LINUX__

#include <endian.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <regex.h>
#include <string.h>
#include <stdarg.h> 
#include <errno.h>
#include <ctype.h>
#include <endian.h>
#include <syslog.h> 
#include <semaphore.h> 
 
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/sem.h>
#include <sys/ipc.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>

#include <getopt.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>

#ifdef HAVE_IPV6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#endif

#define HTON_DEFINED	1

#endif /* __OS_LINUX__ */

