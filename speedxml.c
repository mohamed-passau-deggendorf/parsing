#define _GNU_SOURCE
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <syscall.h>
#include <string.h>
#include <sys/wait.h>

#define READING_SIZE 2048
#define BUFFER_SIZE 8192


enum SpeedXMLStateMachine {
Starting,
WaitingForQM,
WaitingForX,
WaitingForM,
WaitingForL,
InsideXMLHeader,
WaitingForClosingXMLHeader,
InsideComment,
Ready,
ReadingTextNode,
ReadingFirstTagChar,
ReadingTag,
InsideTag,
ReadingAttribute,
ReadingValue,
ReadingStringValue,
EscapingStringValue,
ReadingFirstValueCharacter,
ReadingClosingTag,
WaitForClosingCharacter,
WaitingForFirstCommentDash,
WaitingForSecoundCommentDash,
WaitingForCommentDashEnding,
WaitingForCommentCosingCharEnding
};

enum SpeedNodeType{TextNode,ElementNode};
struct SpeedAttribute {
char name[512];
char value[512];
};

struct SpeedElement  {
    char tagname[24];
    struct SpeedNode *children[24];
    unsigned long int children_count;
    struct SpeedAttribute attrs[24];
    unsigned long int attrs_count;
    };
struct SpeedNode{
enum SpeedNodeType type;
union {
struct SpeedElement *element;
char * textnode;
}node;
};


struct SpeedElementList {
    struct SpeedElement** roots;
    int root_count;
};

void rais_exception(int code){
printf("Error %d\n",code);
exit(EXIT_FAILURE);
}
inline bool isAlphaNumerical(char c){

 return c=='.' || c=='+' || c=='-' || c=='_' || c==':'
         || ( (c >='a' && c<= 'z' )||(c >='A' && c<= 'Z' )||(c >='0' && c<= '1' ) )
 && c != '<' &&  c != '>' && c!=' ';

}

inline bool isSpacingCharacter(char c) {
    return c == ' ' || c=='\n' || c=='\r' || c=='\t';
}


