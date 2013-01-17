#include <stdio.h>
#include <stdlib.h>
#include "mv_eth_proc.h"

extern char **environ; /* all environments */

static unsigned int port = 0, q = 0, weight = 0, status = 0, mac[6] = {0,};
static unsigned int policy =0, command = 0, packet = 0;
static unsigned int value = 0;
static unsigned int inport, outport, dip, sip, da[6] = {0, }, sa[6] = {0, };
static unsigned int db_type = 0;

void show_usage(int badarg)
{
        fprintf(stderr,
	"Usage: 										\n"
	" mv_eth_tool -h		       Display this help				\n"
	"                                                                                       \n"
	" --- Global commands ---								\n"
        " mv_eth_tool -txdone <quota>          Set threshold to start tx_done operations       	\n"
	" mv_eth_tool -reuse   <0|1>           SKB reuse support:   0 - Disable, 1 - Enable    	\n"
        " mv_eth_tool -recycle <0|1>           SKB recycle support: 0 - Disable, 1 - Enable     \n"
        " mv_eth_tool -nfp <0|1>               NFP support: 0 - Disable, 1 - Enable             \n"
        "                                                                                       \n"
	" --- Port commands ---                                                                 \n"
        " mv_eth_tool -rxcoal <port> <usec>    Set RX interrupt coalescing value               	\n"
        " mv_eth_tool -txcoal <port> <usec>    Set TX interrupt coalescing value               	\n"
	" mv_eth_tool -txen   <port> <deep>    Set deep of lookup for TX enable race.          	\n"
        "                                      0 - means disable.                              	\n"
	" mv_eth_tool -ejp    <port> <0|1>     Set EJP mode: 0 - Disable, 1 - Enable           	\n"
	" mv_eth_tool -tx_noq <idx> <0|1>      Set queuing mode: 0 - Disable, 1 - Enable        \n"
	" mv_eth_tool -lro    <port> <0|1>     Set LRO mode: 0 - Disable, 1 - Enable           	\n"
	" mv_eth_tool -lro_desc <port> <value> Set LRO max concurrent sessions			\n"
	"                                                                                       \n"
	" --- Port/Queue commands ---                                                           \n"
	" mv_eth_tool -tos <port> <rxq> <tos>  Map TOS field to RX queue number                 \n" 
	"                                                                                       \n"
	" mv_eth_tool -srq <port> <rxq> <bpdu|arp|tcp|udp>                                     	\n"
        "                                      Set RX queue number for different packet types. 	\n"
	"                                      rxq==8 means no special treatment.		\n"
	"		                       rxq==8 for Arp packets means drop.      		\n"
	"                                                                                       \n"
	" mv_eth_tool -sq <port> <rxq> <%%2x:%%2x:%%2x:%%2x:%%2x:%%2x> 				\n"
	"                                      Set RX queue number for a Multicast MAC         	\n"
	"                                      rxq==8 indicates delete entry.		        \n"
        " mv_eth_tool -srp <port> <WRR|FIXED>  Set the Rx Policy to WRR or Fixed 		\n"
	"											\n"
	" --- NFP commands ---                                                                  \n"
        " mv_eth_tool -fp_st                   Display NFP Status (enabled/disabled)            \n"
        " mv_eth_tool -fprs <inp> <outp> <DIP> <SIP> <DA> <SA>					\n"
        "                                      Set NFP rule for IPv4 routing			\n"
        "                                      where DA, SA - MAC addresses xx:xx:xx:xx:xx:xx   \n"
	" mv_eth_tool -fprd <DIP> <SIP>        Delete NFP Rule for IPv4 routing			\n"
	" mv_eth_tool -fp_print <DB>           Print NFP Rule Database,                        	\n"
        "                                      where DB is routing,nat,bridge,ppp,sec           \n"
	"											\n"
	" mv_eth_tool -St <option> <port>			       		                \n"
	"   Display different status information of the port through the kernel printk.		\n"
	"   OPTIONS:										\n"
	"   p                                   Display General port information.		\n"
        "   mac                                 Display MAC addresses information               \n"
	"   q   <0..7>                          Display specific q information.			\n"
	"   rxp                                 Display Rx Policy information.			\n"
	"   txp                                 Display Tx Policy information.			\n"
	"   cntrs                               Display the HW MIB counters			\n"
	"   regs                                Display a dump of the giga registers		\n"
	"   stats                               Display port statistics information. 		\n"
	"   netdev                              Display net_device status information.          \n"
	"   nfp                                 Display port NFP statistics                     \n"
	"   tos                                 Display port TOS to RXQ mapping                 \n"
	"   switch                              Display switch statistics                 	\n"
        "\n"	
        );
        exit(badarg);
}

