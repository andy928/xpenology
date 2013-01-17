#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/telephony.h>
#include <time.h>

#define USE_LINEAR

#ifdef USE_LINEAR
#define BUFSIZE 160
#else
#define BUFSIZE	80
#endif

#define PHONE_MV_READ_SLIC_REG	_IOWR ('q', 0xB0, unsigned char)
#define PHONE_MV_WRITE_SLIC_REG	_IOW ('q', 0xB1, unsigned int)
#define PHONE_MV_READ_REG	_IOWR ('q', 0xB2, unsigned int)
#define PHONE_MV_WRITE_REG	_IOW ('q', 0xB3, unsigned int)
#define PHONE_MV_SPI_TEST	_IO ('q', 0xB4)
#define PHONE_MV_STAT_SHOW	_IO ('q', 0xB5)

#define MV_TOOL_VER	"ver 1.3"
#define DTMF_SIZE	1600
#define SILENCE_SIZE	600

/* 
v1.2, Tzachi, Dec 01, Add start/stop ioctl in loopback.
*/

/* FXS APIs */
void sw_loopback(int fd, int pcm);
void digital_loopback(int fd,int pcm);
void sw_loopback_two_phones(int fd0, int fd1, int pcm);
int sw_tone(int fd, int pcm);
int spi_test(int fd);

/* FXO APIs */
void fxo_dtmf_test(int fd, int pcm);
void fxs_fxo_dtmf_test(int fd0,int fd1, int pcm);


/*#define BLOCKING_RW*/

int main(int argc, char *argv[])
{
	char *name;
	int fd, fd1, t, i, fdflags,pcm;
	int rc = 0;

	if (argc > 1) {
		name = argv[1];

		/* open the device */
		fd = open(name, O_RDWR); 
		if (fd <= 0) {	
			printf("## Cannot open %s device.##\n",name);
			exit(2);
		}

		/* set some flags */
		fdflags = fcntl(fd,F_GETFL,0);
		fdflags |= O_NONBLOCK;
		fcntl(fd,F_SETFL,fdflags);
	}
	else {
		name = "/dev/fxs0";
		fd = open(name, O_RDWR); 
		if (fd <= 0) {	
			printf("## Cannot open %s.##\n",name);
			exit(2);
		}
#ifndef BLOCKING_RW
		fdflags = fcntl(fd,F_GETFL,0);
		fdflags |= O_NONBLOCK;
		fcntl(fd,F_SETFL,fdflags);
#endif
		name = "/dev/fxs1";
		fd1 = open(name, O_RDWR); 
		if (fd1 <= 0) {	
			printf("## Cannot open %s.##\n",name);
			exit(2);
		}
		
#ifndef BLOCKING_RW
		fdflags = fcntl(fd1,F_GETFL,0);
		fdflags |= O_NONBLOCK;
		fcntl(fd1,F_SETFL,fdflags);
#endif
	}
		name = "/dev/pcm";
		pcm = open(name, O_RDWR); 
		if (pcm <= 0) {	
			printf("## Cannot open %s.##\n",name);
			exit(2);
		}
#ifndef BLOCKING_RW
		fdflags = fcntl(pcm,F_GETFL,0);
		fdflags |= O_NONBLOCK;
		fcntl(pcm,F_SETFL,fdflags);
#endif


	while(1) {
		char str[32];
		int reg=0;
		int val=0;
		printf("\n\tMarvell VoIP Tool %s:\n",MV_TOOL_VER);
		printf("\t0. Read ProSLIC register\n");
		printf("\t1. Write ProSLIC register\n");
                printf("\t2. Read MV internal register\n");
                printf("\t3. Write MV internal register\n");
		printf("\t4. Start ring\n");
		printf("\t5. Stop ring\n");
		printf("\t6. Start SLIC dial-tone\n");
		printf("\t7. Stop SLIC dial-tone\n");
                printf("\t8. Start SW tone\n");
                printf("\t9. SPI stress test\n");
                printf("\ta. Self echo on local phone (CTRL+C to stop)\n");
                printf("\tb. Loopback two local phones (CTRL+C to stop)\n");
		printf("\tc. Show driver statistics\n");
		printf("\td. DTMF test (FXO)\n");
		printf("\te. DTMF test 2 (FXS --> FXO)\n");
		printf("\tf. Multi session(5 channels) (2xFXS, 1xFXS, FXS --> FXO)\n");
		printf("\tg. Digital Loopback\n");
		printf("\tx. exit\n");

		gets(str);
		switch(str[0])
		{
                        case '0':
                                printf("Enter SLIC register offset: ");
                                gets(str);
                                reg = atoi(str);
                                printf("Reading SLIC register %d: ",reg);
                                sleep(1);
                                rc = ioctl(fd,PHONE_MV_READ_SLIC_REG,reg);
                                if(rc>=0)
                                        printf("0x%x\n",rc);
                                break;
                        case '1':
                                printf("Enter SLIC register offset: ");
                                gets(str);
                                reg = atoi(str);
                                printf("Enter value: ");
                                gets(str);
                                val = atoi(str);
                                printf("Writing %d to SLIC register %d\n",val,reg);
                                reg <<= 16;
                                reg |= val;
                                sleep(1);
                                rc = ioctl(fd,PHONE_MV_WRITE_SLIC_REG,reg);
                                break;
                        case '2':
                                printf("Enter MV internal register offset: ");
                                gets(str);
                                reg = atoi(str);
                                printf("Reading MV internal register %x: ",reg);
                                sleep(1);
                                rc = ioctl(fd,PHONE_MV_READ_REG,reg);
                                if(rc>=0)
                                        printf("0x%x\n",rc);
                                break;
                        case '3':
                                printf("Not supported\n");
                                break;
			case '4':
				printf("Start ringing...\n");
				sleep(1);
                                rc = ioctl(fd,PHONE_RING_START,0);
				break;
                        case '5':
		                printf("Stop ringing\n");
				sleep(1);
                                rc = ioctl(fd,PHONE_RING_STOP,0);
                                break;
                        case '6':
				printf("Start SLIC dial-tone...\n");
				sleep(1);
                                rc = ioctl(fd,PHONE_DIALTONE,0);
                                break;
			case '7':
				printf("Stop SLIC dial-tone\n");
				sleep(1);
				rc = ioctl(fd,PHONE_CPT_STOP,0);
				break;
                        case '8':
		                printf("Start SW tone...\n");
				rc = sw_tone(fd, pcm);
				break;
			case '9':
                                printf("Starting SPI tress test\n");
				sleep(1);
                                rc = ioctl(fd,PHONE_MV_SPI_TEST);
                                break;
			case 'a':
                                printf("Entering loop back on local phone...\n");
                                sw_loopback(fd, pcm);
				break;
			case 'b':
                                printf("Entering loop back between two local phones...\n");
                                sw_loopback_two_phones(fd,fd1,pcm);
				break;
			case 'c':
				printf("Driver statistics\n");
				sleep(1);
				rc = ioctl(fd,PHONE_MV_STAT_SHOW);
				break;
			case 'd':
				printf("DTMF test (FXO)\n");
				sleep(1);
				fxo_dtmf_test(fd, pcm);
				break;
			case 'e':
				printf("Voice call (FXS --> FXO)\n");
				sleep(1);
				fxs_fxo_dtmf_test(fd,fd1,pcm);
				break;
			case 'f':
				printf("Multi session(5 channels) (2xFXS, 1xFXS, FXS --> FXO)\n");
				sleep(1);
				multi_session_test(fd,fd1,pcm);
				break;
			case 'g':
				printf("Digital Loopback\n");
				sleep(1);
				digital_loopback(fd,pcm);
				break;
                        case 'x':
				exit(0);
			default:
				printf("Invalid command\n");
		}
	}

	if(rc<0) printk("Tool failed to perform action!\n");
	close(fd);
	close(fd1);
	clese(pcm);
	return 0;
}

