#include <stdio.h>

#define DEBUG

char str[256];

/*************************************************************************************/

void show_help(void) {
  printf("\n-----------\n");
  printf("rr reg       - register read, used to read a register\n");
  printf("rw reg value - register write, used to write a value to a register\n");
  printf("md offset    - memory dump, used to dump 128 bytes in memory\n");
  printf("mw reg value - memory write, used to write a value to a physical memory\n");
#if defined (CONFIG_MV_ETHERNET) || defined (CONFIG_MV_GATEWAY)
  printf("sr dev_addr reg - smi register read, used to read a Switch or Phy register\n");
  printf("sw dev_addr reg value - smi register write, used to write a 16-bit value to a Switch or Phy register\n");
#endif
#ifdef CONFIG_MV_CPU_PERF_CNTRS
  printf("cchelp\n");
  printf("tcc cc0_type cc1_type cc2_type cc3_type - trigger cpu counter capture \n");
  printf("dcc - display cpu counters \n");
#endif
#ifdef CONFIG_MV_CPU_PERF_CNTRS
  printf("l2help\n");
  printf("tl2 l20_type l21_type - trigger cpu l2 counter capture \n");
  printf("dl2 - display cpu l2 counters \n");
#endif

  printf("quit - exit from tool\n");
  printf("-----------\n");
}

void show_counters_help(void) {
    int i = 1;
    printf("cpu counter type can be one of the below, make sure no error is recieved\n");
    printf("for example: tcc 11 2 1 4\n");
    printf("%d: MV_CPU_CNTRS_CYCLES\n",i++);
    printf("%d: MV_CPU_CNTRS_ICACHE_READ_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_ACCESS\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_READ_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_READ_HIT\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_WRITE_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_WRITE_HIT\n",i++);
    printf("%d: MV_CPU_CNTRS_DTLB_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_TLB_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_ITLB_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_INSTRUCTIONS\n",i++);
    printf("%d: MV_CPU_CNTRS_SINGLE_ISSUE\n",i++);
    printf("%d: MV_CPU_CNTRS_MMU_READ_LATENCY\n",i++);
    printf("%d: MV_CPU_CNTRS_MMU_READ_BEAT\n",i++);
    printf("%d: MV_CPU_CNTRS_BRANCH_RETIRED\n",i++);
    printf("%d: MV_CPU_CNTRS_BRANCH_TAKEN\n",i++);
    printf("%d: MV_CPU_CNTRS_BRANCH_PREDICT_MISS\n",i++);
    printf("%d: MV_CPU_CNTRS_BRANCH_PREDICT_COUNT\n",i++);
    printf("%d: MV_CPU_CNTRS_WB_FULL_CYCLES\n",i++);
    printf("%d: MV_CPU_CNTRS_WB_WRITE_LATENCY\n",i++);
    printf("%d: MV_CPU_CNTRS_WB_WRITE_BEAT\n",i++);
    printf("%d: MV_CPU_CNTRS_ICACHE_READ_LATENCY\n",i++);
    printf("%d: MV_CPU_CNTRS_ICACHE_READ_BEAT\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_READ_LATENCY\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_READ_BEAT\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_WRITE_LATENCY\n",i++);
    printf("%d: MV_CPU_CNTRS_DCACHE_WRITE_BEAT\n",i++);
    printf("%d: MV_CPU_CNTRS_LDM_STM_HOLD\n",i++);
    printf("%d: MV_CPU_CNTRS_IS_HOLD\n",i++);
    printf("%d: MV_CPU_CNTRS_DATA_WRITE_ACCESS\n",i++);
    printf("%d: MV_CPU_CNTRS_DATA_READ_ACCESS\n",i++);
    printf("%d: MV_CPU_CNTRS_BIU_SIMULT_ACCESS\n",i++);
    printf("%d: MV_CPU_CNTRS_BIU_ANY_ACCESS\n",i++);
}

void show_l2_counters_help(void) {
    int i = 1;
    printf("cpu l2 counter type can be one of the below, make sure no error is recieved\n");
    printf("%d: MV_CPU_L2_CNTRS_DATA_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_DATA_MISS_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_INST_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_INST_MISS_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_DATA_READ_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_DATA_READ_MISS_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_DATA_WRITE_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_DATA_WRITE_MISS_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_RESERVED\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_DIRTY_EVICT_REQ\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_EVICT_BUFF_STALL\n",i++);
    printf("%d: MV_CPU_L2_CNTRS_ACTIVE_CYCLES\n",i++);
}
/*************************************************************************************/