static void parse_pt(char *src)
{
        if (!strcmp(src, "bpdu"))
        	packet = PT_BPDU;
	else if(!strcmp(src, "arp"))
        	packet = PT_ARP;
	else if(!strcmp(src, "tcp"))
        	packet = PT_TCP;
	else if(!strcmp(src, "udp"))
        	packet = PT_UDP;
	else {
		fprintf(stderr, "Illegall packet type, packet type should be bpdu/arp/tcp/udp. \n");	
                exit(-1);
        }
        return;
}

static void parse_db_name(char *src)
{
	if (!strcmp(src, "routing") || !strcmp(src, "route"))
		db_type = DB_ROUTING;
	else if (!strcmp(src, "nat"))
		db_type = DB_NAT;
	else if (!strcmp(src, "fdb") || !strcmp(src, "bridge"))
		db_type = DB_FDB;
	else if (!strcmp(src, "ppp"))
		db_type = DB_PPP;
	else if (!strcmp(src, "sec"))
		db_type = DB_SEC;
	else {
		fprintf(stderr, "Illegall DB name, expected routing|nat|bridge|ppp|sec. \n");
		exit(-1);
	}
	return;
}

static void parse_port(char *src)
{
	int count;

        count = sscanf(src, "%x",&port);

        if (count != 1)  {
		fprintf(stderr, "Port parsing error: count=%d\n", count);	
                exit(-1);
        }
        return;
}


static void parse_q(char *src)
{
	int count;

        count = sscanf(src, "%x",&q);

        if (count != 1) {
		fprintf(stderr, "Queue parsing error: count=%d\n", count);	
                exit(-1);
        }
        return;
}

static void parse_policy(char *src)
{

        if (!strcmp(src, "WRR"))
        	policy = WRR;
	else if (!strcmp(src, "FIXED"))
        	policy = FIXED;
	else {
		fprintf(stderr, "Illegall policy, policy can be WRR or Fixed.\n");	
                exit(-1);
        }
        return;
}

static void parse_status(char *src)
{
    	if (!strcmp(src, "p")) {
      		status = STS_PORT;
      	}
        else if (!strcmp(src, "mac")) {
                status = STS_PORT_MAC;
        }
     	else if(!strcmp(src, "q")) {
       		status = STS_PORT_Q;
     	}
   	else if(!strcmp(src, "rxp")) {
          	status = STS_PORT_RXP;
      	}
     	else if(!strcmp(src, "txp")) {
           	status = STS_PORT_TXP;
      	}
        else if(!strcmp(src, "cntrs")) {
                status = STS_PORT_MIB;
        }
        else if(!strcmp(src, "regs")) {
                status = STS_PORT_REGS;
        }
      	else if(!strcmp(src, "stats")) {
             	status = STS_PORT_STATS;
        }
        else if(!strcmp(src, "nfp")) {
                status = STS_PORT_NFP_STATS;
        }
	else if(!strcmp(src, "netdev")) {
                status = STS_NETDEV;
        }
        else if(!strcmp(src, "tos")) {
                status = STS_PORT_TOS_MAP;
        }
        else if(!strcmp(src, "switch")) {
                status = STS_SWITCH_STATS;
        }
        else {
                fprintf(stderr, "Illegall ststus %d.\n");
                exit(-1);
        }
        return;
}
static void parse_dec_val(char *src, unsigned int* val_ptr)
{
    int i, count;

    count = sscanf(src, "%d", val_ptr);
    if(count != 1) {
        fprintf(stderr, "Illegall value - should be decimal.\n");
        exit(-1);
    }
    return;
}

static void parse_hex_val(char *src, unsigned int* hex_val_ptr)
{
    int i, count;

    count = sscanf(src, "%x", hex_val_ptr);
    if(count != 1) {
        fprintf(stderr, "Illegall value - should be hex.\n");
        exit(-1);
    }
    return;
}

