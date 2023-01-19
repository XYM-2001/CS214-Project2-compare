#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

struct file_queue{
    struct file* front;
    int count;
    struct file* rear;
};
struct dir_queue{
    struct node* front;
    int count;
    struct node* rear;
};
struct node{
    char data[100];
    double count;
    struct node *next;
};
typedef struct file{
    double total;
    char path[100];
    struct node* words;
    struct file* next;
}file;

struct twoFile{
    file* f1;
    file* f2;
};

pthread_mutex_t lock;
struct file_queue* fileq;
struct dir_queue* dirq;
struct file_queue* ffileq;
int dirt = 0, filet = 0, ffilet = 0;//count threads that are currently running
char *suffix;


struct node* node_init(char* data){
    struct node* temp=(struct node*)malloc(sizeof(struct node));
    strcpy(temp->data,data);
    temp->count = 1;
    temp->next = NULL;
    return temp;
}


struct file* file_init(char* path){
    struct file* fl=(struct file*)malloc(sizeof(struct file));
    fl->total = 0.0;
    strcpy(fl->path, path);
    fl->next = NULL;
    fl->words = NULL;
    //printf("fileinit\n");
    return fl;
}


struct file_queue* create_file_queue(){
    struct file_queue* q=(struct file_queue*)malloc(sizeof(struct file_queue));
    q->front=q->rear=NULL;
    q->count=0;
    return q;
}


struct dir_queue* create_dir_queue(){
    struct dir_queue* q=(struct dir_queue*)malloc(sizeof(struct dir_queue));
    q->front=q->rear=NULL;
    q->count=0;
    return q;
}



void dir_enqueue(struct dir_queue *q, char* path){//insert item in the end
    struct node* temp = node_init(path);    

    if (q->rear == NULL) {
        q->front = q->rear = temp;
    q->count += 1;
        return;
    }
 
    // Add the new node at the end of dir queue and change rear
    q->rear->next = temp;
    q->rear = temp;
    q->count += 1;
}



void file_enqueue(struct file_queue *q, file* temp){//push file name to the last one
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        q->count += 1;
        return;
    }
 
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
    q->count += 1;
}


void dir_dequeue(struct dir_queue* q){//remove the first item
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;
 
    // Store previous front and move front one node ahead
    struct node* temp = q->front;
 
    q->front = q->front->next;
 
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
    q->count -= 1;
    free(temp);
}




int free_node(struct node* n){
    struct node* temp = n;
    while(n!= NULL){
        temp = n;
        n = n->next;
        free(temp);
    }
    return 0;
}

void file_dequeueAndFree(struct file_queue* q){
    // If queue has 1 element, return NULL.
    if (q->count < 2){
        free_node(q->front->words);
        free(q->front);
        //printf("freefile\n");
        return;
    }
 
    // Store previous front and move front one node ahead
    struct file* temp = q->front;
 
    q->front = q->front->next;
 
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL){
    	q->rear = NULL;
    }
    q->count -= 1;
    //printf("freefile\n");
    free_node(temp->words);
    free(temp);
}

void file_dequeue(struct file_queue* q){
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;
 
    // Store previous front and move front one node ahead
 
    q->front = q->front->next;
 
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
    q->count -= 1;
}



