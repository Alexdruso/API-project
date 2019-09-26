/*inclusion part*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*definition part*/
#define BEGIN -1
#define ADDENT 1
#define DELENT 2
#define ADDREL 3
#define DELREL 4
#define REPORT 5
#define END 0
#define MAGIC_NUMBER 1009
#define MAGIC_NUMBER_2 109
#define BUFFER_SIZE 40
#define UNSUCCESSFUL -1
/*data structures definition part*/
typedef struct ent{
    char*name;
    struct rel*rel;
    struct ent*next;
}ent;

typedef struct rel_pointer{
    struct ent* pointed_ent;
    struct rel_pointer*next;
}rel_pointer;

typedef struct rel{
    char*name;
    unsigned int rel_number; /*necessario a verificare il max*/
    struct rel*next;
    struct rel_pointer**hash_table;
}rel;

typedef struct max_rel{
    char*rel_name;
    unsigned int number;
    unsigned int array_lenght;
    char**ent_max;
    struct max_rel* next;
}max_rel;

//variabili globali
max_rel* max=NULL;
/*function definition part*/




/*funzioni essenziali*/

rel*find_rel(ent*entity, char*rel_name){
    rel*buffer=entity->rel;
    int comparison=-1;

    while(buffer!=NULL&&((comparison=strcmp(buffer->name,rel_name))<0)){
        buffer=buffer->next;
    }

    if(comparison==0) return buffer;

    return NULL;

    /*while(buffer!=NULL){
         if(strcmp(buffer->name,rel_name)==0){
             return buffer;
         }
         buffer=buffer->next;
     }
     return buffer;*/
}

ent* find_and_destroy(rel_pointer**hash_entry, char* destination){
    rel_pointer*buffer=*(hash_entry);
    rel_pointer*previous=*(hash_entry);
    ent*destination_ent;

    if(buffer!=NULL&&strcmp(buffer->pointed_ent->name,destination)==0){
        *hash_entry=buffer->next;
        destination_ent=buffer->pointed_ent;
        free(buffer);
        return destination_ent;
    }

    while(buffer!=NULL&&strcmp(buffer->pointed_ent->name,destination)<0){
        previous=buffer;
        buffer=buffer->next;
    }

    if(buffer==NULL||strcmp(buffer->pointed_ent->name,destination)>0) return NULL;

    previous->next=buffer->next;
    destination_ent=buffer->pointed_ent;
    free(buffer);

    return destination_ent;
}

/*funzioni per la gestione dei massimi*/

int binary_search(char**array,char*entity, unsigned int dimension){
    int start=0;
    int end=(int)dimension-1;
    int mid=0;
    int comparison;

    while(start<=end){
        mid=(start+end)>>1;

        comparison=strcmp(array[mid],entity);

        if(comparison<0){
            start=mid+1;
        }
        else if(comparison>0){
            end=mid-1;
        }
        else return mid;
    }

    return UNSUCCESSFUL;
}

int pop_from_max(max_rel* max_struct,char*entity){

    unsigned int limit=max_struct->array_lenght;

    char**array=max_struct->ent_max;

    int index=binary_search(array,entity, limit);

    if(index!=UNSUCCESSFUL){

        limit--;
        max_struct->array_lenght=limit;

        //printf("Elimino %s dai massimi, la lunghezza dell'array è ora %d\n", entity,limit);

        //printf("L'indice è %d\n", index);

        for(int i=index;i<(int)limit;i++){
            array[i]=array[i+1];
        }

        if(max_struct->array_lenght>0){
            max_struct->ent_max=realloc(array,limit* sizeof(char*));
            //report();
            return 0;
        }
        else{
            free(array);
            max_struct->ent_max=NULL;
            max_struct->number=0;
            //report();
            return 1;
        }


    }

}

max_rel* find_rel_in_max(char*rel_name){
    max_rel*buffer=max;
    int comparison=-1;

    while(buffer!=NULL&&((comparison=strcmp(buffer->rel_name,rel_name))<0)){
        buffer=buffer->next;
    }

    if(comparison==0) return buffer;

    return NULL;

    /*while(buffer!=NULL){
        if(strcmp(buffer->rel_name,rel_name)==0){
            return buffer;
        }
        buffer=buffer->next;
    }
    return buffer;*/
}

