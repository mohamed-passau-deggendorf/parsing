#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>


#define READING_SIZE 2048
#define BUFFER_SIZE 8192
#define NUM_BUFFER_SIZE 1024
#define JSON_ATTR_CAPACITY 8
#define JSON_ARRAY_CAPACITY 8



enum SpeedJSONStateMachine {
OutSide,
Ready,
ReadyForArray,
ReadingAttributeName,
ReadingAttributeStringName,
WaitingForColumn,
WaitingForValue,
ReadingAttributeValueBoolTrue,
ReadingAttributeValueBoolTrue1,
ReadingAttributeValueBoolTrue2,
ReadingAttributeValueBoolFalse,
ReadingAttributeValueBoolFalse1,
ReadingAttributeValueBoolFalse2,
ReadingAttributeValueBoolFalse3,
ReadingAttributeValueBoolFalse4,
ReadingAttributeValueBoolFalse5,
ReadingAttributeValueInt,
ReadingAttributeValueDec,
ReadingAttributeValueString,
EscapingAttributeValueString,
ReadingArrayElementBool,
ReadingArrayElementInt,
ReadingArrayElementDec,
ReadingArrayElementString,
EscapingArrayElementString,
DessisionCommaClosingBracket,
DessisionCommaClosingSquareBracket,
WaitingForComma,
WaitingForCommaArray,
WaitingForCommaOutside


};

enum SpeedValueType {
BoolValue,
IntValue,
DoubleValue,
StringValue,
ObjectValue,
ArrayValue,


Ending,
ArrayEnding,
Comma

};

struct SpeedJSONValue {
enum SpeedValueType type;
union {
bool boolvalue;
int intvalue;
double doublevalue;
struct SpeedJSONObject *objectvalue;
struct SpeedJSONArray *arrayvalue;
char *stringvalue;

}value;

};

struct SpeedJSONValue;
struct SpeedJSONAttribute {
char name[64];
struct SpeedJSONValue value;

};
struct SpeedJSONObject {
struct SpeedJSONAttribute* attrs;
unsigned int attrs_count;
unsigned int attrs_capacity;
};

struct SpeedJSONArray {
struct SpeedJSONValue *array;
unsigned int count;
unsigned int array_capacity;

};




void rais_exception(int code){
printf("Error %d\n",code);
exit(EXIT_FAILURE);
}

static inline bool isAlphaNumerical(char c){

 return c=='.' || c=='+' || c=='-' || c=='_' 
         || ( (c >='a' && c<= 'z' )||(c >='A' && c<= 'Z' )||(c >='0' && c<= '9' ) )
 && c != '<' &&  c != '>' && c!=' ';

}
static inline bool isNumerical(char c){
 return c=='+' || c=='-' ||  (c >='0' && c<= '9' );

}

static inline bool isDigit(char c) {
return  c >='0' && c<= '9' ;
}

static inline bool isSpacingCharacter(char c) {
    return c == ' ' || c=='\n' || c=='\r' || c=='\t';
}

