
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/resource.h>
#include<sys/wait.h>
#include<dirent.h>
#include <signal.h>

char historys[10240][1024];//store histories
char *line; //store the command from function getline()
char line1[1024];//store the command from gets()
char command[500][500]; //stores the command spereated by " "
int count;         // count how many parts are there in the seperated command
int his_count=0;//index for history

//protype
int separate_command();
int execute_command();
int find_command(char *command);
void history();
int change_dir( char arr[]);
void interruptHandlerC(int sig);
void interruptHandlerZ(int sig);
char *get_a_line();
int my_system(char* line);
char *cml="tt_fifo";

int main(int aa, char **bb){
    if(bb[1]&&bb[1][0]){cml=bb[1];}
    while(1){
        line =get_a_line();
        int length=strlen(line);
        if(length>=1) {
            //here is >=1 because gets() does not include '\n',
            // if a line contains '\n' only, then the size of returned line1 by get_a_line() is 0
            my_system(line);
        }
    }
}

char *get_a_line(){
    line1[0]=0;
    printf("$tiny_shell:");
    fflush(stdout);
    signal(SIGINT, interruptHandlerC);
    signal(SIGTSTP, interruptHandlerZ);
    if(gets(line1)!=NULL){
        return line1;
    } else{
        exit (0);
    }
}

int my_system(char* line) {
    int re = separate_command();
    if (!re) {
        return 0;
    }
    if (re != 12) {
        memcpy(historys[his_count], line, sizeof(historys[his_count]));
        his_count++;
        execute_command();

    }
}


void interruptHandlerC(int sig){
    puts("Do you want to exit? Press 'Y' for yes");
    gets(line1);
    if(line1[0]=='Y'||line1[0]=='y'){
        puts("exit successfully");
        exit(0);
    }
    return;
}
// handle ctrl_Z
void interruptHandlerZ(int sig){
    puts("Ctrl_z is ingored");
 
    return;
}

//print history to screen
void history(){
//if history stores more than 100 commands, print the last 100 only
    if (his_count>100){
        int count2=his_count-100;
        for(;count2<count;count2++){
            puts(historys[count2]);
        }
    }else {

        for (int i = 0; i < his_count; i++) {

            puts( historys[i]);
        }
    }
    return ;
}
//change dirctary
int change_dir( char arr[]){
    //cd ..
    chdir(arr);

}

// set limit for the process usage
int limit(char arr[]){
    if(!arr){
        puts("limit need to be given a value");
        return 0;
    }
    if(!arr[0]){
        puts("limit need to be given a value");
        return 0;
    }
    struct rlimit old_lim, new_lim;
    getrlimit(RLIMIT_DATA,&old_lim);

    new_lim.rlim_cur=atoi(arr);
    new_lim.rlim_max=old_lim.rlim_max;
    if(setrlimit(RLIMIT_DATA,&new_lim)==-1){
        puts("Handle setrlimit() error\n");
    }
}

/**
 * separate the command by " "store them in command[][]
 * 
 */
int separate_command()
{
    char *str=line;
    int state=0;
    int i=0;
    int j=0;

    strcat(line," ");
    while(*str){
        //if the last char is space, state is set to 0, pointer(str) ++ to check next char
        //if last char is not space, state is set to 1, store in command
        switch(state){
            case 0:
                if(!isspace(*str))
                    state=1;
                else
                    str++;
                break;
                //if the last
            case 1:
                if(isspace(*str)){
                    //if char is space, and state is 1, add the '\0' in the end of char arr[]
                    //this means finish reading first component of command
                    command[i][j]='\0';
                    i++;
                    j=0;
                    state=0;
                }else{

                    command[i][j]=*str;
                    j++;
                }
                str++;
                break;
        }
    }
    count=i;
    if(count==0)
        return 0;

    //call history if is the internal command "history"
    if(strcmp(command[0],"history")==0){
        memcpy(historys[his_count], line, sizeof(historys[his_count]));
        his_count++;
        history();
        //call history if is the internal command such as "cd .."
        return 12;
    }else if((strcmp(command[0],"cd")==0)||(strcmp(command[0],"chdir")==0)){
        memcpy(historys[his_count], line, sizeof(historys[his_count]));
        his_count++;
        change_dir(command[1]);
        return 12;

    }else if(strcmp(command[0],"limit")==0) {
        memcpy(historys[his_count], line, sizeof(historys[his_count]));
        his_count++;

         limit(command[1]);
        return 12;
    }else if(!find_command(command[0])){
        puts("ERROR: command not found");
        return 0;
    }
    return 1;
}

//execute command
int execute_command()
{
    int i,j,fd,pid,fd2,pid2;
    char* cmd1[50];
    char* cmd2[50];
    int p_counter=0;
    int type=0;//when type=0 is a normal command ; whene type=1 is a pipe
    //arg[] stores the content asw the command[]
    //the arg[] of the last index contain a NULL
    for( i=0;i<count;i++){
        cmd1[i]=command[i];
    }
    cmd1[i]=NULL;

    //$ ls | wc -l |
    //command end with | is invaild
    if(strcmp(cmd1[count-1],"|")==0){
        printf("ERROR:command error\n");
        return 0;
    }

    for(i=0;i<count;i++){

        if(strcmp(cmd1[i],"|")==0){
            p_counter++;
            type=1;//pipe
            cmd1[i]=NULL;
            for(j=i+1;j<count;j++){
                cmd2[j-i-1]=cmd1[j];
            }
            cmd2[j-i-1]=NULL;
            if(!find_command(cmd2[0])){
                printf("ERROR:command not found\n");
                return 0;
            }
        }
    }
    //last|cut -d ' ' -f 1 |sort|uniq -c|sort -n
    //this shell does not deal with more than one |
    if(p_counter>1){
        printf("ERROR:don't identify the command");
        return 0;
    }

    pid=fork();
    if(pid<0){//error occurred
        perror("fork error");
        exit(0);
    }else if(pid==0){// in child process
        switch(type){
            case 0://normal
                execvp(cmd1[0],cmd1);
                break;

            case 1://pipe
            //parent and child execute cmd2, cmd1 separately
                pid2=fork();
                if(pid2==0){
                    fd2=open(cml,O_WRONLY);//write from fifo
                    dup2(fd2,fileno(stdout));
                    execvp(cmd1[0],cmd1);//command on the left of pipe, and executed by child
                }else{
                    fd=open(cml,O_RDONLY);//read from fifo
                    dup2(fd,fileno(stdin));
                    execvp(cmd2[0],cmd2);
                }
//
                break;
        }
    }else{
        waitpid(pid,NULL,0);
    }
    return 1;
}



//find the command from dirctory
int find_command(char *command) {
    char temp[1024];
    char *dir; //store separated path  ex./usr/local/bin
    char temp2[1024];//store the path+file
    
    char *path = getenv("PATH"); //system call to get path

    strcpy(temp, path);
   //sprintf(temp,"%s:./",path);// append ./ to path
    dir = strtok(temp, ":");
    // PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
    //find the file in the dirctory first
    //
   if (strstr(command,"/")&&(access(command,X_OK))!=-1)
        return 1;

    while (dir !=NULL) {
        sprintf(temp2,"%s/%s",dir,command);//store dir and command into temp2
        if ((access(temp2,X_OK))!=-1){
            return 1;
        }

        dir = strtok(NULL, ":");
    }
    return 0;

}


