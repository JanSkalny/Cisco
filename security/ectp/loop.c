#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

#include <pcap/pcap.h>

int main(int argc, char **argv) 
{
	pcap_t *p;
	unsigned char *msg, mac[6];
	int len, ret, i;
	char err[PCAP_ERRBUF_SIZE];

	if ( (argc != 3) || 
		 (strlen(argv[2]) < 17) ) {
		fprintf(stderr, "missing argument\n usage: ./loop interface xx:xx:xx:xx:xx:xx\n\n");
		exit(1);
	}

	memset(mac, 0, 6);
	for (i=0; i!=6; i++) 
		mac[i] = strtol(argv[2]+i*3, 0, 16);

	if (!(p = pcap_create(argv[1], err))) {
		fprintf(stderr, "pcap_create failed: %s\n", err);
		exit(1);
	}

	if (pcap_set_snaplen(p, 1500)) 
		pcap_perror(p, "pcap_set_snaplen failed");

	if (pcap_set_promisc(p, 1)) 
		pcap_perror(p, "pcap_set_promisc failed");

	if (pcap_set_timeout(p, 1000)) 
		pcap_perror(p, "pcap_set_promisc failed");

	if ((ret = pcap_activate(p))) {
		pcap_perror(p, "pcap_activate failed");
		exit(1);
	}

	len = 60;

	if (!(msg = malloc(len))) {
		fprintf(stderr, "malloc: out of memory\n");
		exit(1);
	}

	// c0:00:0f:d0:00:00
	// c0:00:0f:d0:00:00
	// 90:00	ECTP
	// 00:00	skip=0
	// 01		function=1 (REPLY)
	memset(msg, 0, len);
	msg[12] = 0x90;
	msg[16] = 0x01;
	memcpy(msg, mac, 6);
	memcpy(msg+6, mac, 6);

	/*
	for (i=0; i!=59; i++) 
		printf("%02x:", msg[i]);
	printf("%02x\n", msg[i+1]);
	*/

	for (i=0; i!=1000; i++) {
		ret = pcap_inject(p, msg, len);
		usleep(10000);
	}
	printf("pcap_inject ret=%d\n", ret);

	free(msg);

	return 0;
}