void gt_reg_read(unsigned int rgst) {
  unsigned int reg = rgst, value;
  char ch;
  FILE *resource_dump,*output;

  output = fopen ("./xxx.out","w");
  if (!output) { printf ("Cannot open file\n");return;}

  while (1){ 
    resource_dump = fopen ("/proc/resource_dump" , "w");
    if (!resource_dump) {
      printf ("Error opening file /proc/resource_dump\n");
      exit(-1);
    }
    fprintf (resource_dump,"register  r %08x",reg);
    fclose (resource_dump);
    resource_dump = fopen ("/proc/resource_dump" , "r");
    if (!resource_dump) {
      printf ("Error opening file /proc/resource_dump\n");
      exit(-1);
    }
    fscanf (resource_dump , "%x" , &value);
    fclose (resource_dump);
    printf ("%08x : %08x ",reg,value);

    if (!fgets (str, 255, stdin)) {
     printf ("Error in reading line from stdin\n");
      exit (-1);
    }
    if (str[0] == '.') break;
    reg += 4;
  
  }
}

/*************************************************************************************/
#if defined (CONFIG_MV_ETHERNET) || defined (CONFIG_MV_GATEWAY)

void gt_smi_reg_read(unsigned int dev_addr, unsigned int rgst) {
  unsigned int reg = rgst, value;
  char ch;
  FILE *resource_dump,*output;

  output = fopen ("./xxx.out","w");
  if (!output) { printf ("Cannot open file\n");return;}

  while (1){ 
    resource_dump = fopen ("/proc/resource_dump" , "w");
    if (!resource_dump) {
      printf ("Error opening file /proc/resource_dump\n");
      exit(-1);
    }
    fprintf (resource_dump,"smi  r %08x %08x", dev_addr, reg);
    fclose (resource_dump);
    resource_dump = fopen ("/proc/resource_dump" , "r");
    if (!resource_dump) {
      printf ("Error opening file /proc/resource_dump\n");
      exit(-1);
    }
    fscanf (resource_dump , "%x" , &value);
    fclose (resource_dump);
    printf ("SMI Device: %08x, Reg: %08x, Data: %08x ", dev_addr, reg, value);

    if (!fgets (str, 255, stdin)) {
     printf ("Error in reading line from stdin\n");
      exit (-1);
    }
    if (str[0] == '.') break;
    reg += 1;
  
  }
}

/*************************************************************************************/

void gt_smi_reg_write(unsigned int dev_addr, unsigned int rgst, unsigned int vlue) {
  unsigned int reg = rgst, value = vlue;
  unsigned int element;
  char ch;
  FILE *resource_dump;
  resource_dump = fopen ("/proc/resource_dump" , "w");
  if (!resource_dump) {
    printf ("Eror opening file /proc/resource_dump\n");
    exit(-1);
  }
  fprintf (resource_dump,"smi  w %08x %08x %08x", dev_addr, reg, value);
  fclose (resource_dump);
}
#endif
/*************************************************************************************/

void gt_mem_dump(unsigned int rgst) {
  unsigned int offset = rgst, value , i , j;
  char ch;
  FILE *resource_dump;
  FILE *output;
  output = fopen ("./xxx.out","w");
  if (!output) { printf ("Canot open file\n");return;}
  i = 0;
  while (1) {
    if (i == 0) printf ("\n");
    printf ("%08x : ",offset);
    for (j = 0 ; j < 8 ; j++) {
      resource_dump = fopen ("/proc/resource_dump" , "w");
      if (!resource_dump) {
	printf ("Eror opening file /proc/resource_dump\n");
	exit(-1);
      }

      fprintf (resource_dump,"memory    r %08x",offset);
      fclose (resource_dump);
      resource_dump = fopen ("/proc/resource_dump" , "r");
      if (!resource_dump) {
	printf ("Eror opening file /proc/resource_dump\n");
	exit(-1);
      }
      fscanf (resource_dump , "%x" , &value);
      fclose (resource_dump);
            printf ("%08x ",value);
      
      {
	unsigned int temp;
	temp = ((value & 0xff) << 24) |
	  ((value & 0xff00) << 8) |
	  ((value & 0xff0000) >> 8) |
	  ((value & 0xff000000) >> 24);
	value = temp;
      }
      fprintf (output, "%c",value & 0xff);
      value = value >> 8;
      fprintf (output, "%c",value & 0xff);
      value = value >> 8;
      fprintf (output, "%c",value & 0xff);
      value = value >> 8;
      fprintf (output, "%c",value & 0xff);
      value = value >> 8;

      offset += 4;
      }
      if (!fgets (str, 255, stdin)) {
	printf ("Error in reading line from stdin\n");
	exit (-1);
      }
      if (str[0] == '.') break;
      i ++;

  }
  fclose (output);
  printf ("\n");
}

/*************************************************************************************/

void gt_mem_write(unsigned int rgst, unsigned int vlue) {
  unsigned int reg = rgst, value = vlue;
  unsigned int element;
  char ch;
  FILE *resource_dump;
  resource_dump = fopen ("/proc/resource_dump" , "w");
  if (!resource_dump) {
    printf ("Eror opening file /proc/resource_dump\n");
    exit(-1);
  }
  fprintf (resource_dump,"memory    w %08x %08x",reg,value);
  fclose (resource_dump);
}

/*************************************************************************************/