int checksuffix(char* name){//check if the file name ends with correct suffix, return 1 if correct
    size_t lenstr = strlen(name);
        size_t lensuffix = strlen(suffix);
        if(lensuffix > lenstr){
            return 0;
        }
        return strncmp(name + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int new(char* word, struct node* list){
    struct node* temp = list;
    while(temp != NULL){
        if(strcmp(word, temp->data) == 0){
            temp->count += 1.0;
            return 1;
        }
        temp = temp->next;
    }
    return 0;
}



void *countwfd(void *v){
    //pthread_mutex_lock(&lock);
    FILE *fd;
    fd = fopen(fileq->front->path, "r");
    if(fd != NULL){
        int ch;
        char temp[100] = "\0";
        while((ch = fgetc(fd)) != EOF){
            if(!isspace(ch)){//skip some spaces at the beginning
                char a = ch;
                if(isupper(a)){
                   a = tolower(a);
                }
                strncat(temp, &a, 1);
                break;
            }
        }
        if(strlen(temp) == 0){//empty file
            fclose(fd);
            file_enqueue(ffileq, fileq->front);
            file_dequeue(fileq);
            return 0;
        }
        int spacecount = 0;
        while((ch = fgetc(fd)) != EOF){
            if(isspace(ch)){
                spacecount += 1;
                continue;
            }
            else if(spacecount == 0){
                if(!ispunct(ch)){//keep adding chars to the current buff
                    char a = ch;
                    if(isupper(a)){
                        a = tolower(a);
                    }
                    strncat(temp, &a, 1);
                }
            }
            else{
                spacecount = 0;
                struct node *tempn = fileq->front->words;
                if(tempn == NULL){
                    fileq->front->total += 1.0;
                    fileq->front->words = node_init(temp);
                    //printf("%s--%f\t", temp, fileq->front->total);
                }
                else{
                    while(tempn->next != NULL){//make tempn point to the last element
                        tempn = tempn->next;
                    }
                    if(new(temp, fileq->front->words) == 0){
                        tempn->next = node_init(temp);//add the word to the file list if it's a new word
                        fileq->front->total += 1.0; //increment the total(number of words) by 1
                        //printf("%s--%f\t", temp, fileq->front->total);
                    }
                    else{
                        fileq->front->total += 1.0;
                        //printf("%s--%f\t", temp, fileq->front->total);
                    }
                }
                temp[0] = '\0';
                if(!ispunct(ch)){//keep adding chars to the current buff
                    char a = ch;
                    if(isupper(a)){
                        a = tolower(a);
                    }
                    strncat(temp, &a, 1);
                }
            }
        }
        struct node *tempn = fileq->front->words;
        if(fileq->front->words == NULL){//for the situation where there is only 1 word in the file
        	fileq->front->words = node_init(temp);
            fileq->front->total += 1.0;
            //printf("%s--%f\t", temp, fileq->front->total);
        }
        else{
            while(tempn->next != NULL){
                tempn = tempn->next;

            }
            if(new(temp, fileq->front->words) == 0){
                tempn->next = node_init(temp);//add the word to the file list if it's a new word
                fileq->front->total += 1.0; //increment the total(number of words) by 1
                //printf("%s--%f\t", temp, fileq->front->total);
            }
            else{
                 fileq->front->total += 1.0;
                 //printf("%s--%f\t", temp, fileq->front->total);
            }
        }//add the last word to the file list
        fclose(fd);
        tempn = fileq->front->words;//reset the pointer position
        while(tempn != NULL){//divide all word counts by total, get the WFD
            tempn->count /= fileq->front->total;
            tempn = tempn->next;
        }
        file_enqueue(ffileq, fileq->front);
        file_dequeue(fileq);
    }
    else{
        perror("cannot open file\n");
        return 0;
    }
    //pthread_mutex_unlock(&lock);
    return 0;
}



void *readd(void *v){
    struct node* temp = dirq->front;
    DIR *dir;
    if(((dir = opendir(temp->data)) == NULL)){//cannot open directory, go next
        perror("cannot open directory\n");
        return NULL;
    }
    struct dirent *de;
    de = readdir(dir);
    de = readdir(dir);
    
    while((de = readdir(dir)) != NULL){//read every entry in the directory
        struct stat s;
        char whole[100];
        strcpy(whole, temp->data);
        strcat(whole, "/");
        strcat(whole, de->d_name);
        if(stat(whole, &s) == 0 && S_ISDIR(s.st_mode)){//it's a dirctory
            dir_enqueue(dirq, whole);
        }
        else if(stat(whole, &s) == 0 && S_ISREG(s.st_mode)){//it's a file
            if(checksuffix(whole) == 1){
                file_enqueue(fileq, file_init(whole));
            }
        }
        else{//something else, skip
            continue;
        }
    }
    dir_dequeue(dirq);
    closedir(dir);
    return NULL;
}

void* JSD(void* t){
    struct twoFile* f = (struct twoFile*)t;
    file* f1 = f->f1;
    file* f2 = f->f2;
    struct node* p = f1->words;
    
        int numOfWords = 0;
        p = f1->words;
        while(p != NULL){
            numOfWords+=1;
            p = p->next;
    }
    f1->total=numOfWords;

    numOfWords = 0;
    p = f2->words;
        while(p != NULL){
            numOfWords+=1;
            p = p->next;
    }
    f2->total=numOfWords;
    
    if(f1->words==NULL&&f2->words==NULL){
        printf("%f %s %s\n", sqrt(0),f1->path,f2->path);
        return NULL;
    }
    if(f1->words==NULL||f2->words==NULL){
        printf("%f %s %s\n", sqrt(0.5),f1->path,f2->path);
        return NULL;
    }


    file* tempf =(struct file*)malloc(sizeof(struct file));
    tempf->total=0;
    tempf->words = NULL;
    int ifFound = 0;
    struct node* temN1 = f1->words;
    struct node* temN2 = f2->words;
    struct node* temN3 = tempf->words;
    struct node* ptr = tempf->words;
    for(int i=0;i<f1->total;i++){//start from f1 to calculate average file
        for(int j=0;j<f2->total;j++){//search every words in f2
            if(strcmp(temN1->data,temN2->data)==0){//match
                ifFound = 1;
                if(tempf->words==NULL){
                    tempf->words=node_init(temN1->data);
                    tempf->total+=1;
                    tempf->words->count = (temN1->count+temN2->count)/2;
                    ptr = tempf->words;
                }
                else{
                    ptr->next = node_init(temN1->data);
                    tempf->total+=1;
                    ptr->next->count = (temN1->count+temN2->count)/2;
                    ptr = ptr->next;
                }
                break;//found same in f2, stop search
            }
            else{
                temN2 = temN2->next;
            }
        }
        if(ifFound==0){//means each node in temN2 is not match the node from temN1
            if(tempf->words==NULL){
                tempf->words=node_init(temN1->data);
                tempf->total+=1;
                tempf->words->count = (temN1->count)/2;
                ptr = tempf->words;
            }
            else{
                ptr->next = node_init(temN1->data);
                tempf->total+=1;
                ptr->next->count = (temN1->count)/2;
                ptr = ptr->next;
            }
        }
        ifFound=0;//refresh ifFound
        temN1 = temN1->next;
        temN2 = f2->words;//refresh temN2;
    }
    //refresh temN1 and temN2;
    temN1 = f1->words;
    temN2 = f2->words;
    temN3 = tempf->words;
    ifFound=0;
    for(int i=0;i<f2->total;i++){//inset node from the unique node in temN2
        for(int j=0;j<tempf->total;j++){
            if(strcmp(temN2->data,temN3->data)==0){//match
                ifFound = 1;
                break;//found same in tempf, stop search
            }
            else{
                temN3 = temN3->next;
            }
        }
        if(ifFound==0){//means each node in tempf is not match the node from temN2
            if(tempf->words==NULL){
                tempf->words=node_init(temN2->data);
                tempf->total+=1;
                tempf->words->count = (temN2->count)/2;
                ptr = tempf->words;
            }
            else{
                ptr->next = node_init(temN2->data);
                tempf->total+=1;
                ptr->next->count = (temN2->count)/2;
                ptr = ptr->next;
            }
        }
        ifFound=0;
        temN2 = temN2->next;
        temN3=tempf->words;
    }
    //now tempf has the values of mean frequency
    //now count two KLD
    double kld1=0;
    double kld2=0;
    temN1 = f1->words;
    temN2 = f2->words;
    temN3 = tempf->words;
    ifFound=0;
    for(int i=0;i<f1->total;i++){//calculate KLD f1||tempf
        for(int j=0;j<tempf->total;j++){
            if(strcmp(temN1->data,temN3->data)==0){//match
                ifFound = 1;
                kld1 = kld1 + (temN1->count*log2((temN1->count)/(temN3->count)));
                break;//found same in tempf, stop search
            }
            else{
                temN3 = temN3->next;
            }
        }
        ifFound=0;
        temN1 = temN1->next;
        temN3 = tempf->words;
    }
    //start calculate KLD f2||tempf
    for(int i=0;i<f2->total;i++){//calculate KLD f1||tempf
        for(int j=0;j<tempf->total;j++){
            if(strcmp(temN2->data,temN3->data)==0){//match
                ifFound = 1;
                kld2 = kld2 + (temN2->count*log2((temN2->count)/(temN3->count)));
                break;//found same in tempf, stop search
            }
            else{
                temN3 = temN3->next;
            }
        }
        ifFound=0;
        temN2 = temN2->next;
        temN3 = tempf->words;
    }
    double jsd = 0;
    jsd = sqrt(0.5*kld1+0.5*kld2);
    printf("%f %s %s\n", jsd,f1->path,f2->path);

    free_node(tempf->words);
    free(tempf);
    //free_node(temN1);
    //free_node(temN2);
    return NULL;
}


int main(int argc, char** argv){
	if(argc < 2){
		perror("no argument\n");
		exit(EXIT_FAILURE);
	}
    dirq = create_dir_queue();
    fileq = create_file_queue();
    ffileq = create_file_queue();
    int dirthreads = 1, filethreads = 1, i;
    int analythreads = 1;
    suffix = ".txt";
    for(i = 1; i < argc; i++){//read optional args first, this require all opt args to occur before the regular args
            if(argv[i][0] != '-'){//not an optional arg
                break;
            }
            else if(argv[i][1] == 'd'){//directory threads
                char buf[strlen(argv[i])-1];
                for(int j = 0; j < strlen(argv[i]); j++){
                    buf[j] = argv[i][j+2];
                }
                buf[strlen(argv[i])-1] = '\0';
                dirthreads = atoi(buf);
            }
            else if(argv[i][1] == 'f'){//file threads
                char buf[strlen(argv[i])-1];
                for(int j = 0; j < strlen(argv[i]); j++){
                    buf[j] = argv[i][j+2];
                }
                buf[strlen(argv[i])-1] = '\0';
                filethreads = atoi(buf);
            }
            else if(argv[i][1] == 'a'){//analysis threads
                char buf[strlen(argv[i])-1];
                for(int j = 0; j < strlen(argv[i]); j++){
                    buf[j] = argv[i][j+2];
                }
                buf[strlen(argv[i])-1] = '\0';
                analythreads = atoi(buf);
            }
            else if(argv[i][1] == 's'){//file name suffix
                suffix = malloc((strlen(argv[i])));
                for(int j = 2; j < strlen(argv[i]); j++){
                    suffix[j-2] = argv[i][j];
                }
                suffix[strlen(argv[i])-2] = '\0';
            }
            else{//argument error
                perror("user argument error\n");
                exit(EXIT_FAILURE);
            }
        }
    struct stat s;
    for(;i < argc; i++){//loop through regular arguments add each elements to its corresponding queue
        if(stat(argv[i], &s) == 0 && S_ISDIR(s.st_mode)){//is a dirctory
            dir_enqueue(dirq, argv[i]);
        }
        else if(stat(argv[i], &s) == 0 && S_ISREG(s.st_mode)){//is a file
            file_enqueue(fileq, file_init(argv[i]));
        }
    }
    pthread_t dirtid[dirthreads], filetid[filethreads];
    pthread_t analytid[analythreads];
    while(dirq->front != NULL){
        if(dirt >= dirthreads){
            dirt = 0;
        }
        pthread_create(&dirtid[dirt], NULL, readd, NULL);
        pthread_join(dirtid[dirt], NULL);
        dirt += 1;
    }
    if(fileq->count < 2){
    	perror("not enough file to compare\n");
    	exit(EXIT_FAILURE);
    }
    while(fileq->front != NULL){
        if(filet >= filethreads){
            filet = 0;
        }
        pthread_create(&filetid[filet], NULL, countwfd, NULL);
        pthread_join(filetid[filet], NULL);
        filet += 1;
    }
    /*struct file* t = ffileq->front;
    while(t != NULL){
        printf("%s\n", t->path);
        struct node* ptr = t->words;
        while(ptr != NULL){
            printf("%s\t", ptr->data);
            ptr = ptr->next;
        }
        printf("\n");
        t = t->next;
    }*/
    struct twoFile* args = malloc(sizeof(struct twoFile));
    while(ffileq->front->next != NULL){
        args->f1 = ffileq->front;
        struct file* ptr = ffileq->front->next;
        while(ptr != NULL){
            args->f2 = ptr;
            if(ffilet >= analythreads){
                ffilet = 0;
            }
            pthread_create(&analytid[ffilet], NULL, JSD, args);
            pthread_join(analytid[ffilet], NULL);
            ffilet+=1;
            ptr = ptr->next;
        }
        //printf("%f\n", ffileq->front->next->total);
        args->f1 = args->f1->next;
        file_dequeueAndFree(ffileq);
        
    }
    file_dequeueAndFree(ffileq);
    free(dirq);
    free(fileq);
    free(ffileq);
    free(args);
    return 0;
}