static int parse_mac(char *src, unsigned int macaddr[])
{
        int count;
        int i;
        int buf[6];

        count = sscanf(src, "%2x:%2x:%2x:%2x:%2x:%2x",
                &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);

        if (count != 6) {
		fprintf(stderr, "MAC parsing error: Expected %%2x:%%2x:%%2x:%%2x:%%2x:%%2x.\n");
                exit(-1);
        }

        for (i = 0; i < count; i++) {
                macaddr[i] = buf[i];
        }
        return 0;
}

static int parse_ip(char *src, unsigned int* ip)
{
    int count, i;
    int buf[4];

    count = sscanf(src, "%d.%d.%d.%d",
                &buf[0], &buf[1], &buf[2], &buf[3]);

    if (count != 4) {
        fprintf(stderr, "Illegall IP address (should be %%d.%%d.%%d.%%d)\n");
        exit(-1);
    }
    *ip = (((buf[0] & 0xFF) << 24) | ((buf[1] & 0xFF) << 16) |
           ((buf[2] & 0xFF) << 8) | ((buf[3] & 0xFF) << 0));
    return 0;
}

static void parse_cmdline(int argc, char **argp)
{
	unsigned int i = 1;

	if(argc < 2) {
		show_usage(1);
	}

	if (!strcmp(argp[i], "-h")) {
		show_usage(0);
	}
	else if (!strcmp(argp[i], "-srq")) {
		command = COM_SRQ;
		i++;	
		if(argc != 5)
			show_usage(1); 
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_pt(argp[i++]);
	}
	else if (!strcmp(argp[i], "-sq")) {
		command = COM_SQ;
		i++;
		if(argc != 6)
			show_usage(1); 
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_mac(argp[i++], mac);
	}
	else if (!strcmp(argp[i], "-srp")) {
		command = COM_SRP;
		i++;
		if(argc != 4)
			show_usage(1); 
		parse_port(argp[i++]);
		parse_policy(argp[i++]);
	}
	else if (!strcmp(argp[i], "-srqw")) {
		command = COM_SRQW;
		i++;
		if(argc != 5)
			show_usage(1); 
		parse_port(argp[i++]);
		parse_q(argp[i++]);
		parse_hex_val(argp[i++], &weight);
	}
    	else if (!strcmp(argp[i], "-stp")) {
        	command = COM_STP;
        	i++;
        	if(argc != 6)
            		show_usage(1);
		parse_port(argp[i++]);
        	parse_q(argp[i++]);
		parse_hex_val(argp[i++], &weight);
		parse_policy(argp[i++]);
    	}
    	else if (!strcmp(argp[i], "-fprs")) {
        	command = COM_IP_RULE_SET;
        	i++;
        	if(argc != 8)
            		show_usage(1);
		parse_dec_val(argp[i++], &inport);
        	parse_dec_val(argp[i++], &outport);
		parse_ip(argp[i++], &dip);
		parse_ip(argp[i++], &sip);
        	parse_mac(argp[i++], da);
		parse_mac(argp[i++], sa);
    	}
	else if (!strcmp(argp[i], "-fprd")) {
		command = COM_IP_RULE_DEL;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_ip(argp[i++], &dip);
		parse_ip(argp[i++], &sip);
	}
	else if (!strcmp(argp[i], "-fp_st")) {
		command = COM_NFP_STATUS;
		if(argc != 2)
			show_usage(1);
	}
	else if (!strcmp(argp[i], "-fp_print")) {
		command = COM_NFP_PRINT;
		i++;
		if (argc != 3)
			show_usage(1);
		parse_db_name(argp[i++]);
	}
    	else if (!strcmp(argp[i], "-txdone")) {
        	command = COM_TXDONE_Q;
        	i++;
		if(argc != 3)
			show_usage(1);
        	parse_dec_val(argp[i++], &value);
    	}
    	else if (!strcmp(argp[i], "-txen")) {
                command = COM_TX_EN;
                i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
                parse_dec_val(argp[i++], &value);
        }
	else if (!strcmp(argp[i], "-lro")) {
		command = COM_LRO;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_dec_val(argp[i++], &value);
	}
	else if (!strcmp(argp[i], "-lro_desc")) {
		command = COM_LRO_DESC;
		i++;
		if(argc != 4)
			show_usage(1);
		parse_port(argp[i++]);
		parse_dec_val(argp[i++], &value);
	}
        else if (!strcmp(argp[i], "-reuse")) {
                command = COM_SKB_REUSE;
                i++;
		if(argc != 3)
			show_usage(1);
                parse_dec_val(argp[i++], &value);
        }
        else if (!strcmp(argp[i], "-recycle")) {
                command = COM_SKB_RECYCLE;
                i++;
                if(argc != 3)
                        show_usage(1);
                parse_dec_val(argp[i++], &value);
        }
        else if (!strcmp(argp[i], "-nfp")) {
                command = COM_NFP;
                i++;
                if(argc != 3)
                        show_usage(1);
                parse_dec_val(argp[i++], &value);
	}
	else if (!strcmp(argp[i], "-rxcoal")) {
        	command = COM_RX_COAL;
        	i++;
		if(argc != 4)
			show_usage(1);
        	parse_port(argp[i++]);
        	parse_dec_val(argp[i++], &value);
    	}
    	else if (!strcmp(argp[i], "-txcoal")) {
        	command = COM_TX_COAL;
        	i++;
		if(argc != 4)
			show_usage(1);
        	parse_port(argp[i++]);
        	parse_dec_val(argp[i++], &value);
    	}
        else if (!strcmp(argp[i], "-ejp")) {
                command = COM_EJP_MODE;
                i++;
                if(argc != 4)
                        show_usage(1);
                parse_port(argp[i++]);
                parse_dec_val(argp[i++], &value);
	}
        else if (!strcmp(argp[i], "-tos")) {
                command = COM_TOS_MAP;
                i++;
                if(argc != 5)
                        show_usage(1);
                parse_port(argp[i++]);
		parse_q(argp[i++]);
                parse_hex_val(argp[i++], &value);
        }
		else if (!strcmp(argp[i], "-tx_noq")) {
				command = COM_TX_NOQUEUE;
				i++;
				if(argc != 4)
						show_usage(1);
				parse_port(argp[i++]);
				parse_dec_val(argp[i++], &value);
	}
    	else if (!strcmp(argp[i], "-St")) {
        	command = COM_STS;
        	i++;
		if(argc < 4)
			show_usage(1);
		parse_status(argp[i++]);
		if( status == STS_PORT_Q ) {
            		if(argc != 5)
                		show_usage(1);
               		parse_q(argp[i++]);
	    	}
            	else if(argc != 4)
                	show_usage(1);
	
		parse_port(argp[i++]);
        }	
	else {
		show_usage(i++);
	}
}