max_rel* find_max_rel(char*rel_name){
    max_rel* current=max;
    max_rel*buffer;

    if((current==NULL)||(strcmp(current->rel_name,rel_name)>0)){
        current=malloc(sizeof(max_rel));
        current->rel_name=rel_name;
        current->number=0;
        current->array_lenght=0;
        current->ent_max=NULL;
        current->next=max;
        max=current;
        return current;
    }
    else{

        if(strcmp(current->rel_name,rel_name)==0){
            free(rel_name);
            return current;
        }

        while(current->next!=NULL&&(strcmp(current->next->rel_name,rel_name)<0)){
            current=current->next;
        }

        if(current->next!=NULL){
            if(strcmp(current->next->rel_name,rel_name)==0) {
                free(rel_name);
                return current->next;
            }
        }

        buffer=malloc(sizeof(max_rel));
        buffer->rel_name=rel_name;
        buffer->number=0;
        buffer->array_lenght=0;
        buffer->ent_max=NULL;
        buffer->next=current->next;
        current->next=buffer;
        return buffer;
    }
}

int find_spot(char*name, char**array, int start, int end){
    //linearizzata

    int mid=0;
    int comparison;

    while(start<end){
        mid=(start+end)>>1;

        comparison=strcmp(array[mid],name);

        if(comparison<0){
            start=mid+1;
        }
        else if(comparison>0){
            end=mid-1;
        }
        else return mid;
    }

    if (start==end) {
        if(strcmp(array[start],name)>0) {
            return start;
        }
        else {
            return start + 1;
        }
    }

    if(start>end) return start;

    //ricorsiva

    /*int mid = (start+end)>>1;
    int i=strcmp(array[mid],name);
    if(i<0) return find_spot(name, array, mid+1, end);
    else if(i>0) return find_spot(name, array, start, mid-1);
    else return mid;*/
}

void add_to_max(ent*new_max, max_rel* max_struct){

        int index = find_spot(new_max->name, max_struct->ent_max, 0, (int) max_struct->array_lenght - 1);

        max_struct->array_lenght++;

        max_struct->ent_max = realloc(max_struct->ent_max, max_struct->array_lenght * sizeof(char *));

        char **array = max_struct->ent_max;

        char *buffer1 = new_max->name;

        char *buffer2;

        int limit = (int) max_struct->array_lenght;

        for (; index < limit; index++) {
            buffer2 = array[index];
            array[index] = buffer1;
            buffer1 = buffer2;
        }

        //printf("L'array ora è lungo %d\n", max_struct->array_lenght);

        //report();
}

void found_new_max(ent*new_max, max_rel*max_struct, int new_number){
    max_struct->ent_max=realloc(max_struct->ent_max,sizeof(char*));
    max_struct->ent_max[0]=new_max->name;
    max_struct->number=new_number;
    max_struct->array_lenght=1;
}

void refresh_max(max_rel*max_struct,ent**hash_table,char*rel_name){
    ent*temp=NULL;

    for(int i=0;i<MAGIC_NUMBER;i++){
        temp=hash_table[i];
        while(temp!=NULL){
            rel*rel_temp=find_rel(temp,rel_name);

            if(rel_temp!=NULL&&rel_temp->rel_number>0){
                if(rel_temp->rel_number==max_struct->number){
                    //printf("Inserisco %s tra i massimi\n",temp->name);
                    add_to_max(temp,max_struct);
                }
                else if(rel_temp->rel_number>max_struct->number) {
                    //printf("%s è il nuovo massimo\n", temp->name);
                    found_new_max(temp,max_struct,(int)rel_temp->rel_number);
                }
            }

            temp=temp->next;
        }
    }

}