void fxo_dtmf_test(int fd, int pcm)
{
	char rd_buff[5*BUFSIZE], number[20];
	int count=1,total=0, dtmf_length,fd_org;
	char *dtmf_name = "/usr/dtmf.pcm";
	fd_set wr_fds, rd_fds, ex_fds, master;
        int i = 0, dtmf_fd = open(dtmf_name,O_RDWR);
        if(dtmf_fd <= 0) {
                printf("$$ Cannot open %s.$$\n",dtmf_name);
                exit(2);
        }

	fd_org = dtmf_fd;

	/* go OFFHOOK */
	ioctl(fd, PHONE_PSTN_SET_STATE, 2);
	sleep(1);

	printf("$$hook is off$$\n");

	printf("$$Please enter phone number: ");
	gets(number);
        
	ioctl(fd, PHONE_PLAY_START);
	printf("$$Dialing:");

	while (number[i] != '\0') 
	{
		dtmf_length = DTMF_SIZE + SILENCE_SIZE;
		dtmf_fd = fd_org;
		lseek(dtmf_fd, ((int)(number[i] - '0') * dtmf_length), SEEK_SET);
		printf("%d ",(int)(number[i] - '0'));

	  while(dtmf_length > 0)
	  {
		
		FD_ZERO(&wr_fds);
	        FD_ZERO(&rd_fds);
        	FD_ZERO(&ex_fds);

		FD_SET(pcm,&wr_fds);
                FD_SET(pcm,&rd_fds);
                FD_SET(pcm,&ex_fds);
	
		select(pcm+1,&rd_fds,&wr_fds,&ex_fds,NULL);


                if(FD_ISSET(pcm,&ex_fds)) {
				int ex;
				ex = ioctl(fd,PHONE_EXCEPTION,0);
                     		printf("$$exception 0x%x$$\n",ex);
                }

		if(FD_ISSET(pcm,&rd_fds)) {
				printf("$$read$$\n");
		}

		if(FD_ISSET(pcm,&wr_fds)) {
			count = dtmf_length > BUFSIZE ? BUFSIZE : dtmf_length;
			count = read(dtmf_fd, rd_buff+BUFSIZE, count);
			/*printf("read=%d\n",count);*/
			count = write(pcm, rd_buff, 5*BUFSIZE);
			dtmf_length -= count;
			total+=count;
		/*	printf("write=%d\n",count);*/
        	}
	   }
	   i++;
	}

	dtmf_fd = fd_org;
	//sleep(10);
	ioctl(fd,PHONE_PLAY_STOP);

	/* go ONHOOK */
	ioctl(fd, PHONE_PSTN_SET_STATE, 0);
	sleep(1);
	printf("\n$$hook is on$$\n");
	printf("$$Send %d samples$$\n",(total/2));

	close(dtmf_fd);

	return;

}

