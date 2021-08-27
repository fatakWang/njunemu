#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_si(char *args){
//use args for argument
if(args==NULL){
cpu_exec(1);
return 0;
}
char* p=args;
int i=0,n=0,j=0,k=1;
for( ;*p!='\0';p++){
if(*p<'0'||*p>'9'){
printf("please enter a vaild number\n");
return 0;
}//if
else {
i++;
}
}//for
n=0;
for(p=args;*p!='\0';p++){
for(j=0,k=1;j<i-1;j++){k*=10;}
n+=(*p-'0'+0)*k;
i--;
}
//calculate n

cpu_exec(n);
return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_info(char *args){
if(*args=='r'){
printf("eax=  %08x\n",cpu.eax);
printf("ecx=  %08x\n",cpu.ecx);
printf("edx=  %08x\n",cpu.edx);
printf("ebx=  %08x\n",cpu.ebx);
printf("esp=  %08x\n",cpu.esp);
printf("ebp=  %08x\n",cpu.ebp);
printf("esi=  %08x\n",cpu.esi);
printf("edi=  %08x\n",cpu.edi);
}

return 0;
}

static int cmd_x(char *args){
char* p=args;
int i=0;
for( ;*p!=' ';p++){
if(*p<'0'||*p>'9'){
printf("please enter a vaild argument\n");
return 0;
}//if
else {
i++;
}
}//for
int n1=0,j=0,k=0;
for(p=args;*p!=' ';p++){
for(j=0,k=1;j<i-1;j++){k*=10;}
n1+=(*p-'0'+0)*k;
i--;
}
//calculate the first argument as n1

//text the second argument start as 0x
char *endptr;
uint32_t n2=strtoul(++p,&endptr,16);
/*if(endptr!=NULL){
printf("please enter a vaild argument\n");
return 0;
}*/
//calculate the second ar


int i2=0;
for(i2=0;i2<n1;i2++){
printf("%06x :  0x%08x\n",n2,swaddr_read(n2,4));
n2+=4;
}

return 0;

}




static int cmd_help(char *args);

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{"si","execute the program step by step or execute it as you will",cmd_si},
	{"info","printf the register's status",cmd_info},
	{"x","printf the memory's status",cmd_x},

	/* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