/*funzioni dedicate all'input*/
int get_command(){
    char command_line[7];
    int i=scanf("%s", command_line);

    if(command_line[0]=='a'){
        if(command_line[3]=='e') return ADDENT;
        else return ADDREL;
    }
    else if(command_line[0]=='d'){
        if(command_line[3]=='e') return DELENT;
        else return DELREL;
    }
    else if(command_line[0]=='r') return REPORT;
    else return END;
   /* if (strcmp(command_line,"addent")==0) return ADDENT;
    if (strcmp(command_line,"delent")==0) return DELENT;
    if (strcmp(command_line,"addrel")==0) return ADDREL;
    if (strcmp(command_line,"delrel")==0) return DELREL;
    if (strcmp(command_line,"report")==0) return REPORT;
    if (strcmp(command_line,"end")==0) return END;
    */
    return i;
}

char* get_name(){
    char buffer[BUFFER_SIZE];
    int i=0;

    while((buffer[i]=(char)getchar())!='"');

    i++;

    while((buffer[i]=(char)getchar())!='"') {
        i++;
    }

    i++;

    buffer[i]='\0';

    i++;

    char*name=malloc(i* sizeof(char));

    for (int j = 0; j < i; ++j) {
        name[j]=buffer[j];
    }

    return name;

    //char*name;
    //name=malloc(BUFFER_SIZE* sizeof(char));
    //int i=scanf("%ms",&name);
    //scanf("%s",name);
    return name;
}

char* get_name2(char*buffer){
    int i=0;

    while((buffer[i]=(char)getchar())!='"');

    i++;

    while((buffer[i]=(char)getchar())!='"') {
        i++;
    }

    i++;

    buffer[i]='\0';

    return buffer;
}

/*funzione di hash*/

unsigned int hash(const char* name){
    int i=0;
    unsigned int hash=0;
    unsigned int temp=0;

    while(name[i]!='\0'){
        for(int j=0;j<sizeof(int)&&name[i]!='\0';j++) {
            temp = temp << 8*sizeof(char);
            temp = temp | (unsigned int) name[i];
            i++;
        }
        hash=hash+temp;
    }
    return (unsigned int)hash;
}

//funzioni per gestire la report

/*report*/

int report(){
    max_rel*temp=max;

    char** array;
    int lol=0;

    while(temp!=NULL){
        if(temp->number!=0) lol++;
        temp=temp->next;
    }

    if(lol==0){
        fputs("none\n",stdout);
        return 0;
    }

    temp=max;

    while(temp!=NULL){
        unsigned int limit=temp->array_lenght;
        if(limit!=0) {
            fputs(temp->rel_name,stdout);
            fputc(' ',stdout);
            array=temp->ent_max;
            for(unsigned int i=0;i<limit;i++){
                fputs(array[i],stdout);
                fputc(' ',stdout);
            }
            /*char number[12];
            sprintf(number,"%d",temp->number);
            fputs(number,stdout);*/
            printf("%d",temp->number);
            //printf("l'array era lungo %d",limit);
            fputc(';',stdout);
            lol--;
            if(lol!=0) fputc(' ',stdout);
        }
        temp=temp->next;
    }
    fputc('\n',stdout);
    return 0;
}

/*addent*/
void insert_ent(ent** hash_entry, char*ent_name){
    ent* current=*hash_entry;
    ent*buffer;

    if((current==NULL)||(strcmp(current->name,ent_name)>0)){
        current=malloc(sizeof(ent));
        current->name=ent_name;
        current->rel=NULL;
        current->next=*hash_entry;
        *hash_entry=current;
        return;
    }
    else{

        if(strcmp(current->name,ent_name)==0){
            free(ent_name);
            return;
        }

        while(current->next!=NULL&&(strcmp(current->next->name,ent_name)<0)){
            current=current->next;
        }

        if(current->next!=NULL){
            if(strcmp(current->next->name,ent_name)==0) {
                free(ent_name);
                return;
            }
        }

        buffer=malloc(sizeof(ent));
        buffer->name=ent_name;
        buffer->rel=NULL;
        buffer->next=current->next;
        current->next=buffer;
        return;
    }
}