void fxs_fxo_dtmf_test(int fd0,int fd1,int pcm)
{
	int fail_count=0, i = 0, hook_state = 0, max = 0;
	static int phone_dev_fd[2];
	unsigned char aud_buf[2][5*BUFSIZE]={0};
	fd_set wr_fds, rd_fds, ex_fds, master;

	phone_dev_fd[0]=fd0;
	phone_dev_fd[1]=fd1;

		
	FD_ZERO(&master);    /*  clear the master and temp sets*/
	FD_ZERO(&rd_fds);
	FD_ZERO(&wr_fds);
	FD_ZERO(&ex_fds);

	for(i = 0; i < 2; i++) 
		FD_SET(phone_dev_fd[i], &master);

	/* extract max fd */
	max = (fd0 > fd1) ? fd0 : fd1;
	max = (pcm > max) ? pcm : max;

	FD_SET(pcm, &master);

	/* go OFFHOOK on FXO */
	ioctl(fd1, PHONE_PSTN_SET_STATE, 2);
	sleep(1);

	while (fail_count<5) 
	{
		int a, slic_ex;
		
		/* copy it */
		rd_fds = master; 
		wr_fds = master;
		ex_fds = master;

		select(max+1,&rd_fds,&wr_fds,&ex_fds,NULL);
	 
                if(FD_ISSET(fd0, &ex_fds))
 		{
                        int ex;
                        ex = ioctl(phone_dev_fd[0],PHONE_EXCEPTION,0);
		 
                        if(ex > 0) 
			{
                                	if(ex & 2 && !hook_state) 
					{
							hook_state = 1;
							ioctl(fd0,PHONE_PLAY_START);
							ioctl(fd0,PHONE_REC_START);	
							ioctl(fd1,PHONE_REC_START);
							ioctl(fd1,PHONE_PLAY_START);
							printf("-> hook-off\n");
					}
                                	else if(ex & (1 << 16) && hook_state)
 					{
							hook_state = 0;
							ioctl(fd0,PHONE_PLAY_STOP);
                                        		ioctl(fd1,PHONE_PLAY_STOP);
							ioctl(fd0,PHONE_REC_STOP);
							ioctl(fd1,PHONE_REC_STOP);
                                        		printf("-> hook-on\n");
							goto fxs_fxo_exit;
					}
			}
                        else 
			{
                                printf("$$PHONE_EXCEPTION failed$$\n");
                                fail_count++;
                        }
		  
                }
		 
		if(FD_ISSET(pcm, &rd_fds)) {
			a = read(pcm, aud_buf[0], 5*BUFSIZE);
			//printf("read %d\n",a);
			if (a <= 0) {
				printf("$$read() failed.$$\n");
				fail_count++;
			}
			memcpy(aud_buf[1], aud_buf[0]+1*BUFSIZE, BUFSIZE);
			memcpy(aud_buf[1]+1*BUFSIZE, aud_buf[0], BUFSIZE);
		}

		if(FD_ISSET(pcm, &wr_fds)) {
			a = write(pcm, aud_buf[1], 5*BUFSIZE);
        	       	if (a <= 0) {
                       			printf("$$write() failed.$$\n");
                        		fail_count++;
			}
			//printf("write %d\n",a);
		}
	}
fxs_fxo_exit:
	/* go ONHOOK on FXO */
	ioctl(fd1, PHONE_PSTN_SET_STATE, 0);
	sleep(2);
	return;
}