void speed_xml_dump_file(int fd,struct SpeedElementList *list)
{

    int c;
    struct SpeedNode *current;
    struct SpeedNode *writing_stack[2048];
    int writing_stack_top ;
    char tag_buffer[1024];




    for(c=0;c<list->root_count;c++) {
    writing_stack[0]=list->roots[c];
    writing_stack_top = 0;

    while (writing_stack_top > -1)
    {
    current=writing_stack[writing_stack_top--];
    switch(current->type) {

    case ElementNode:
             {
        char tag_buffer[1024]="<";
         int l;
        strcat(tag_buffer,current->node.element->tagname);
        if(current->node.element->attrs_count > 0)
           for(l=0;l<current->node.element->attrs_count;l++) {
               strcat(tag_buffer," ");
               strcat(tag_buffer,current->node.element->attrs[l].name);
               strcat(tag_buffer,"=");
               strcat(tag_buffer,"\"");
               strcat(tag_buffer,current->node.element->attrs[l].value);
               strcat(tag_buffer,"\"");}

               strcat(tag_buffer,">");
        write(fd,tag_buffer,strlen(tag_buffer));

        struct SpeedNode *virtual_node = syscall(SYS_mmap,NULL,sizeof( struct SpeedNode ),PROT_READ| PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
        virtual_node->type=TextNode;
        virtual_node->node.textnode= syscall(SYS_mmap,NULL,24,PROT_READ| PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
        strcpy(virtual_node->node.textnode,"</");
        strcat(virtual_node->node.textnode,current->node.element->tagname);
        strcat(virtual_node->node.textnode,">");
        writing_stack[++writing_stack_top]=virtual_node;
        int k ;
        int nchildren = current->node.element->children_count;
         for(k=nchildren-1;k>=0;k--) writing_stack[++writing_stack_top]=current->node.element->children[k];


            }

        break;


    case TextNode :
        write(fd,current->node.textnode,strlen(current->node.textnode));
        break;

    }



 }

    }

}








struct SpeedElementList* speed_xml_parse_file(int fd)
{

    struct SpeedElementList* result =
            syscall(SYS_mmap,NULL,sizeof(struct SpeedElementList),PROT_READ|PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    result->roots = syscall(SYS_mmap,NULL,4*sizeof(struct SpeedElement),PROT_READ|PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    result->root_count = 0;



char buffer[BUFFER_SIZE];
int stack_top = -1;
struct SpeedNode * stack[256];
struct SpeedNode *node;
struct SpeedNode *root;
struct SpeedNode **children;
int tag_offset = 0,attr_offset = 0;
int read_byte, offset=0 , k;


enum SpeedXMLStateMachine speed_machine=Starting,speed_history=Ready;
while( (read_byte = read(fd,buffer + offset,
                         (BUFFER_SIZE-offset)  >= READING_SIZE ? READING_SIZE :  (BUFFER_SIZE-offset)   )) >0 )

{ // DO NOT REMOVE THIS BRACKET

for(k=offset;k<read_byte + offset;k++)


switch (speed_machine) {


case ReadingTextNode :
     if(buffer[k] == '<')  { speed_history=speed_machine; speed_machine=ReadingFirstTagChar;
         node->node.textnode[tag_offset]='\0';
         tag_offset=0; node=stack[--stack_top];

     }else {
 syscall(SYS_mmap,node->node.textnode + tag_offset , 1,PROT_READ | PROT_WRITE , MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
         node->node.textnode[tag_offset++]=buffer[k];

     }
    break;

case WaitingForX: if(buffer[k]=='x' ) speed_machine=WaitingForM;  else rais_exception(5); break;
case WaitingForM: if(buffer[k]=='m' ) speed_machine=WaitingForL;  else rais_exception(6); break;
case WaitingForL: if(buffer[k]=='l' ) speed_machine=InsideXMLHeader; else rais_exception(7); break;
case InsideXMLHeader : if(buffer[k]=='?') {speed_machine=WaitingForClosingXMLHeader; } break;
case WaitingForClosingXMLHeader: if(buffer[k]=='>') speed_machine=Ready;
     else rais_exception(8); break;

case WaitingForQM: if(buffer[k]=='?') {speed_machine=WaitingForX; }
    else{ speed_history=speed_machine; speed_machine=ReadingFirstTagChar;
        goto ReadingFirstTagChar; }
    break;

case Starting :
if(buffer[k]=='<' ) { speed_machine=WaitingForQM;
} else  speed_machine=Ready; goto Ready;


break;

case Ready:
    Ready:
    if(speed_machine!=Ready) goto EndReady;

   if(buffer[k] == '<') { speed_history=speed_machine; speed_machine=ReadingFirstTagChar;  } else
   if(!isSpacingCharacter(buffer[k]))
   {


struct SpeedNode* parent = node;
stack[++stack_top]=node=syscall(SYS_mmap,0,sizeof(struct SpeedNode),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);

if(stack_top > 0) {
++parent->node.element->children_count;
   
parent->node.element->children[parent->node.element->children_count -1]=node; }
else result->roots[result->root_count++]=node;

node->type=TextNode;
node->node.textnode = syscall(SYS_mmap,0,1,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
       speed_machine=ReadingTextNode  ;
   node->node.textnode[0]=buffer[k];
   tag_offset=1;
   }
EndReady : {}
break;

case WaitingForFirstCommentDash: if(buffer[k]=='-')speed_machine=WaitingForSecoundCommentDash;
    else rais_exception(4);break;

case WaitingForSecoundCommentDash:  if(buffer[k]=='-') speed_machine=InsideComment;
    else rais_exception(5);break;

case InsideComment: if(buffer[k]=='-') speed_machine=WaitingForCommentDashEnding;     break;
case WaitingForCommentDashEnding : if(buffer[k]=='-') speed_machine=WaitingForCommentCosingCharEnding; else speed_machine=InsideComment; break;
case WaitingForCommentCosingCharEnding:  if(buffer[k]=='>') { speed_machine=speed_history;  } else speed_machine=InsideComment; break;


case ReadingFirstTagChar :
    ReadingFirstTagChar:
    if(buffer[k]=='!')  speed_machine=WaitingForFirstCommentDash; else
    if(isAlphaNumerical(buffer[k])) {
struct SpeedNode *parent = node;
node = stack[++stack_top] =syscall(SYS_mmap,0,1,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);

if(stack_top>0){
        ++parent->node.element->children_count;

    parent->node.element->children[parent->node.element->children_count-1]=node;

}else result->roots[result->root_count++]=node;

node->type = ElementNode;
node->node.element = syscall(SYS_mmap,0,sizeof(struct SpeedElement),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
node->node.element->children_count=0;
node->node.element->attrs_count=0;



node->node.element->tagname[0]=buffer[k];
tag_offset=1;
speed_machine=ReadingTag;
    }else if(buffer[k] == '>') speed_machine=Ready;
else if(buffer[k] == '/') {speed_machine = ReadingClosingTag; tag_offset=0; } else rais_exception(0);
break;

case ReadingClosingTag :
if(buffer[k] == '>') {tag_offset=0;speed_machine=Ready; node = stack[--stack_top];
}else
    if(node->node.element->tagname[tag_offset] != buffer[k]) {

rais_exception(2); }
    else
tag_offset++;
    break;

case ReadingTag :


    if( isSpacingCharacter(buffer[k]) ) {speed_machine=InsideTag;
    node->node.element->tagname[tag_offset++]='\0'; tag_offset=0;}
    else if(buffer[k] == '/') {
        node->node.element->tagname[tag_offset++]='\0';  tag_offset=0;
        speed_machine = WaitForClosingCharacter; }
    else if(buffer[k] == '>') {speed_machine=Ready;
         node->node.element->tagname[tag_offset++]='\0'; tag_offset=0;}
    else if(isAlphaNumerical(buffer[k])){

        node->node.element;
        node->node.element->tagname[tag_offset++]=buffer[k];
    }
 break;
case InsideTag :

    if( isAlphaNumerical(buffer[k]) ) {
    speed_machine=ReadingAttribute;
     node = stack[stack_top];

     ++node->node.element->attrs_count;
   
    attr_offset = 1;

    node->node.element->attrs[node->node.element->attrs_count-1].name[0]=buffer[k];


    } else if(buffer[k] == '>') speed_machine=Ready;
    else if(buffer[k] == '/') speed_machine = WaitForClosingCharacter;


 break;


case WaitForClosingCharacter :
    if(buffer[k] != '>') rais_exception(1);else
    {node = stack[--stack_top]; speed_machine=Ready; tag_offset=0;}

   break;
case ReadingAttribute : if( buffer[k]=='=' ) {
        speed_machine=ReadingFirstValueCharacter; attr_offset = 0;}
    else if(buffer[k] == ' '){ speed_machine=InsideTag;
    stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].name[attr_offset+1]='\0';
     attr_offset = 0;
                }
  else {
        stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].name[attr_offset++]=buffer[k];
        stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].value[attr_offset+1]='\0';

} break;

case EscapingStringValue:
if( buffer[k]=='\"' || buffer[k]=='\\'  ) speed_machine = ReadingStringValue;

stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].name[attr_offset++]=buffer[k];
stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].value[attr_offset+1]='\0';

break;

case ReadingStringValue :
if( buffer[k]=='\"' ) {speed_machine = InsideTag; attr_offset=0;

}else
if( buffer[k]=='\\' ) speed_machine = EscapingStringValue;
else
  {  stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].value[attr_offset++]=buffer[k];
      stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].value[attr_offset+1]='\0';
}

   break;
case ReadingFirstValueCharacter :
if( buffer[k]==' ' ) { speed_machine = InsideTag; attr_offset=0;
} else if( buffer[k]=='\"' ) speed_machine = ReadingStringValue;
else {stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count - 1].value[0]=buffer[k]; attr_offset=1; speed_machine=ReadingValue;}

    break;
case ReadingValue : if( isSpacingCharacter(buffer[k]) ) {
        printf("Found Attribulte %s = %s \n",stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].name
                ,stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].value);
        speed_machine = InsideTag; attr_offset=0;
    }
        else if(buffer[k]=='>') {
                attr_offset=0;
                speed_machine= Ready;}
 else  
    stack[stack_top]->node.element->attrs[stack[stack_top]->node.element->attrs_count-1].value[attr_offset++]=buffer[k];
break;



}





if(offset + read_byte  <= BUFFER_SIZE) offset += read_byte; else offset = 0;
}




return result;
}

int main(int argc,char* argv[]) {
if(argc< 3) exit(EXIT_FAILURE);



int fd0,fd1;

speed_xml_dump_file(fd1 = open(argv[1],O_WRONLY |  O_TRUNC | O_CREAT,0444),
                    speed_xml_parse_file(fd0 = open(argv[2],O_RDONLY)));

close(fd0);
close(fd1);

exit(EXIT_SUCCESS);
}