int addent(ent**hash_table){
    unsigned int index;
    char*ent_name=get_name();


    index=hash(ent_name)%MAGIC_NUMBER;
    insert_ent(&hash_table[index], ent_name);

    return 0;
}

/*delent*/

ent* pop_ent(ent**hash_entry,char*ent_name){
    ent*buffer=*(hash_entry);
    ent*previous=*(hash_entry);

    if(buffer!=NULL&&strcmp(buffer->name,ent_name)==0){
        *hash_entry=buffer->next;
        return buffer;
    }

    while(buffer!=NULL&&strcmp(buffer->name,ent_name)<0){
        previous=buffer;
        buffer=buffer->next;
    }

    if(buffer==NULL||strcmp(buffer->name,ent_name)>0) return NULL;

    previous->next=buffer->next;

    return buffer;
}

rel* pop_first_rel(rel**head){
    rel*buffer=*head;
    *head=buffer->next;
    return buffer;
}

rel_pointer* pop_first_rel_pointer(rel_pointer**head){
    rel_pointer*buffer=*head;
    *head=buffer->next;
    return buffer;
}

void wipe_deleted_ent_from_earth(ent**hash_table, char*ent_name, unsigned int index){
    ent*current_ent;

    for(int i=0;i<MAGIC_NUMBER;i++){
        current_ent=hash_table[i];

        while(current_ent!=NULL){
            rel*current_rel=current_ent->rel;

            while(current_rel!=NULL){
                if(current_rel->hash_table!=NULL) find_and_destroy(&(current_rel->hash_table[index]), ent_name);

                current_rel=current_rel->next;
            }

            current_ent=current_ent->next;
        }
    }
}