void multi_session_test(int fd0,int fd1,int pcm)
{

	//char rd_buff[BUFSIZE];
	char *name;
	int fail_count=0, i = 0,max = 0,fd2,fd3,fd4;
	static int phone_dev_fd[5];
	unsigned char aud_buf[2][5*BUFSIZE];
	int dev_fd, first_offhook = 1, on_hook = 0;
	int ex, hook_state[5] = {0, 0, 0, 0, 0};

	fd_set wr_fds, rd_fds, ex_fds, master;

	/* open devices: 2,3,4 */
	name = "/dev/fxs2";
	fd2 = open(name, O_RDWR); 
	if (fd2 <= 0) {	
		printf("## Cannot open %s.##\n",name);
		exit(2);
	}
		
	name = "/dev/fxs3";
	fd3 = open(name, O_RDWR); 
	if (fd3 <= 0) {	
		printf("## Cannot open %s.##\n",name);
		exit(2);
	}

	name = "/dev/fxo0";
	fd4 = open(name, O_RDWR); 
	if (fd4 <= 0) {	
		printf("## Cannot open %s.##\n",name);
		exit(2);
	}

	phone_dev_fd[0]=fd0;
	phone_dev_fd[1]=fd1;
	phone_dev_fd[2]=fd2;
	phone_dev_fd[3]=fd3;
	phone_dev_fd[4]=fd4;

	FD_ZERO(&wr_fds);
	FD_ZERO(&rd_fds);
	FD_ZERO(&ex_fds);
	FD_ZERO(&master);

	for(i = 0; i < 5; i++) {
		FD_SET(phone_dev_fd[i], &master);
		max = (phone_dev_fd[i] > max) ? phone_dev_fd[i] : max;
	}
	
	FD_SET(pcm, &master);
	max = (pcm > max) ? pcm : max;
	
	while (fail_count<5) 
	{
		int a, slic_ex;
		
		/* copy it */
		rd_fds = master; 
		wr_fds = master;
		ex_fds = master;
		
		FD_ZERO(&wr_fds);
		FD_ZERO(&rd_fds);
		FD_ZERO(&ex_fds);
		FD_ZERO(&master);

		if(select(max+1,&rd_fds,&wr_fds,&ex_fds,NULL) == -1) {
			printf("Error select()\n");
			return;	
		}
	
		for(i = 0; i < 5;i++)
		   {
		     /* Get file descriptor */
		     dev_fd = phone_dev_fd[i];
 
                     if(FD_ISSET(dev_fd,&ex_fds))
 		     {
                        ex = 0;
                        ex = ioctl(dev_fd,PHONE_EXCEPTION,0);
		 
                        if(ex > 0) 
			{
				if((i == 4) && (ex & 4)) /* ring */
				{
					printf("Ringing...\n");
					ioctl(fd0,PHONE_RING_START,0);
				}
                                else if((ex & 2) && (hook_state[i] == 0)) /* off-hook */
				{
				   printf("-> off-hook(dev-%d)\n", i);
				   hook_state[i] = 1;
				   if(first_offhook) {
					ioctl(fd0,PHONE_RING_STOP,0);
					/* go OFFHOOK on FXO */
					ioctl(fd4, PHONE_PSTN_SET_STATE, 2);
					first_offhook = 0;
					on_hook = 0;	
					ioctl(fd0,PHONE_REC_START);
					ioctl(fd1,PHONE_REC_START);
					ioctl(fd2,PHONE_REC_START);
					ioctl(fd3,PHONE_REC_START);
					ioctl(fd4,PHONE_REC_START);

					ioctl(fd0,PHONE_PLAY_START);
					ioctl(fd1,PHONE_PLAY_START);
					ioctl(fd2,PHONE_PLAY_START);
					ioctl(fd3,PHONE_PLAY_START);
					ioctl(fd4,PHONE_PLAY_START);
				   }
				}
                                else if((ex & (1 << 16)) && (hook_state[i] == 1)) /* on-hook */
 				{
					on_hook++;
					hook_state[i] = 0;
					printf("-> on-hook(dev-%d)\n", i);
					if(on_hook == 4) {
						first_offhook = 1;
						/* go ONHOOK on FXO */
						ioctl(fd4, PHONE_PSTN_SET_STATE, 0);

						ioctl(fd0,PHONE_REC_STOP);
						ioctl(fd1,PHONE_REC_STOP);
						ioctl(fd2,PHONE_REC_STOP);
						ioctl(fd3,PHONE_REC_STOP);
						ioctl(fd4,PHONE_REC_STOP);

						ioctl(fd0,PHONE_PLAY_STOP);
						ioctl(fd1,PHONE_PLAY_STOP);
						ioctl(fd2,PHONE_PLAY_STOP);
						ioctl(fd3,PHONE_PLAY_STOP);
						ioctl(fd4,PHONE_PLAY_STOP);	
                                       		goto multi_session_exit;
					}
				}
			}
		   } //for 
                }
		 
		if(FD_ISSET(pcm, &rd_fds)) {
			a = read(pcm, aud_buf[0], 5*BUFSIZE);
			//printf("read %d\n",a);
			if (a <= 0) {
				printf("$$read() failed.$$\n");
				fail_count++;
			}
			memcpy(aud_buf[1], aud_buf[0]+(4*BUFSIZE), BUFSIZE); // 0 <--> 4
			memcpy(aud_buf[1]+(4*BUFSIZE), aud_buf[0], BUFSIZE); // 0 <--> 4

			memcpy(aud_buf[1]+(2*BUFSIZE), aud_buf[0]+BUFSIZE, BUFSIZE); // 1 <--> 2
			memcpy(aud_buf[1]+BUFSIZE, aud_buf[0]+(2*BUFSIZE), BUFSIZE); // 1 <--> 2

			memcpy(aud_buf[1]+(3*BUFSIZE), aud_buf[0]+(3*BUFSIZE), BUFSIZE); // 3 <--> 3

		}

		if(FD_ISSET(pcm, &wr_fds)) {
			a = write(pcm, aud_buf[1], 5*BUFSIZE);
        	       	if (a <= 0) {
                       			printf("$$write() failed.$$\n");
                        		fail_count++;
			}
			//printf("write %d\n",a);
		}
	}
multi_session_exit:
	/* go ONHOOK on FXO */
	ioctl(fd4, PHONE_PSTN_SET_STATE, 0);
	close(fd2);
	close(fd3);
	close(fd4);
	return;

}


