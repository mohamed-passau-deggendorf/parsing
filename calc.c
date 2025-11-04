/*
IMPORTANT : this program is not finished and will be finished later !!!!

*/
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/mman.h> 
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <sys/statfs.h>

#define BUF_SIZE 8192
#define READING_SIZE 2048
#define BUFFER_SIZE 8192



void raise_exception(){
printf("Syntax Error \n");
exit(EXIT_FAILURE);
}


enum OperatorPriority{
PLUS_MINUS,
MUL_DIV

};
bool is_operator(char op){return op == '+' ||  op == '-' || op == '*' ||  op == '/';}
enum OperatorPriority get_priority(char op){
if(op == '+' ||  op == '-') return PLUS_MINUS;
if(op == '*' ||  op == '/') return MUL_DIV;

}

int main(int argc,char*argv[]){

char buffer[BUF_SIZE];
printf("Expr :: "); scanf("%s",buffer);
int k ;
enum NodeType{
Value,
Parenthese,
Operator,

};

struct OperatorNode{
struct Node* operands[3];
enum OperatorPriority priority;
};

struct Node{
enum NodeType type;
union {
double value;
struct Node* parenthese;
struct OperatorNode operator;
};
};
struct Node* shadow_stack[2048];
struct Node* operator_stack[2048];
int shadow_stack_top=-1;
int operator_stack_top=-1;

struct Node* current_operator=NULL;

enum State{
Ready,
ReadingFirstOperand,
ReadingSecondOperand,


};

enum State state;
struct Node *root=malloc(sizeof (struct Node));
struct Node *current=root;
char vbuffer[1024];
int voffset=0;

buffer[-1]=1;
for(k=0;buffer[k-1]!='\0';k++) 
switch(state)
{
case Ready:
if(buffer[k]<='9' && buffer[k]>='0')
{ current->type=Value; vbuffer[0]=buffer[k];voffset=1; state=ReadingFirstOperand; }
else if(buffer[k] == '(') {
  current->type=Parenthese;
  shadow_stack[++shadow_stack_top]=current;
  current->parenthese=malloc(sizeof (struct Node));
  current=current->parenthese;			

}else raise_exception();


break;
case ReadingFirstOperand:
if(buffer[k]<='9' && buffer[k]>='0') {vbuffer[voffset++]=buffer[k];}else
if(is_operator(buffer[k]) ){
	struct Node *op = malloc(sizeof(struct Node));
	op->operator.priority=get_priority(buffer[k]);
	if(current_operator == NULL){
		op->operator.operands[0]=current;
		op->operator.operands[2]=NULL;
		current_operator=op;
		}else if(current_operator->operator.priority < op->operator.priority) {
			op->operator.operands[0]=current_operator->operator.operands[0];
			current_operator->operator.operands[0]->operator.operands[2]=op->operator.operands[0];
			current_operator->operator.operands[0]=op;
			op->operator.operands[2]=current_operator;



			}else { 
			struct Node *ptr=current_operator;
			while(ptr!=NULL&& ptr->operator.priority >= op->operator.priority)ptr=ptr->operator.operands[2] ;
			
			 }
			state=ReadingSecondOperand;



}else if(buffer[k]=='\0') printf("Bye !!! \n");


break;


		case ReadingSecondOperand:
		if(buffer[k]<='9' && buffer[k]>='0'){
		current=malloc(sizeof (struct Node));
		current->type=Value; vbuffer[0]=buffer[k];voffset=1; state=ReadingFirstOperand;
		current_operator->operator.operands[1]=current;


		}else raise_exception();

		break;

}



return EXIT_SUCCESS;
}