int delent(ent**hash_table){
    char ent_name[BUFFER_SIZE];

    get_name2(ent_name);

    //char*ent_name=get_name()

    unsigned int ent_index=hash(ent_name)%MAGIC_NUMBER;

    //ho tolto l'entità dalla hash table
    ent*deleted_ent=pop_ent(&hash_table[ent_index],ent_name);
    //se l'entità era presente

    if(deleted_ent!=NULL){

        rel*head=deleted_ent->rel;

        while(head!=NULL){

            rel* popped_rel=pop_first_rel(&head);

            if(popped_rel!=NULL){
                char*rel_name=popped_rel->name;
                max_rel* max_struct=find_rel_in_max(rel_name);

                if(max_struct!=NULL){

                    //tolgo eventualmente la mia entità da ogni record
                    if(popped_rel->rel_number==max_struct->number){
                        int flag=pop_from_max(max_struct,deleted_ent->name);

                        popped_rel->rel_number=0;

                        if(flag==1) {
                            refresh_max(max_struct, hash_table, rel_name);
                        }
                    }

                    //ora invece svuoto la hash table
                    if(popped_rel->hash_table!=NULL){
                        rel_pointer**rel_hash_table=popped_rel->hash_table;
                        for (int i=0;i<MAGIC_NUMBER_2;i++) {

                            while(rel_hash_table[i]!=NULL){

                                rel_pointer* popped_rel_pointer=pop_first_rel_pointer(&rel_hash_table[i]);
                                ent*popped_ent=popped_rel_pointer->pointed_ent;

                                free(popped_rel_pointer);

                                if(popped_ent!=NULL){
                                    rel*rel_pointer=find_rel(popped_ent,rel_name);

                                    if(rel_pointer!=NULL) {
                                        rel_pointer->rel_number--;

                                        if ((rel_pointer->rel_number + 1) == max_struct->number) {
                                            int flag = pop_from_max(max_struct, popped_ent->name);

                                            if (flag == 1) {
                                                refresh_max(max_struct, hash_table, rel_name);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        free(rel_hash_table);
                        popped_rel->hash_table=NULL;
                    }
                }
            }

            free(popped_rel);
        }

        //ora elimino le tracce di ent
        unsigned int index=hash(deleted_ent->name)%MAGIC_NUMBER_2;

        wipe_deleted_ent_from_earth(hash_table, deleted_ent->name, index);

        free(deleted_ent->name);
        free(deleted_ent);
    }

    //free(ent_name);
    return 0;
}

/*addrel*/

rel* insert_rel(rel**head, char*rel_name){
    rel* current=*head;
    rel*buffer;

    if((current==NULL)||(strcmp(current->name,rel_name)>0)){
        current=malloc(sizeof(rel));
        current->name=rel_name;
        current->rel_number=0;
        current->hash_table=NULL;
        current->next=*head;
        *head=current;
        return current;
    }
    else{

        if(strcmp(current->name,rel_name)==0){
            return current;
        }

        while(current->next!=NULL&&(strcmp(current->next->name,rel_name)<0)){
            current=current->next;
        }

        if(current->next!=NULL){
            if(strcmp(current->next->name,rel_name)==0) {
                return current->next;
            }
        }

        buffer=malloc(sizeof(rel));
        buffer->name=rel_name;
        buffer->rel_number=0;
        buffer->hash_table=NULL;
        buffer->next=current->next;
        current->next=buffer;
        return buffer;
    }
}

void increase_counter(ent*destination,char*rel_name, max_rel*max_struct){
    rel*my_rel;

    my_rel=insert_rel(&destination->rel,rel_name);
    my_rel->rel_number++;

    if(my_rel->rel_number>max_struct->number) found_new_max(destination,max_struct,(int)my_rel->rel_number);
    else if(my_rel->rel_number==max_struct->number) add_to_max(destination,max_struct);
    else return;
}

ent*find(ent*hash_entry,char*destination){
//estrailo, va fatto prima


    int comparison=-1;

    while(hash_entry!=NULL&&(comparison=strcmp(hash_entry->name,destination))<0){
        hash_entry=hash_entry->next;
    }

    if(comparison==0) return hash_entry;

    return NULL;


    /*while(hash_entry!=NULL){
        if(strcmp(hash_entry->name,destination)==0){
            return hash_entry;
        }
        hash_entry=hash_entry->next;
    }
    return hash_entry;*/
}

int add(ent*destination,rel_pointer**head) {
    rel_pointer* current=*head;
    rel_pointer*buffer;

    if((current==NULL)||(strcmp(current->pointed_ent->name,destination->name)>0)){
        current=malloc(sizeof(rel_pointer));
        current->pointed_ent=destination;
        current->next=*head;
        *head=current;
        return 1;
    }
    else{

        if(strcmp(current->pointed_ent->name,destination->name)==0){
            return 0;
        }

        while(current->next!=NULL&&(strcmp(current->next->pointed_ent->name,destination->name)<0)){
            current=current->next;
        }

        if(current->next!=NULL){
            if(strcmp(current->next->pointed_ent->name,destination->name)==0) {
                return 0;
            }
        }

        buffer=malloc(sizeof(rel_pointer));
        buffer->pointed_ent=destination;
        buffer->next=current->next;
        current->next=buffer;
        return 1;
    }
}


void find_and_add(ent*origin_ent,ent*destination_ent,char*rel_name, max_rel*max_struct){ //devo cercare di far puntare alla stessa stringa

    rel*my_rel=insert_rel(&origin_ent->rel,rel_name);

    if(my_rel->hash_table==NULL){
        my_rel->hash_table=calloc(MAGIC_NUMBER_2, sizeof(rel_pointer*));
    }

    unsigned int d_index=hash(destination_ent->name)%MAGIC_NUMBER_2;

    int flag=add(destination_ent,&my_rel->hash_table[d_index]);

    if(flag==1) increase_counter(destination_ent,rel_name,max_struct);

}

int addrel(ent**hash_table){
    char*origin=get_name();
    char*destination=get_name();
    char*rel_name=get_name();

    max_rel* max_struct=find_max_rel(rel_name);

    rel_name=max_struct->rel_name;

    unsigned int index=hash(origin)%MAGIC_NUMBER; //indice della destinazione

    ent*origin_ent=find(hash_table[index],origin);


    if(origin_ent!=NULL) {
        index = hash(destination)%MAGIC_NUMBER;
        ent* destination_ent=find(hash_table[index],destination);

        if(destination_ent!=NULL){
            find_and_add(origin_ent, destination_ent, rel_name, max_struct);
        }
    }


    free(origin);
    free(destination);
    return 0;
}

/*delrel*/

int delrel(ent**hash_table) {
    //char *origin = get_name();
    //char *destination = get_name();
    //char *rel_name = get_name();

    char origin[BUFFER_SIZE];
    char destination[BUFFER_SIZE];
    char rel_name[BUFFER_SIZE];

    get_name2(origin);
    get_name2(destination);
    get_name2(rel_name);

    unsigned  int index=hash(origin)%MAGIC_NUMBER;

    ent*origin_ent=find(hash_table[index],origin);

    if(origin_ent!=NULL){
        rel* rel_pointer=find_rel(origin_ent, rel_name);

        if(rel_pointer!=NULL&&rel_pointer->hash_table!=NULL){
            index=hash(destination)%MAGIC_NUMBER_2;
            ent*destination_ent=find_and_destroy(&(rel_pointer->hash_table[index]),destination);

            if(destination_ent!=NULL){
                rel_pointer=find_rel(destination_ent, rel_name);

                rel_pointer->rel_number--;

                max_rel*max_struct=find_rel_in_max(rel_name);

                if(max_struct!=NULL) {

                    if ((rel_pointer->rel_number + 1) == max_struct->number) {
                        int flag=pop_from_max(max_struct,destination_ent->name);

                        if(flag==1){
                            refresh_max(max_struct,hash_table,rel_name);
                        }
                    }
                }
            }

        }
    }

    //free(origin);
    //free(destination);
    //free(rel_name);
    return 0;
}



/*prova*/
void rel_print_aux(rel_pointer* rel_ent){
    while(rel_ent!=NULL){
        printf("%s\n",rel_ent->pointed_ent->name);
        rel_ent=rel_ent->next;
    }
}

void rel_print(ent*hash_entry){
    rel* buffer=hash_entry->rel;
    rel_pointer**buffer2;

    if(buffer==NULL){
        printf("\nnone\n");
        return;
    }
    while(buffer!=NULL){
        buffer2=buffer->hash_table;
        if(buffer2!=NULL) {
            printf("\n-relazione \"%s\" con %d entranti con\n",buffer->name, buffer->rel_number);
            for (int i = 0; i < MAGIC_NUMBER_2; i++) {
                rel_print_aux(buffer2[i]);
            }
        }
        else{
            printf("\n-relazione %s con %d entranti e none\n", buffer->name, buffer->rel_number);
        }
        buffer=buffer->next;
    }
}

void entry_print(ent*hash_entry){
    if(hash_entry==NULL) return;
    printf("L'entità %s ha le seguenti relazioni:",hash_entry->name);
    rel_print(hash_entry);
    entry_print(hash_entry->next);
}
void hash_print(ent**hash_table){
    for(int i=0;i<MAGIC_NUMBER;i++){
        entry_print(hash_table[i]);
    }
}
void max_print(){
    printf("Riassumento le relazioni sono:\n");

    max_rel* buffer=max;

    while(buffer!=NULL){
        printf("-%s\n", buffer->rel_name);
        printf("con massimi:\n");
        for(int i=0;i<(int)buffer->array_lenght;i++){
            printf("%s\n", buffer->ent_max[i]);
        }
        buffer=buffer->next;
    }
}

int main(){

    //freopen("input1.txt","r",stdin);

    int command=BEGIN;
    ent*hash_table[MAGIC_NUMBER]={NULL};

    while(command!=END){
        command=get_command();

        switch(command){
            case ADDENT:
                addent(hash_table);
                break;
            case DELENT:
                delent(hash_table);
                break;
            case ADDREL:
                addrel(hash_table);
                break;
            case DELREL:
                delrel(hash_table);
                break;
            case REPORT:
                report();
                break;
            default:
                break;
        }

        //hash_print(hash_table);
    }


    //hash_print(hash_table);
    //max_print();

    exit(0);
    //return END;
}