void sw_loopback_two_phones(int fd0, int fd1, int pcm)
{
	fd_set master;   	/* master file descriptor list */
	fd_set read_fds; 	/* temp file descriptor list for select() */
	fd_set write_fds; 	/* temp file descriptor list for select() */
	fd_set event_fds; 	/* temp file descriptor list for select() */
	int dev_fd;       	/* tmp file descriptor number */
	int i, max, fdmax;
	int msg_len, session_started = 0;
	unsigned char aud_buf[2][5*BUFSIZE]={0};
	static int phone_dev_fd[2];
	int offhook_count = 0, hook_state[2] = {0, 0};

	phone_dev_fd[0]=fd0;
	phone_dev_fd[1]=fd1;

	/* extract max fd */
	fdmax = (fd0 > fd1) ? fd0 : fd1;
	max = (pcm > fdmax ? pcm : fdmax);

	FD_ZERO(&master);    /*  clear the master and temp sets*/
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&event_fds);

	/*  add fd to the master set */ 
	for (i =0 ; i < 2; i++) 
		FD_SET(phone_dev_fd[i], &master);

	FD_SET(pcm, &master);
	
	printf("Waiting on select %d \n",pcm);
	while(1)
	{
		read_fds = master; /* copy it */
		write_fds = master;
		event_fds = master;
		if (-1 == select(max+1, &read_fds, &write_fds, &event_fds, NULL)) 
		{
			printf("Error select()\n");
			return;
		}
			/*
			Read
			*/
			if (FD_ISSET(pcm, &read_fds))
			{
				/* read data  from SLIC */	  					
				msg_len = read(pcm, aud_buf[0], 5*BUFSIZE);										     if (msg_len <= 0) {
                       			printf("$$read() failed.$$\n");
                        		return;
				}

				memcpy(aud_buf[1], aud_buf[0] + BUFSIZE, BUFSIZE);
				memcpy(aud_buf[1] + BUFSIZE, aud_buf[0], BUFSIZE);
			}

			/*
			Write
			*/
			if (FD_ISSET(pcm, &write_fds))
			{
				msg_len = write(pcm, aud_buf[1], 5*BUFSIZE);
				if (msg_len <= 0) {
                       			printf("$$write() failed.$$\n");
                        		return;
				}
					
			}
			/*
			Events
			*/
			for(i = 0; i < 2;i++)
			{
			     /* Get file descriptor */
			     dev_fd = phone_dev_fd[i];

			   if (FD_ISSET(dev_fd, &event_fds) ) 
			   {
			  
				int ex = ioctl(dev_fd, PHONE_EXCEPTION, 0);
				if(ex > 0)
				{				
					if ((ex & 2) && (hook_state[i] == 0))
					{
							offhook_count++;
							hook_state[i] = 1;
							printf("##hook is off for %d##\n", i);
							 if(offhook_count == 2 && !session_started) {
								printf("PLAY START\n");
								ioctl(fd0,PHONE_PLAY_START);
								ioctl(fd0,PHONE_REC_START);	
							  	ioctl(fd1,PHONE_REC_START);
								ioctl(fd1,PHONE_PLAY_START);
								session_started = 1;
							}					
					}
					else if((ex & (1 << 16)) && (hook_state[i] == 1))
					{						
					   		offhook_count--;
					   		hook_state[i] = 0;
					  		printf("##hook is on for %d##\n", i);
					   		if(!offhook_count) {
								printf("PLAY STOP\n");
								ioctl(fd0,PHONE_PLAY_STOP);
								ioctl(fd0,PHONE_REC_STOP);
								ioctl(fd1,PHONE_REC_STOP);
								ioctl(fd1,PHONE_PLAY_STOP);
								session_started = 0;
				           		}
					}
			    	}
			}
		}
	}
}