int main(int argc,char * argv[]) {
if(argc< 2) exit(EXIT_FAILURE);
int fd = open(argv[1],O_RDONLY);
char buffer[BUFFER_SIZE];
int stack_top = -1;
struct SpeedJSONValue * stack[256];
enum SpeedJSONStateMachine speed_machine=OutSide;
int read_byte, offset=0 , k;
struct SpeedJSONValue *speed_current;
int attribute_offset=0;
double double_offset=1;
bool negative=false;
struct SpeedJSONValue * roots[JSON_ATTR_CAPACITY];
int root_cout = 0;

char num_buffer[NUM_BUFFER_SIZE];
int num_buffer_offset = 0;

while( (read_byte = read(fd,buffer + offset,
                         (BUFFER_SIZE-offset)  >= READING_SIZE ? READING_SIZE :  (BUFFER_SIZE-offset)   )) >0 ) {
    for(k=offset;k<read_byte + offset;k++) {
    switch (speed_machine) {
        case OutSide :
       if(buffer[k]=='{')  {speed_machine=Ready; 
       roots[++root_cout] = stack[++stack_top]=speed_current=malloc(sizeof(struct SpeedJSONValue));
       speed_current->type=ObjectValue;
       speed_current->value.objectvalue = malloc(sizeof (struct SpeedJSONObject));

       speed_current->value.objectvalue->attrs = malloc(JSON_ATTR_CAPACITY*sizeof (struct SpeedJSONAttribute));
        speed_current->value.objectvalue->attrs_count=0;
        speed_current->value.objectvalue->attrs_capacity=JSON_ATTR_CAPACITY;
            }
       else if(buffer[k]=='[') {
                speed_machine=ReadyForArray;
                (roots[++root_cout] = stack[++stack_top]=speed_current=malloc(sizeof(struct SpeedJSONValue)))->type=ArrayValue;
                speed_current->value.arrayvalue = malloc(sizeof(struct SpeedJSONArray));
                speed_current->value.arrayvalue->array = malloc(JSON_ARRAY_CAPACITY * sizeof(struct SpeedJSONValue));
                speed_current->value.arrayvalue->count=0;
                  speed_current->value.arrayvalue->array_capacity = JSON_ARRAY_CAPACITY;
                    }

       else if(!isSpacingCharacter(buffer[k])) rais_exception(0);
        break;

    case WaitingForCommaOutside:
        if(buffer[k] == ',') speed_machine=OutSide;
        else if(!isSpacingCharacter(buffer[k]))   rais_exception(55); 
        break;


    case Ready:
        if(buffer[k] == '\"') {

          struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;

            speed_machine=ReadingAttributeStringName;


            ++current_object->attrs_count;
           if(current_object->attrs == NULL)
               exit(400);
           attribute_offset=0;

        }else if(isAlphaNumerical(buffer[k])) {
            struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;


            ++current_object->attrs_count;
           if(current_object->attrs == NULL)
               exit(400);
            current_object->attrs[current_object->attrs_count-1].name[0]=buffer[k];
             attribute_offset=1;




            speed_machine=ReadingAttributeName;


        } else if(buffer[k] == '}') {

                stack_top--;
            speed_current=stack[stack_top];
            if(stack_top > -1)
            if(speed_current->type==ObjectValue) speed_machine=Ready;else
                if(speed_current->type == ArrayValue)
                    speed_machine=ReadyForArray;else;else//the two elses are needed do not remove one of them
                speed_machine=WaitingForCommaOutside;




      }else if(!isSpacingCharacter(buffer[k])) rais_exception(7);
           
    break;

    case ReadyForArray :
        ;



        if(isNumerical(buffer[k])) {
            struct SpeedJSONArray *current_array=stack[stack_top]->value.arrayvalue;
            speed_machine=ReadingArrayElementInt;


            current_array->count++;
            if(current_array->count >= current_array->array_capacity) {
                current_array->array_capacity*=2;
                current_array->array=realloc(current_array->array,sizeof(struct SpeedJSONValue)*current_array->array_capacity);
            }

            current_array->array[current_array->count -1].value.intvalue=buffer[k]-48;
            current_array->array[current_array->count - 1].type=IntValue;


        }else if(buffer[k]=='{'){


            speed_current=stack[++stack_top]=malloc(sizeof(struct SpeedJSONValue));
            struct SpeedJSONValue *speed_parent = stack[stack_top-1];
            speed_current->type=ObjectValue;
            stack[stack_top]->value.objectvalue=malloc(JSON_ATTR_CAPACITY*sizeof(struct SpeedJSONObject));
            stack[stack_top]->value.objectvalue->attrs_count=0;
            stack[stack_top]->value.objectvalue->attrs_capacity=JSON_ATTR_CAPACITY;

            stack[stack_top]->value.objectvalue->attrs = malloc(JSON_ATTR_CAPACITY * sizeof(struct SpeedJSONAttribute));
            if( stack[stack_top]->value.objectvalue->attrs == NULL)  exit(200);

             speed_parent->value.arrayvalue->count++;
             if(speed_parent->value.arrayvalue->count >= speed_parent->value.arrayvalue->array_capacity) {
                 speed_parent->value.arrayvalue->array_capacity*=2;
                 speed_parent->value.arrayvalue->array=realloc(speed_parent->value.arrayvalue->array,sizeof(struct SpeedJSONValue)*speed_parent->value.arrayvalue->array_capacity);
             }
            speed_parent->value.arrayvalue->array[speed_parent->value.arrayvalue->count-1].type=ObjectValue;
            speed_parent->value.arrayvalue->array[speed_parent->value.arrayvalue->count-1].value.objectvalue= stack[stack_top]->value.objectvalue;

            speed_machine=Ready;

        } else if(buffer[k]=='[') { 
            struct SpeedJSONValue *speed_parent = stack[stack_top];
            speed_current=stack[++stack_top]=malloc(sizeof(struct SpeedJSONValue));

            speed_parent->value.arrayvalue->count++;
            if(speed_parent->value.arrayvalue->count >= speed_parent->value.arrayvalue->array_capacity) {
                speed_parent->value.arrayvalue->array_capacity*=2;
                speed_parent->value.arrayvalue->array=realloc(speed_parent->value.arrayvalue->array,sizeof(struct SpeedJSONValue)*speed_parent->value.arrayvalue->array_capacity);
            }


            speed_current->type=ArrayValue;
            speed_current->value.arrayvalue=malloc(sizeof(struct SpeedJSONArray));
            speed_current->value.arrayvalue->count=0;
            speed_current->value.arrayvalue->array = malloc(JSON_ARRAY_CAPACITY * sizeof(struct SpeedJSONValue));
             speed_current->value.arrayvalue->array_capacity=JSON_ARRAY_CAPACITY;


            speed_parent->value.arrayvalue->array[speed_parent->value.arrayvalue->count-1].type=ArrayValue;
            speed_parent->value.arrayvalue->array[speed_parent->value.arrayvalue->count-1].value.arrayvalue= stack[stack_top]->value.arrayvalue;



             speed_machine=ReadyForArray;

        }else if(buffer[k]==']'){
            stack_top--;

            if(stack_top > -1) {
            speed_current=stack[stack_top];
         if(speed_current->type == ObjectValue)    speed_machine=DessisionCommaClosingBracket;else
             if(speed_current->type == ArrayValue) speed_machine=DessisionCommaClosingSquareBracket;

            }else speed_machine=WaitingForCommaOutside;

        }else if(buffer[k]=='\"') {
            struct SpeedJSONArray *current_array=stack[stack_top]->value.arrayvalue;

            current_array->count++;
            if(current_array->count >= current_array->array_capacity) {
                current_array->array_capacity*=2;
                current_array->array=realloc(current_array->array,sizeof(struct SpeedJSONValue)*current_array->array_capacity);
            }

            struct SpeedJSONValue *speed_array_value=&current_array[current_array->count-1];
            speed_machine=ReadingArrayElementString;
        speed_array_value->type=StringValue;
        speed_array_value->value.stringvalue=malloc(1);

        attribute_offset=0; }
        else rais_exception(12);

        break;

    case ReadingArrayElementInt:
         if(isNumerical(buffer[k])){
          struct SpeedJSONArray *current_array=stack[stack_top]->value.arrayvalue;
            int x = current_array->array[current_array->count-1].value.intvalue;
          current_array->array[current_array->count-1].value.intvalue = 10*x+(int)buffer[k]-48;


         } else if(buffer[k]=='.'){
              struct SpeedJSONArray *current_array=stack[stack_top]->value.arrayvalue;
              double x = (double)current_array->array[current_array->count-1].value.intvalue;
              current_array->array[current_array->count-1].value.doublevalue=x;
               current_array->array[current_array->count-1].type=DoubleValue;
               double_offset = 1;
                speed_machine=ReadingArrayElementDec;


         }

         else if(buffer[k]==',') speed_machine=ReadyForArray;
         else if(buffer[k] == ']') {

                stack_top--;
                if(stack_top > -1) {
                if(stack[stack_top]->type == ObjectValue) speed_machine=WaitingForComma;else
                    if(stack[stack_top]->type == ArrayValue) speed_machine=WaitingForCommaArray;
                    }else speed_machine=WaitingForCommaOutside;
         }
        else if(isSpacingCharacter(buffer[k])) speed_machine=DessisionCommaClosingSquareBracket;
            else rais_exception(8);

        break;

     case ReadingArrayElementDec:

        if(isDigit(buffer[k])){
            double x = stack[stack_top]->value.arrayvalue->array[stack[stack_top]->value.arrayvalue->count-1].value.doublevalue;
              double_offset=double_offset*0.1;
               x=x+double_offset*((double)(buffer[k]-48));



            stack[stack_top]->value.arrayvalue->array[stack[stack_top]->value.arrayvalue->count-1].value.doublevalue=x;
        }else if(isSpacingCharacter(buffer[k])) speed_machine = DessisionCommaClosingSquareBracket;
        else if(buffer[k]==',') {
            speed_machine=ReadyForArray;
            double_offset=1;

                 }
        else  if(buffer[k]==']') {
            double_offset=1;

                stack_top--;
                if(stack_top > -1) {
                speed_current=stack[stack_top];
                if(speed_current->type=ObjectValue) speed_machine=DessisionCommaClosingBracket;else
                if(speed_current->type=ArrayValue) speed_machine=DessisionCommaClosingSquareBracket;

                }else speed_machine=WaitingForCommaOutside;
        }


        else rais_exception(10);


        break;

      case DessisionCommaClosingSquareBracket:
        if(buffer[k]==']') {
         if(stack_top > 0)
             stack_top--;
             speed_current=stack[stack_top];
             if(stack[stack_top]->type == ArrayValue ) speed_machine=WaitingForCommaArray;else
                 if(stack[stack_top]->type == ObjectValue) speed_machine=WaitingForComma;

        }else if(buffer[k]==',') speed_machine=ReadyForArray;

        else rais_exception(12);



        break;



    case ReadingAttributeName:

         if(buffer[k]==':') speed_machine=WaitingForValue; else
        if(isAlphaNumerical(buffer[k])){

            struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;
             stack[stack_top]->value.objectvalue->attrs[stack[stack_top]->value.objectvalue->attrs_count-1].name[attribute_offset++]=buffer[k];


        }else rais_exception(2);

        break;

    case ReadingAttributeStringName:
       if(buffer[k]=='\"'){
        speed_machine=WaitingForColumn;
       }else if(isAlphaNumerical(buffer[k])){

           struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;
            current_object->attrs[current_object->attrs_count-1].name[attribute_offset++]=buffer[k];
       }




        break;


    case WaitingForColumn:

if(buffer[k]==':') speed_machine=WaitingForValue; else rais_exception(1);
       break;



    case WaitingForValue:
            ;//DO NOT REMOVE THE SEMICOLUMN
        struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;
        struct SpeedJSONValue speed_value ;



         if(current_object->attrs_count >= current_object->attrs_capacity)
         {
             current_object->attrs_capacity*=4;
           current_object->attrs = realloc(current_object->attrs, current_object->attrs_capacity*sizeof(struct SpeedJSONAttribute));
                if(current_object->attrs == NULL) exit(33);

         }

        if(buffer[k]== 't' ){
            speed_value.type=BoolValue;
            speed_value.value.boolvalue=true;
         speed_machine=ReadingAttributeValueBoolTrue;
         current_object->attrs[current_object->attrs_count-1].value=speed_value;

        }else if(buffer[k]=='f') {
            speed_value.type=BoolValue;
            speed_value.value.boolvalue=false;
        speed_machine=ReadingAttributeValueBoolFalse;
        current_object->attrs[current_object->attrs_count-1].value=speed_value;

        }else
        if(buffer[k]=='\"') {
            speed_machine=ReadingAttributeValueString;
        speed_value.type=StringValue;
        speed_value.value.stringvalue=malloc(1);
        current_object->attrs[current_object->attrs_count-1].value=speed_value;
        attribute_offset=0; }else if(isNumerical(buffer[k])) {
            speed_machine=ReadingAttributeValueInt;
            speed_value.type=IntValue;
            num_buffer_offset=0;
            if(buffer[k]=='-') negative=true;
            else if(buffer[k]=='+') negative=false;
            else {
                negative=false;
                speed_value.value.intvalue=(int)buffer[k]-48;
            current_object->attrs[current_object->attrs_count-1].value=speed_value;

                            }


        } else if(buffer[k]=='{')  {


            speed_current=stack[++stack_top]=malloc(sizeof(struct SpeedJSONValue));
            struct SpeedJSONValue *speed_parent = stack[stack_top-1];
            speed_current->type=ObjectValue;
            stack[stack_top]->value.objectvalue=malloc(sizeof(struct SpeedJSONObject));
            stack[stack_top]->value.objectvalue->attrs_count=0;
            stack[stack_top]->value.objectvalue->attrs_capacity=JSON_ATTR_CAPACITY;

            stack[stack_top]->value.objectvalue->attrs = malloc(JSON_ATTR_CAPACITY*sizeof(struct SpeedJSONAttribute));

            speed_parent->value.objectvalue->attrs[speed_parent->value.objectvalue->attrs_count-1].value.type=ObjectValue;
            speed_parent->value.objectvalue->attrs[speed_parent->value.objectvalue->attrs_count-1].value.value.objectvalue= stack[stack_top]->value.objectvalue;
            speed_machine=Ready;

        } else if(buffer[k]=='[') {
            speed_machine=ReadyForArray;



            speed_current=stack[++stack_top]=malloc(sizeof(struct SpeedJSONValue));
            struct SpeedJSONValue *speed_parent = stack[stack_top-1];
            speed_current->type=ArrayValue;
            speed_current->value.arrayvalue=malloc(sizeof(struct SpeedJSONArray));
            speed_current->value.arrayvalue->count=0;
            speed_current->value.arrayvalue->array_capacity=JSON_ARRAY_CAPACITY;
            speed_current->value.arrayvalue->array = malloc(JSON_ARRAY_CAPACITY* sizeof(struct SpeedJSONValue));

            speed_parent->value.objectvalue->attrs[speed_parent->value.objectvalue->attrs_count-1].value.type=ArrayValue;
            speed_parent->value.objectvalue->attrs[speed_parent->value.objectvalue->attrs_count-1].value.value.arrayvalue= stack[stack_top]->value.arrayvalue;




        }else if(!isSpacingCharacter(buffer[k]))  rais_exception(4);


        break;

    case ReadingAttributeValueBoolTrue:
    if(buffer[k]=='r') speed_machine=ReadingAttributeValueBoolTrue1;else rais_exception(62);
        break;

    case ReadingAttributeValueBoolTrue1:
    if(buffer[k]=='u') speed_machine=ReadingAttributeValueBoolTrue2;else rais_exception(63);
        break;

    case ReadingAttributeValueBoolTrue2:
    if(buffer[k]=='e') speed_machine= DessisionCommaClosingBracket; else rais_exception(64);
        break;

    case ReadingAttributeValueBoolFalse:
         if(buffer[k]=='a') speed_machine=ReadingAttributeValueBoolFalse1;else rais_exception(65);
         break;

    case ReadingAttributeValueBoolFalse1:
         if(buffer[k]=='l') speed_machine=ReadingAttributeValueBoolFalse2;else rais_exception(66);
         break;
    case ReadingAttributeValueBoolFalse2:
         if(buffer[k]=='s') speed_machine=ReadingAttributeValueBoolFalse3;else rais_exception(66);
         break;
    case ReadingAttributeValueBoolFalse3:
         if(buffer[k]=='e') speed_machine=DessisionCommaClosingBracket;else rais_exception(66);
         break;






    case ReadingAttributeValueInt:
        ;
        current_object =  stack[stack_top]->value.objectvalue;
        struct SpeedJSONValue *speed_value_ptr = &(current_object->attrs + current_object->attrs_count-1)->value;
        speed_value_ptr = &current_object->attrs[current_object->attrs_count-1].value;
        if(isNumerical(buffer[k])) {
             speed_value_ptr->value.intvalue*=10;
            speed_value_ptr->value.intvalue+=((int)(buffer[k]-48));
        }
        else if(buffer[k]=='.') {
            double x = (double)speed_value_ptr->value.intvalue;
             double_offset=1;
             current_object = stack[stack_top]->value.objectvalue;
             current_object->attrs[current_object->attrs_count-1].value.type=DoubleValue;

            speed_value_ptr->value.doublevalue = x;
            speed_machine=ReadingAttributeValueDec;
        } else if(buffer[k]==',') {
            current_object = stack[stack_top]->value.objectvalue;
            speed_machine=Ready;



                 }
        else if(isSpacingCharacter(buffer[k])) speed_machine=DessisionCommaClosingBracket;
        else  if(buffer[k]=='}') {


                stack_top--; //DO NOT REMOVE THE BRACKETS SURROUNDING THE "IF"
            if(stack_top > -1) {
                speed_current=stack[stack_top];
         if(speed_current->type == ObjectValue)    speed_machine=Ready;else
             if(speed_current->type == ArrayValue) speed_machine=ReadyForArray; }
                else speed_machine=WaitingForCommaOutside;

        }
        else rais_exception(5);

        break;

    case ReadingAttributeValueDec:
        if(isDigit(buffer[k])){
            double x = stack[stack_top]->value.objectvalue->attrs[stack[stack_top]->value.objectvalue->attrs_count-1].value.value.doublevalue;
               double_offset=double_offset*0.1;


                x=x+double_offset*((double)(buffer[k]-48));



            stack[stack_top]->value.objectvalue->attrs[stack[stack_top]->value.objectvalue->attrs_count-1].value.value.doublevalue=x;
        }else if(isSpacingCharacter(buffer[k])) speed_machine = DessisionCommaClosingBracket;
        else if(buffer[k]==',') {
            speed_machine=Ready;
            double_offset=1;

                 }
        else  if(buffer[k]=='}') {
            double_offset=1;

                stack_top--;

                if(stack_top > -1) {
                speed_current=stack[stack_top];

             if(speed_current->type == ObjectValue)    speed_machine=Ready;else
                 if(speed_current->type == ArrayValue) speed_machine=ReadyForArray;
                }else speed_machine=WaitingForCommaOutside;

        }


        else  rais_exception(6);


        break;




      case WaitingForComma:
        WaitingForComma :

 
        if(buffer[k]==',') speed_machine=Ready;else
       rais_exception(30);
        break;

    case WaitingForCommaArray:
        if(buffer[k]==',') speed_machine=ReadyForArray;else
       rais_exception(32); 
        break;

    case ReadingAttributeValueString:
     if(buffer[k]=='\"' ) {
        attribute_offset=0;
        speed_machine=DessisionCommaClosingBracket;
     }else  if(buffer[k]=='\\' ) {
        speed_machine=EscapingAttributeValueString;

     } else{
        struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;
        struct SpeedJSONValue *speed_value  =   &(current_object->attrs + current_object->attrs_count-1)->value;
        speed_value->value.stringvalue[attribute_offset++]=buffer[k];
        speed_value->value.stringvalue[attribute_offset]='\0';

     }
        break;


    case EscapingAttributeValueString:
         if(buffer[k]=='\"' ) {
             struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;
             struct SpeedJSONValue *speed_value  =  & current_object->attrs[current_object->attrs_count-1].value;
             speed_value->value.stringvalue[attribute_offset++]='\"';
         }else if(buffer[k]=='\\' ) {
             struct SpeedJSONObject *current_object =  stack[stack_top]->value.objectvalue;
             struct SpeedJSONValue *speed_value  =   & (current_object->attrs[current_object->attrs_count-1].value);
             speed_value->value.stringvalue[attribute_offset++]='\\';
         }
        break;

     case DessisionCommaClosingBracket:
        if(buffer[k]=='}') {

             stack_top--;

             if(stack_top > -1) {
             speed_current=stack[stack_top];
             if(stack[stack_top]->type == ArrayValue ) speed_machine=ReadyForArray;else
                 if(stack[stack_top]->type == ObjectValue) speed_machine=Ready; }
                else speed_machine=WaitingForCommaOutside;

        }else if(buffer[k]==',') speed_machine=Ready;

        else if(!isSpacingCharacter(buffer[k])) rais_exception(3);

        break;



    } }
    if(offset + read_byte  <= BUFFER_SIZE)
    offset += read_byte; else offset = 0; }




    int fd1 = open("speedout.json",O_WRONLY);
    struct SpeedJSONAttribute *writing_stack[256];
    int writing_stack_top = -1;
        for(k=1;k<=root_cout;k++) {
           struct SpeedJSONValue *speed = roots[k];

     
           struct SpeedJSONAttribute* root_attr=malloc(sizeof(struct SpeedJSONAttribute));
           root_attr->value=*speed;



               strcpy(root_attr->name,"");
               writing_stack[++writing_stack_top]=root_attr;




           while(writing_stack_top!=-1){

             struct SpeedJSONAttribute *current = writing_stack[writing_stack_top--];

             if(current->value.type==ObjectValue) {

                 printf("%s:{",current->name);

                 struct SpeedJSONAttribute *virtual_attribute=malloc(sizeof(struct SpeedJSONAttribute));
                 strcpy(virtual_attribute->name,"");//just for safety
                 virtual_attribute->value.type=Ending;
                 writing_stack[++writing_stack_top]=virtual_attribute;

                 int l;
                 if(current->value.value.objectvalue->attrs_count > 0) {
                     writing_stack[++writing_stack_top]= &current->value.value.objectvalue->attrs[current->value.value.objectvalue->attrs_count-1];

                 for(l=current->value.value.objectvalue->attrs_count-2;l>=0;l--)
                      {
                        virtual_attribute=malloc(sizeof(struct SpeedJSONAttribute));
                        strcpy(virtual_attribute->name,"");//just for safety
                        virtual_attribute->value.type=Comma;
                        writing_stack[++writing_stack_top]=virtual_attribute;

                     writing_stack[++writing_stack_top]= &current->value.value.objectvalue->attrs[l]; }

                                }

             }
             else  if(current->value.type==ArrayValue){

                         printf("%s(%d):[",current->name,current->value.value.arrayvalue->count);
                         struct SpeedJSONAttribute *virtual_attribute=malloc(sizeof(struct SpeedJSONAttribute));
                         struct SpeedJSONAttribute *array_attribute;
                         strcpy(virtual_attribute->name,"");//just for safety
                         virtual_attribute->value.type=ArrayEnding;
                         writing_stack[++writing_stack_top]=virtual_attribute;
                         int l;

                         if(current->value.value.arrayvalue->count > 0) {
                              array_attribute = malloc(sizeof(struct SpeedJSONAttribute));
                              array_attribute->value = current->value.value.arrayvalue->array[current->value.value.arrayvalue->count - 1 ];
                              array_attribute->name[0]='\0';
                              writing_stack[++writing_stack_top]= array_attribute;
                            for(l=current->value.value.arrayvalue->count-2;l>=0;l--){

                                virtual_attribute=malloc(sizeof(struct SpeedJSONAttribute));
                                strcpy(virtual_attribute->name,"");//just for safety
                                virtual_attribute->value.type=Comma;
                                writing_stack[++writing_stack_top]=virtual_attribute;
                                array_attribute = malloc(sizeof(struct SpeedJSONAttribute));
                                array_attribute->name[0]='\0';
                                array_attribute->value=current->value.value.arrayvalue->array[l];
                                writing_stack[++writing_stack_top]=array_attribute;



                            }



                         }


             }

             else if(current->value.type==Ending) printf("}");
              else if(current->value.type==ArrayEnding) printf("]");
              else if(current->value.type==Comma) printf(",");



             else
                 if(current->name[0] != '\0') {
                 if (current->value.type == IntValue) printf("%s:i%d",current->name,current->value.value.intvalue);
             else if (current->value.type == DoubleValue) printf("%s:f%f",current->name,current->value.value.doublevalue);
             else if (current->value.type == StringValue) printf("%s:\"%s\"",current->name,current->value.value.stringvalue);
             else if (current->value.type == BoolValue) printf("%s:b%d",current->name,current->value.value.boolvalue);
                 else printf("%s<???> %d",current->name,current->value.type);

            }else {

                     if (current->value.type == IntValue) printf("i%d",current->value.value.intvalue);
                 else if (current->value.type == DoubleValue) printf("f%f",current->value.value.doublevalue);
                 else if (current->value.type == StringValue) printf("\"%s\"",current->value.value.stringvalue);
                 else if (current->value.type == BoolValue) printf("b%d",current->value.value.boolvalue);
                     else printf("-- <???> %d",current->value.type);


                 }



           }


        }

        printf("\n");

        exit(EXIT_SUCCESS);
}