static int procit(void)
{
  	FILE *mvethproc;
  	mvethproc = fopen(FILE_PATH FILE_NAME, "w");
  	if (!mvethproc) {
    		printf ("Eror opening file %s/%s\n",FILE_PATH,FILE_NAME);
    		exit(-1);
  	}

	switch (command) {
		case COM_TXDONE_Q:
		case COM_SKB_REUSE:
	        case COM_SKB_RECYCLE:
	  	case COM_NFP:
			fprintf (mvethproc, ETH_CMD_STRING, ETH_PRINTF_LIST);
			break;
		case COM_RX_COAL:
		case COM_TX_COAL:
		case COM_TX_EN:
		case COM_EJP_MODE:
		case COM_TX_NOQUEUE:
		case COM_LRO:
		case COM_LRO_DESC:
			fprintf (mvethproc, PORT_CMD_STRING, PORT_PRINTF_LIST);
			break;
		case COM_TOS_MAP:
			fprintf (mvethproc, QUEUE_CMD_STRING, QUEUE_PRINTF_LIST);
			break;
		case COM_IP_RULE_SET:
			fprintf(mvethproc, IP_RULE_STRING, IP_RULE_PRINT_LIST);
			break;
		case COM_IP_RULE_DEL:
			fprintf(mvethproc, IP_RULE_DEL_STRING, IP_RULE_DEL_PRINT_LIST);
			break;
               case COM_NFP_STATUS:
                       fprintf(mvethproc, CMD_STRING, CMD_PRINT_LIST);
                       break;
		case COM_NFP_PRINT:
			fprintf(mvethproc, NFP_DB_PRINT_STRING, NFP_DB_PRINT_PRINT_LIST);
			break;
		default:
			fprintf (mvethproc, PROC_STRING, PROC_PRINT_LIST);
			break;
	}

	fclose (mvethproc);
	return 0;
}

int main(int argc, char **argp, char **envp)
{
        parse_cmdline(argc, argp);
        return procit();
}

