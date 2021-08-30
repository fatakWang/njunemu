#include "nemu.h"
#include<stdio.h>
#include<string.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ,PLUS,REG,DECNUM,HEXNUM,NOTEQ,
	AND,OR,BIGEQ,SMAEQ,DEREF,NEGATIVE
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE,0},				// spaces
	{"\\+", PLUS,4},				// plus
	{"==", EQ,3},					// equal
	{"\\$[a-z]{2,3}",REG,8},
	{"[0-9]+",DECNUM,8},
	{"0(x|X)[0-9a-fA-F]+",HEXNUM,8},
	{"!=",NOTEQ,3},
	{"&&",AND,2},
	{"\\|\\|",OR,1},
	{"!",'!',6},
	{"\\(",'(',7},
	{"\\)",')',7},	
	{"-",'-',4},
	{"\\*",'*',5},
	{"/",'/',5},	
	{"@",DEREF,6},
	{"`",NEGATIVE,6}//@ and ` means the DEREF AND NEGATIVE wouldn't be declared
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	int priority;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;
				if(substr_len>32){
printf("the token is larger than 32\n");
assert(0);}
				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
case NOTYPE:break;					
default:
if(nr_token>32){printf("the expression have too much token\n");assert(0);}
tokens[nr_token].type=rules[i].token_type;		
strncpy(tokens[nr_token].str,substr_start,substr_len+2);			
tokens[nr_token].priority=rules[i].priority;
nr_token++;
break;		
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

bool check_parenttheses(int p,int q){
int i=0,j=0;
bool result=true;
for(i=p;i<q;i++){
if(tokens[i].type=='('){
j++;
}
else if(tokens[i].type==')'){
j--;
}
if(j==0)result=false;
if(j<0){
printf("the ( cannot match ),check it out\n");
assert(0);
}
}
if(tokens[i].type=='('){
j++;
}
else if(tokens[i].type==')'){
j--;
}
if(j==0&&result!=false)result=true;
else if(j!=0){
printf("the ( cannot match ),check it out\n");
assert(0);
}
else{
}
return result;
}//THE TECHNIC SMILIAR TO STACK

int findDomi(int p,int q){
int i=p,minPrior=8,j=p;
for(i=p;i<=q;i++){
if(tokens[i].type=='('){
for(;tokens[i].type!=')';i++);
i++;
}
if(minPrior>=tokens[i].priority){
minPrior=tokens[i].priority;
j=i;
}
}
return j;
}

uint32_t eval(int p,int q){
if(p>q){
assert(0);
}
else if(p==q){
unsigned int	returnNUM;
int i=0;
switch(tokens[p].type){
case DECNUM:sscanf(tokens[p].str,"%d",&returnNUM);return returnNUM;break;
case HEXNUM:sscanf(tokens[p].str,"%x",&returnNUM);return returnNUM;break;
case REG:
for(i=0;i<8;i++){
if(strcmp(tokens[p].str+1,regsl[i])==0){
return reg_l(i); 
}
}
for(i=0;i<8;i++){
if(strcmp(tokens[p].str+1,regsw[i])==0){
return reg_w(i); 
}
}
for(i=0;i<8;i++){
if(strcmp(tokens[p].str+1,regsb[i])==0){
return reg_b(i); 
}
}
break;
default:
printf("please enter the right expression\n");
assert(0);
break;
}
}
else if(check_parenttheses(p,q)==true){
return eval(p+1,q-1);
}
else{
uint32_t returnNUM=0;
int op=findDomi(p,q);
if(tokens[p].type=='!'||tokens[p].type==DEREF||tokens[p].type==NEGATIVE){
if(tokens[p+1].type=='('||tokens[p+1].type=='!'||tokens[p+1].type==DEREF||tokens[p+1].type==NEGATIVE){
returnNUM=eval(p+1,q);
switch(tokens[p].type){
case '!':return !returnNUM;break;
case NEGATIVE:return -returnNUM;break;
case DEREF:return swaddr_read(returnNUM,4);break;
}//switch
}//if p+1
else if(tokens[p+1].type==DECNUM||tokens[p+1].type==HEXNUM||tokens[p+1].type==REG){
if(p+1==q){
returnNUM=eval(p+1,q);
switch(tokens[p].type){
case '!':return !returnNUM;break;
case NEGATIVE:return -returnNUM;break;
case DEREF:return swaddr_read(returnNUM,4);break;
}//switch
}//if p+1==q
else{
if(p+3>q){
printf("please enter a right expression\n");
assert(0);
}
switch(tokens[p+2].type){
case PLUS:return eval(p,p+1)+eval(p+3,q);break;
case '-':return eval(p,p+1)-eval(p+3,q);break;
case '*':return eval(p,p+1)*eval(p+3,q);break;
case '/':return eval(p,p+1)/eval(p+3,q);break;
case EQ:return eval(p,p+1)==eval(p+3,q);break;
case NOTEQ:return eval(p,p+1)!=eval(p+3,q);break;
case AND:return eval(p,p+1)&&eval(p+3,q);break;
case OR:return eval(p,p+1)||eval(p+3,q);break;
default:printf("please enter a right expression");
assert(0);
}//switch p+2
}//p+1!=q
}//elseif p+1
else{
printf("please enter a right expression\n");
assert(0);
}//else p+1
}//sma if

else{
uint32_t val1=eval(p,op-1);
uint32_t val2=eval(op+1,q);
switch(tokens[op].type){
case PLUS:return val1+val2;break;
case '-':return val1-val2;break;
case '*':return val1*val2;break;
case '/':return val1/val2;break;
case EQ:return val1==val2;break;
case NOTEQ:return val1!=val2;break;
case AND:return val1&&val2;break;
case OR:return val1||val2;break;
}//switch 
}//sma else
}//big else
return 0;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
int i=0;
for(i=0;i<nr_token;i++){
if(tokens[i].type=='*'&&(i==0||(tokens[i-1].type!=DECNUM&&
tokens[i-1].type!=HEXNUM&&tokens[i-1].type!=REG))){
tokens[i].type=DEREF;
}
}
for(i=0;i<nr_token;i++){
if(tokens[i].type=='-'&&(i==0||(tokens[i-1].type!=DECNUM&&
tokens[i-1].type!=HEXNUM&&tokens[i-1].type!=REG))){
tokens[i].type=NEGATIVE;
}
}
	return eval(0,nr_token-1);
}