void gt_reg_write(unsigned int rgst, unsigned int vlue) {
  unsigned int reg = rgst, value = vlue;
  unsigned int element;
  char ch;
  FILE *resource_dump;
  resource_dump = fopen ("/proc/resource_dump" , "w");
  if (!resource_dump) {
    printf ("Eror opening file /proc/resource_dump\n");
    exit(-1);
  }
  fprintf (resource_dump,"register  w %08x %08x",reg,value);
  fclose (resource_dump);
}

/*************************************************************************************/

int main(void) {
  unsigned int dev, reg , value , offset , element;
  char inst[256];
  char value_s[10];
  printf ("\n\n\n*******\nWelcome to the GT Shell environment\n\n");
  printf ("Write 'help' for getting help on the instructions\n");
  while (1) {
    memset (str , 0 , 256);
    memset (inst , 0 , 256);
    printf ("MV Shell -> ");
    if (!fgets (str, 255, stdin)) {
      printf ("Error in reading line from stdin\n");
      exit (-1);
    }
    element = sscanf (str , "%s" , inst);
    if (element == 0) continue;
    offset = strlen (inst);
    if (!strcmp (inst , "quit")) break;
    
    if (!strcmp (inst , "rr")) {
      element = sscanf (str+offset , "%x" , &reg);
      if (element == 1) gt_reg_read (reg);
      else printf ("Insufficient parameters\n");
    }
    else if (!strcmp (inst , "rw")) {
      element = sscanf (str+offset , "%x %x" , &reg , &value);
      if (element == 2) gt_reg_write (reg,value);
      else printf ("Insufficient parameters\n");
    }
    else if (!strcmp (inst , "help")) {
      show_help();
    }
    else if (!strcmp (inst , "md")) {
      element = sscanf (str+offset , "%x" , &reg);
      if (element == 1) gt_mem_dump (reg);
      else printf ("Insufficient parameters\n");
    }
    else if (!strcmp (inst , "mw")) {
      element = sscanf (str+offset , "%x %x" , &reg , &value);
      if (element == 2) gt_mem_write (reg,value);
      else printf ("Insufficient parameters\n");
    }
#if defined (CONFIG_MV_ETHERNET) || defined (CONFIG_MV_GATEWAY)
    else if (!strcmp (inst , "sr")) {
      element = sscanf (str+offset , "%x %x %x" , &dev, &reg);
      if (element == 2) gt_smi_reg_read (dev, reg);
      else printf ("Insufficient parameters\n");
    }
    else if (!strcmp (inst , "sw")) {
      element = sscanf (str+offset , "%x %x %x" , &dev, &reg , &value);
      if (element == 3) gt_smi_reg_write (dev, reg, value);
      else printf ("Insufficient parameters\n");
    }
#endif
#ifdef CONFIG_MV_CPU_PERF_CNTRS
   else if (!strcmp (inst , "cchelp")) {
	show_counters_help();
   }
   else if (!strcmp (inst , "tcc")) {
	int cc0, cc1, cc2, cc3;
  	FILE *resource_dump;
  	resource_dump = fopen ("/proc/resource_dump" , "w");
  	if (!resource_dump) {
    		printf ("Eror opening file /proc/resource_dump\n");
    		exit(-1);
  	}
	sscanf(str + 3," %d %d %d %d", &cc0, &cc1, &cc2, &cc3);
  	fprintf (resource_dump,"start_cc %d %d %d %d",cc0, cc1, cc2, cc3);
  	fclose (resource_dump);
   }
   else if (!strcmp (inst , "dcc")) {
  	FILE *resource_dump;
  	resource_dump = fopen ("/proc/resource_dump" , "w");
  	if (!resource_dump) {
    		printf ("Eror opening file /proc/resource_dump\n");
    		exit(-1);
  	}
  	fprintf (resource_dump,"show__cc");
  	fclose (resource_dump);
   }
#endif
#ifdef CONFIG_MV_CPU_PERF_CNTRS
   else if (!strcmp (inst , "l2help")) {
	show_l2_counters_help();
   }
   else if (!strcmp (inst , "tl2")) {
	int l20, l21;
  	FILE *resource_dump;
  	resource_dump = fopen ("/proc/resource_dump" , "w");
  	if (!resource_dump) {
    		printf ("Eror opening file /proc/resource_dump\n");
    		exit(-1);
  	}
	sscanf(str + 3," %d %d", &l20, &l21);
  	fprintf (resource_dump,"start_l2 %d %d",l20, l21);
  	fclose (resource_dump);
   }
   else if (!strcmp (inst , "dl2")) {
  	FILE *resource_dump;
  	resource_dump = fopen ("/proc/resource_dump" , "w");
  	if (!resource_dump) {
    		printf ("Eror opening file /proc/resource_dump\n");
    		exit(-1);
  	}
  	fprintf (resource_dump,"show__l2");
  	fclose (resource_dump);
   }
#endif

    else if (strlen (str) != 0) printf ("Invalid command - %s\n",inst);
  }
  printf ("Good Bye\n");
  
}

/*************************************************************************************/