/* sin table, 256 points */
static short sinTbl[] = {0,402,804,1205,1606,2005,2404,2801,3196,3590,3981,4370,4756,
5139,5519,5896,6270,6639,7005,7366,7723,8075,8423,8765,9102,9433,9759,10079,10393,
10701,11002,11297,11585,11865,12139,12405,12664,12915,13159,13394,13622,13841,14052,
14255,14449,14634,14810,14977,15136,15285,15425,15556,15678,15790,15892,15985,16068,
16142,16206,16260,16304,16339,16363,16378,16383,16378,16363,16339,16304,16260,16206,
16142,16068,15985,15892,15790,15678,15556,15425,15285,15136,14977,14810,14634,14449,
14255,14052,13841,13622,13394,13159,12915,12664,12405,12139,11865,11585,11297,11002,
10701,10393,10079,9759,9433,9102,8765,8423,8075,7723,7366,7005,6639,6270,5896,5519,
5139,4756,4370,3981,3590,3196,2801,2404,2005,1606,1205,804,402,0,-402,-804,-1205,-1606,
-2005,-2404,-2801,-3196,-3590,-3981,-4370,-4756,-5139,-5519,-5896,-6270,-6639,-7005,
-7366,-7723,-8075,-8423,-8765,-9102,-9433,-9759,-10079,-10393,-10701,-11002,-11297,
-11585,-11865,-12139,-12405,-12664,-12915,-13159,-13394,-13622,-13841,-14052,-14255,
-14449,-14634,-14810,-14977,-15136,-15285,-15425,-15556,-15678,-15790,-15892,-15985,
-16068,-16142,-16206,-16260,-16304,-16339,-16363,-16378,-16383,-16378,-16363,-16339,
-16304,-16260,-16206,-16142,-16068,-15985,-15892,-15790,-15678,-15556,-15425,-15285,
-15136,-14977,-14810,-14634,-14449,-14255,-14052,-13841,-13622,-13394,-13159,-12915,
-12664,-12405,-12139,-11865,-11585,-11297,-11002,-10701,-10393,-10079,-9759,-9433,-9102,
-8765,-8423,-8075,-7723,-7366,-7005,-6639,-6270,-5896,-5519,-5139,-4756,-4370,-3981,
-3590,-3196,-2801,-2404,-2005,-1606,-1205,-804,-402,0};
unsigned short txAudioBufs [5*(BUFSIZE/2)];
static unsigned short f1Mem = 0;
static unsigned short f2Mem = 0;
static short seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF};
static short search(short val, short *table, short size)
{
	short i;
	for (i = 0; i < size; i++) {
		if (val <= *table++)
			return (i);
	}
	return (size);
}
unsigned char linear2ulaw(short	pcm_val)
{
	short mask, seg;
	unsigned char uval;
	pcm_val = pcm_val >> 2;
	if (pcm_val < 0) {
		pcm_val = -pcm_val;
		mask = 0x7F;
	} else {
		mask = 0xFF;
	}
        if ( pcm_val > 8159 ) pcm_val = 8159;
	pcm_val += (0x84 >> 2);
	seg = search(pcm_val, seg_uend, 8);
	if (seg >= 8)
		return (unsigned char) (0x7F ^ mask);
	else {
		uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}
short ulaw2linear(unsigned char u_val)
{
	short t;
	u_val = ~u_val;
	t = ((u_val & 0xf) << 3) + 0x84;
	t <<= ((unsigned)u_val & 0x70) >> 4;
	return ((u_val & 0x80) ? (0x84 - t) : (t - 0x84));
}
static void gen_tone(unsigned short f1, unsigned short f2)
{
	short i, j;
	short buf[80];
	register short *ptr;
	short sample;

	ptr = buf;
	for(i=0; i<80; i++) {
		sample = (sinTbl[f1Mem >> 8] + sinTbl[f2Mem >> 8]) >> 2;
#ifndef CONFIG_CPU_BIG_ENDIAN 
		*ptr++ = sample;
#else
		*ptr++ = ((sample & 0xff) << 8)+ (sample >> 8);
#endif 
		f1Mem += f1;
		f2Mem += f2;
	}
	memcpy (txAudioBufs, (void *)buf, 80*2);
	memcpy (txAudioBufs+(BUFSIZE/2), (void *)buf, 80*2);
	memcpy (txAudioBufs+(2*BUFSIZE/2), (void *)buf, 80*2);
	memcpy (txAudioBufs+(3*BUFSIZE/2), (void *)buf, 80*2);
}

int sw_tone(int fd,int pcm)
{
	char wr_buff[5*BUFSIZE];
	char rd_buff[5*BUFSIZE];
	char str[32];
	unsigned int x,fail_count = 0;
	fd_set wr_fds, rd_fds, ex_fds;

	while(1) {
		printf("Take hook off before continuing.\n");
		printf("Choose frequency: (1) 300HZ (2) 630HZ (3) 1000HZ (4) Back to main menu.\n");
		gets(str);
		if(str[0] == '1') {
			x = 2457;
			printf("Generating 300HZ tone\n");
		}
		else if (str[0] == '2') {
			x = 5161;
			printf("Generating 630HZ tone\n");
		}
		else if (str[0] == '3') {
			x = 8192;
			printf("Generating 1000HZ tone\n");
		}
        	else if (str[0] == '4') {
			return 0;
	        }
		else {
			printf("Input error\n");
			return -1;
		}

		printf("Put hook on to return to menu.\n");

	        /* seems like you must 'read' before 'selcet' (???) */
        	//read(fd,rd_buff,BUFSIZE);

	        while (fail_count<5)
        	{
	                int a, ready_fd, slic_ex;

                	FD_ZERO(&wr_fds);
	                FD_ZERO(&rd_fds);
        	        FD_ZERO(&ex_fds);

			FD_SET(pcm,&wr_fds);
                        FD_SET(pcm,&rd_fds);
                        FD_SET(pcm,&ex_fds);

	//		printf("selecting...\n");
	                ready_fd = select(pcm+1,&rd_fds,&wr_fds,&ex_fds,NULL);
        	        if(ready_fd == -1) {
                       		printf("##select error##\n");
                        	return -1;
	                }
	//		printf("continue...\n");
	//               if(FD_ISSET(fd,&wr_fds)) printf("wr_fds is set\n");
	//               if(FD_ISSET(fd,&rd_fds)) printf("rd_fds is set\n");
	//               if(FD_ISSET(fd,&ex_fds)) printf("ex_fds is set\n");
		
	//		sleep(1);
	                if(FD_ISSET(pcm,&ex_fds)) {
        	                int ex;
                	        //printf("ex ioctl...\n");
                        	ex = ioctl(fd,PHONE_EXCEPTION,0);
	                        if(ex >= 0) {
        	                        //printf("##ex = 0x%08x##\n",ex);
                	                if(ex & 2) {
                        	                printf("##hook is off##\n");
						if(ioctl(fd,PHONE_PLAY_START) < 0)
							printf("error PHONE_PLAY_START\n");
						//break;
					}
                                	else {
						printf("##hook is on##\n");
						if(ioctl(fd,PHONE_PLAY_STOP) < 0)
							printf("error PHONE_PLAY_STOP\n");
						break;
					}
                        	}
	                        else {
        	                        printf("##PHONE_EXCEPTION failed##\n");
                	                fail_count++;
                        	}
	                }
        	        if(FD_ISSET(pcm,&rd_fds)) {
                	        //printf("reading...\n");
                        	a = read(pcm, rd_buff, 5*BUFSIZE);
	                        if (a < 0) {
        	                        printf("##read() failed.##\n");
                	                fail_count++;
                        	}
			}
	                if(FD_ISSET(pcm,&wr_fds)) {
				int i;
				gen_tone(x,x);
#ifndef USE_LINEAR
				/* linear to ulaw */
				for(i=0;i<80;i++)
			        	wr_buff[i] = linear2ulaw(txAudioBufs[i]);
	                       	//printf("writing...\n");
        	                a = write(pcm, wr_buff, 5*BUFSIZE);
#else
				a = write(pcm, txAudioBufs, (5*BUFSIZE));
#endif
                	        if (a <= 0) {
                                	printf("##write() failed.##\n");
                               	        fail_count++;
	                        }
        	        }
	        }
		if(fail_count == 5) return -1;
	}
	return 0;
}

void digital_loopback(int fd,int pcm)
{
        char wr_buff[5*BUFSIZE],rd_buff[5*BUFSIZE] ;
	int fail_count=0, cnt = 0;
	unsigned int i = 0, j = 0;
	
	for(i = 0; i < 160; i += 2)
		*(short*)(wr_buff+i) = (short)i;

#ifdef RECORD_DATA
	short ulaw_buff[BUFSIZE];
	char *ulaw_name = "/root/voip/tools/ulaw_log.pcm";
        int i, ulaw_fd = open(ulaw_name,O_RDWR);
        if(ulaw_fd <= 0) {
                printf("## Cannot open %s.##\n",ulaw_name);
                exit(2);
        }
#endif

	/* seems like you must 'read' before 'selcet' (???) 
        read(fd,rd_buff,BUFSIZE);
	*/

	while (fail_count<5) 
	{
		int a, slic_ex;
		fd_set wr_fds, rd_fds, ex_fds;

		FD_ZERO(&wr_fds);
		FD_ZERO(&rd_fds);
                FD_ZERO(&ex_fds);

                FD_SET(pcm,&wr_fds);
		FD_SET(pcm,&rd_fds);
		FD_SET(pcm,&ex_fds);

		//printf("selecting...\n");
		select(pcm+1,&rd_fds,&wr_fds,&ex_fds,NULL);

		//if(FD_ISSET(pcm,&wr_fds)) printf("wr_fds is set\n");
		//if(FD_ISSET(pcm,&rd_fds)) printf("rd_fds is set\n");
		//if(FD_ISSET(pcm,&ex_fds)) printf("ex_fds is set\n");

                if(FD_ISSET(pcm,&ex_fds)) {
                        int ex;
			//printf("ex ioctl...\n");
                        ex = ioctl(fd,PHONE_EXCEPTION,0);
                        if(ex >= 0) {
                                if(ex & 2) {
					printf("##hook is off##\n");
					ioctl(fd,PHONE_PLAY_START);
                                        ioctl(fd,PHONE_REC_START);
                                        //sleep(1);
				}
                                else {
					ioctl(fd,PHONE_REC_STOP);
                                        ioctl(fd,PHONE_PLAY_STOP);
					printf("##hook is on##\n");
					//sleep(1);
				}
                        }
                        else {
                                printf("##PHONE_EXCEPTION failed##\n");
                                fail_count++;
                        }
                }

		if(FD_ISSET(pcm,&rd_fds)) {
			//printf("R\n");
			a = read(pcm, rd_buff, 5*BUFSIZE);
			if (a <= 0) {
				printf("##read() failed.##\n");
				fail_count++;
			}
#ifdef RECORD_DATA
			else {
				/* linear read data and store to log file */
				for(i=0; i<a; i++) {
					ulaw_buff[i] = ulaw2linear(rd_buff[i]);
				}
				write(ulaw_fd,ulaw_buff,i*2);
			}
#endif
		}
		if(FD_ISSET(pcm,&wr_fds)) {
	//		printf("W\n");
			a = write(pcm, wr_buff, 5*BUFSIZE);
        	        if (a <= 0) {
                       		printf("##write() failed.##\n");
                        	fail_count++;
			}
			j = i;
			for(; i < j + 160; i += 2)
				*(short*)(wr_buff+(i % 160)) = (short)i;
		}
	}

_exit:
#ifdef RECORD_DATA
	close(ulaw_fd);
#endif

	return;
}


void sw_loopback(int fd,int pcm)
{
        char rd_buff[5*BUFSIZE];
	int fail_count=0, max;

#ifdef RECORD_DATA
	short ulaw_buff[BUFSIZE];
	char *ulaw_name = "/root/voip/tools/ulaw_log.pcm";
        int i, ulaw_fd = open(ulaw_name,O_RDWR);
        if(ulaw_fd <= 0) {
                printf("## Cannot open %s.##\n",ulaw_name);
                exit(2);
        }
#endif

	/* seems like you must 'read' before 'selcet' (???) 
        read(fd,rd_buff,BUFSIZE);
	*/
	max = (fd > pcm) ? fd : pcm;
	while (fail_count<5) 
	{
		int a, slic_ex;
		fd_set wr_fds, rd_fds, ex_fds;

		FD_ZERO(&wr_fds);
		FD_ZERO(&rd_fds);
                FD_ZERO(&ex_fds);

                FD_SET(pcm,&wr_fds);
		FD_SET(pcm,&rd_fds);
		FD_SET(fd,&ex_fds);

		//printf("selecting...\n");
		select(max+1,&rd_fds,&wr_fds,&ex_fds,NULL);

		if(FD_ISSET(pcm,&wr_fds)) printf("wr_fds is set\n");
		if(FD_ISSET(pcm,&rd_fds)) printf("rd_fds is set\n");
		if(FD_ISSET(fd,&ex_fds)) printf("ex_fds is set\n");

                if(FD_ISSET(fd,&ex_fds)) {
                        int ex;
			//printf("ex ioctl...\n");
                        ex = ioctl(fd,PHONE_EXCEPTION,0);
                        if(ex > 0) {
                                if(ex & 2) {
					printf("##hook is off##\n");
					ioctl(fd,PHONE_PLAY_START);
                                        ioctl(fd,PHONE_REC_START);
                                        //sleep(1);
				}
                                else if(ex & (1 << 16)){
					ioctl(fd,PHONE_REC_STOP);
                                        ioctl(fd,PHONE_PLAY_STOP);
					printf("##hook is on##\n");
					//sleep(1);
				}
                        }
                        else {
                                printf("##PHONE_EXCEPTION failed##\n");
                                fail_count++;
                        }
                }

		if(FD_ISSET(pcm,&rd_fds)) {
			//printf("R\n");
			a = read(pcm, rd_buff, 5*BUFSIZE);
			if (a <= 0) {
				printf("##read() failed.##\n");
				fail_count++;
			}
#ifdef RECORD_DATA
			else {
				/* linear read data and store to log file */
				for(i=0; i<a; i++) {
					ulaw_buff[i] = ulaw2linear(rd_buff[i]);
				}
				write(ulaw_fd,ulaw_buff,i*2);
			}
#endif
		}
		if(FD_ISSET(pcm,&wr_fds)) {
	//		printf("W\n");
			a = write(pcm, rd_buff, 5*BUFSIZE);
        	        if (a <= 0) {
                       		printf("##write() failed.##\n");
                        	fail_count++;
			}
		}
	}

_exit:
#ifdef RECORD_DATA
	close(ulaw_fd);
#endif
	return;
}

/*#define RECORD_DATA*/

void hw_loopback(int fd)
{
        char *wr_buff;/*[BUFSIZE];*/
        char rd_buff[BUFSIZE];
        int i, fail_count=0;
        char *p;

        /* init once the wr_buff */
	wr_buff = "123456789_123456789_123456789_123456789_123456789_123456789_123456789_123456789";
//        for(i=0,p=wr_buff; i<BUFSIZE; i++,p++)
//                *p = i;

        /* seems like you must 'read' before 'selcet' (???) */
        //read(fd,rd_buff,BUFSIZE);

        while (fail_count<5)
        {
                int a, ready_fd, slic_ex;
                fd_set wr_fds, rd_fds, ex_fds;

                FD_ZERO(&wr_fds);
                FD_ZERO(&rd_fds);
                FD_ZERO(&ex_fds);

                FD_SET(fd,&wr_fds);
                FD_SET(fd,&rd_fds);
                FD_SET(fd,&ex_fds);

		printf("##selecting...\n");
                ready_fd = select(fd+1,&rd_fds,&wr_fds,&ex_fds,NULL);
                if(ready_fd == -1) {
                        printf("##select error\n");
                        exit(2);
                }
		if(FD_ISSET(fd,&wr_fds)) printf("wr_fds is set\n");
		if(FD_ISSET(fd,&rd_fds)) printf("rd_fds is set\n");
		if(FD_ISSET(fd,&ex_fds)) printf("ex_fds is set\n");

                if(FD_ISSET(fd,&wr_fds) || FD_ISSET(fd,&rd_fds)) {
			/* debug */
			if(!(FD_ISSET(fd,&wr_fds) && FD_ISSET(fd,&rd_fds))) {
				printf("##read and write fds are not set together\n");
			}
			printf("##reading...\n");
                        a = read(fd, rd_buff, BUFSIZE);
                        if (a < 0) {
	                        printf("##read() failed.\n");
                                fail_count++;
                        }
                        else {
				printf("***\n");
				puts(rd_buff);
				printf("***\n");
			}
			printf("##writing...\n");
	               	a = write(fd, wr_buff, BUFSIZE);
        	        if (a < 0) {
        	              	printf("##write() failed.\n");
                	        fail_count++;
	                }
		}
                if(FD_ISSET(fd,&ex_fds)) {
                        int ex;
                        printf("##exception event\n");
                        ex = ioctl(fd,PHONE_EXCEPTION,0);
                        if(ex >= 0) {
                                if(ex & 2)
                                        printf("##hook is off\n");
                                else
                                        printf("##hook is on\n");
                        }
                        else {
                                printf("##PHONE_EXCEPTION failed\n");
                                fail_count++;
                        }
                }
        }
	exit(2);
}


