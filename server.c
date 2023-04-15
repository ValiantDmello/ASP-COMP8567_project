#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h>
#include <fcntl.h>

//Usage: ./server [port number]

//function to handle erroe
void error(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

//function to write on socket
int writesocket(const char *msg, const long len, const int fd){
    int n = write(fd, msg, len);    //write msg on socket fd
    if(n<0){
        printf("Error on writing to client, please try again\n");
        close(fd);
        exit(EXIT_FAILURE);
    }
    return n;
}

//function to read from socket
int readsocket(char *buffer, const int fd){
    bzero(buffer, 1024);
    int n = read(fd, buffer, 1024); //read into buffer from socket fd
        if(n<0){
            printf("Error on reading from client, please try again\n");
            close(fd);
            exit(EXIT_FAILURE);
        }
    return n;
}

//function to serve findfile command
int findfile(char *args[], const int newsockfd){    
    printf("Finding file: %s\n", args[1]);

    char command[1024];  // buffer to hold the command string
    char result[1024];  // buffer to hold the command output
    char msg[1024]; //buffer to hold msg for client
    FILE *fp;  // file pointer

    // build the command string for find
    bzero(command, 1024);
    snprintf(command, sizeof(command), "find ~ -name %s -printf '%%s,%%CY-%%Cm-%%Cd %%CH:%%CM,%%p,\\n'", args[1]);

    // run the command and capture its output
    fp = popen(command, "r");
    if (fp == NULL) {   //popen failed
        perror("Error on popen: please try again\n");
        snprintf(msg, sizeof(msg), "Error on popen: please try again"); //give message Error on popen: please try again
        writesocket(msg, strlen(msg), newsockfd);    //let client know command failed
        return -1;
    }
    bzero(result, 1024);
    fgets(result, sizeof(result), fp);  //get results from command
    pclose(fp);

    // check if the file was found
    if (strlen(result) <= 1) {
        printf("File not found: %s\n", args[1]);
        snprintf(msg, sizeof(msg), "File not found: %s", args[1]);  //give message that file was not found [filename]
        writesocket(msg,  strlen(msg),newsockfd);    //let client know file was not found
        return 0;
    }

    //split result string to get size, date and path of file
    char *token = strtok(result, ",");
    char *size = token;
    token = strtok(NULL, ",");
    char *date = token;
    token = strtok(NULL, ",");
    char *path = token;
   
    snprintf(msg, sizeof(msg), "Name : %s, Size: %s bytes, Date: %s", args[1], size, date); //build message for client
    printf("sending to client : %s\n", msg);    
    writesocket(msg, strlen(msg), newsockfd);    //send message to client
    
    return 0;
}

//function to serve copy from src to des
int copyfile(char *src, char* des){     
    int source_file, dest_file, bytes_read;
    char cpbuffer[1024];
    //open src file
    source_file = open(src, O_RDONLY);
    if(source_file == -1) {
        perror("Cant open file\n");
        printf("Error open file %s\n", src);
        return -1;
    }
    //open des file   
    dest_file = open(des, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if(dest_file == -1) {
        perror("Cant open file\n");
        printf("Error open file %s\n", des);
        return -1;
    }
    //loop to read from src and write into des   
    while((bytes_read = read(source_file, cpbuffer, sizeof(cpbuffer))) > 0) {
        if(write(dest_file, cpbuffer, bytes_read) != bytes_read) {
            perror("Cant write on file\n");
            printf("Error write file %s\n",des);
            return -1;
        }
    }
    return 0;
}

//function to make tar for a given list of files
int maketar(const int foundfileslen, char *filespath[], char *filesfound[]){
    int n = system("mkdir -p projecttemp927/temp"); //make a directory to store files
    if(n != 0){ //error
        perror("Error mkdir creating temp.tar.gz");
        exit(EXIT_FAILURE);
    }

    char des[1024];
    //loop to iterate through files
    for(int i=0; i<foundfileslen; i++){
        bzero(des, 1024);   
        snprintf(des, sizeof(des), "./projecttemp927/temp/%s", filesfound[i]);  //construct destination path for copy
        if(copyfile(filespath[i], des)<0){  //copy files to destination
            perror("error in \n");  //error
            exit(EXIT_FAILURE);
        }
    }

    if(system("tar -czf temp2.tar.gz -C ./projecttemp927/temp/ .")!=0){  //tar the folder to temp2.tar.gz
        perror("Error creating temp2.tar.gz\n");    //error
        exit(EXIT_FAILURE);
    }

    //rmdir(""./projecttemp927");
    system("rm -rf ./projecttemp927");  //delete the temporary folder that was created
    return 0;
}

int sendtar(const int newsockfd){
    int source_file, dest_file, bytes_read, bytes_wrote;
    char buffer[1024];
    
    //open the source tar file to be sent
    source_file = open("temp2.tar.gz", O_RDONLY);
    if(source_file == -1) {
        perror("open source file");
        return -1;
    }

    bzero(buffer, 1024);
    bytes_read = read(source_file, buffer, 1024);   //read from src file
    if(bytes_read<0){   //error
        perror("Error reading tar\n");
        exit(EXIT_FAILURE);
    }
    //loop to keep reading from src 
    while(bytes_read>0){   //loop exits when 0 bytes read i.e. end of the file reached 
        writesocket(buffer,  bytes_read, newsockfd);
        bzero(buffer, 1024);
        bytes_read = read(source_file, buffer, 1024);
        if(bytes_read<0){   //error
            perror("Error reasing tar\n");
            exit(EXIT_FAILURE);
        }
    } 
    sleep(1);
    writesocket("\0", 1, newsockfd);    //send a null character to client to know to exit read loop
    close(source_file);  
    return 0;
}

int getfiles(char *args[], const int argcount, const int newsockfd){
    printf("serving getfiles\n"); 
    if(argcount<2){     //incorrect argguments
        perror("Incorrect arguments\n");
        return -1;
    }
    int filecount = argcount-1; //filecount if -u not there
    if(strcmp(args[argcount-1], "-u")==0){  //filecount if -u there
        filecount = argcount-2;
    }
    char *files_list[filecount];    //stores files name
    for(int i = 0; i<filecount; i++){   //loop to add file name sto files_list
        files_list[i] = args[i+1];
    }

    char command[1024];  // buffer to hold the command string
    char result[1024];  // buffer to hold the command output
    FILE *fp;  // file pointer

    char *filespath[6];    //array to hold absolute path of file
    char *filesfound[6];    //array to hold names of files found

    int j = 0;//keep count of files
    //loop to check if file from files_list is found
    for(int i=0; i<filecount; i++){
        bzero(command, 1024);
        snprintf(command, sizeof(command), "find ~ -name %s", files_list[i]);   //construct find command
        fp = popen(command, "r");   //run the command
        if (fp == NULL) {
            perror("popen");
            return -1;
        }
        bzero(result, 1024);
        fgets(result, sizeof(result), fp);  //get results from command
        if(strcmp(result, "")!=0){               //if result found means the file exist
            char *token = strtok(result, "\n"); //just take the first file
            filespath[j] = strdup(token);         //add it to array filepath
            filesfound[j] = strdup(files_list[i]); //add filename to filefond array
            pclose(fp);
            j++;
        }   
    }
    if(j==0){   //no files found; send "false" message to client 
        writesocket("False", 5, newsockfd); 
        return 0;
    }else{  //files found; send "true" message to client
        writesocket("True", 4, newsockfd);
    }
    const int foundfileslen = j;    //store the number of files found

    if(maketar(foundfileslen, filespath, filesfound)<0){    //make tar file of the files found
        printf("Error in making tar\n");    //error
        exit(EXIT_FAILURE);
    }
    sendtar(newsockfd); //send tar to client
    system("rm temp2.tar.gz");  //remove tar tar created by server

    return 0;
}

//function to serve gettargz command
int gettargz(char *args[], const int argcount, const int newsockfd){
    int extcount = argcount-1; //filecount if -u not there
    if(strcmp(args[argcount-1], "-u")==0){  //filecount if -u there
        extcount = argcount-2;
    }
    char *exts[extcount];    //stores files name
    for(int i = 0; i<extcount; i++){   //loop to add file name sto files_list
        exts[i] = args[i+1];
    }

    char command[256];
    long fc = 0;
    for(int i=0; i<extcount; i++){
        sprintf(command, " find ~ -type f -name '*.%s'", exts[i]);  //construct a command to find all files wit exts[i] extension

        FILE *fp = popen(command, "r"); //run the command
        if (!fp) {
            printf("Failed to run command\n");  //error
            exit(EXIT_FAILURE);
        }

        int n = system("mkdir -p projecttemp927/temp"); //make a directory to store files to be tarred
        if(n != 0){ //error
            perror("Error mkdir creating temp.tar.gz");
            exit(EXIT_FAILURE);
        }

        char path[1024];
        //loop to get filepath and file name from the result of find command
        while (fgets(path, 1024, fp)) {
            path[strcspn(path, "\n")] = 0;          // remove trailing newline from file path
            char *name = strrchr(path, '/') + 1;    //fine name
           char despath[1024];
            sprintf(despath, "./projecttemp927/temp/%s", name); //construct des path for copying file
            if(copyfile(path, despath)<0){  //copy the found file into a directory, so that it can be tarred later
                perror("error in \n");
                exit(EXIT_FAILURE);
            }
            fc++; //file count
            bzero(path, 1024);
        }

        pclose(fp);
    }

    if(fc==0){   //no files found; send "false" message to client 
        writesocket("False", 5, newsockfd); 
        return 0;
    }else{  //files found; send "true" message to client
        writesocket("True", 4, newsockfd);
    }

    if(system("tar -czf temp2.tar.gz -C ./projecttemp927/temp/ .")!=0){  //tar the directory in wich all files were copied
        perror("Error creating temp2.tar.gz\n");    //error
        exit(EXIT_FAILURE);
    }

    //rmdir("./projecttemp927");
    system("rm -rf ./projecttemp927");  //delete the temporary folder that was created

    sendtar(newsockfd); //send the tar file
    system("rm temp2.tar.gz");  //remove the tar created by server
}

//function to serve sgetfiles command (almost same as above function)
int sgetfiles(char *args[], const int newsockfd){
    printf("serving sgetfiles\n");
    long size1 = atol(args[1]); //size1
    long size2 = atol(args[2]); //size2
    char command[256];
    sprintf(command, "find ~ -size +%ldb -size -%ldb", size1, size2);   //construct find command
    
    int n = system("mkdir -p projecttemp927/temp"); //make a directory to store files to be tarred
    if(n != 0){ //error
        perror("Error mkdir creating temp.tar.gz");
        exit(EXIT_FAILURE);
    }

    FILE *fp = popen(command, "r"); //run the command
    if (!fp) {
        printf("Failed to run command\n");  //error
        exit(EXIT_FAILURE);
    }

    char path[1024];
    //loop to get filepath and file name from the result of find command
    while (fgets(path, 1024, fp)) {
        path[strcspn(path, "\n")] = 0; // remove trailing newline from path
        char *name = strrchr(path, '/') + 1;    //get file name
        char despath[1024];
        sprintf(despath, "./projecttemp927/temp/%s", name); //construct despath string
        if(copyfile(path, despath)<0){  //copy the found file into a directory, so that it can be tarred later
            perror("error in \n");  //error
            exit(EXIT_FAILURE);
        }
    }
    pclose(fp);

    if(system("tar -czf temp2.tar.gz -C ./projecttemp927/temp/ .")!=0){  //tar the folder
        perror("Error creating temp2.tar.gz\n");
        exit(EXIT_FAILURE);
    }

    //rmdir("./projecttemp927");
    system("rm -rf ./projecttemp927");  //delete the temporary folder that was created

    sendtar(newsockfd); //send tar to client
    system("rm temp2.tar.gz");  //remove tar created by server

    return 0;
}

//function to serve dgetfiles command; eaxct same logic as above function
int dgetfiles(char *args[], const int newsockfd){
    printf("serving sgetfiles\n");
    char *date1 = args[1];
    char *date2 = args[2];
    char command[256];
    sprintf(command, "find ~ -newerct '%s 00:00:00' ! -newerct '%s 23:59:59'", date1, date2);
    
    int n = system("mkdir -p projecttemp927/temp"); //make a directory to store files
    if(n != 0){ //error
        perror("Error mkdir creating temp.tar.gz");
        exit(EXIT_FAILURE);
    }

    FILE *fp = popen(command, "r");
    if (!fp) {
        printf("Failed to run command\n");
        exit(EXIT_FAILURE);
    }

    char path[1024];
    while (fgets(path, 1024, fp)) {
        path[strcspn(path, "\n")] = 0; // remove trailing newline
        char *name = strrchr(path, '/') + 1;
        char despath[1024];
        sprintf(despath, "./projecttemp927/temp/%s", name);
        if(copyfile(path, despath)<0){  
            perror("error in \n");  //error
            exit(EXIT_FAILURE);
        }
    }
    pclose(fp);

    if(system("tar -czf temp2.tar.gz -C ./projecttemp927/temp/ .")!=0){  //tar the folder
        perror("Error creating temp2.tar.gz\n");
        exit(EXIT_FAILURE);
    }

    //rmdir("./projecttemp927");
    system("rm -rf ./projecttemp927");  //delete the temporary folder that was created

    sendtar(newsockfd); //send tar
    system("rm temp2.tar.gz");//delete temp tar

    return 0;
}

//function to toekinze and execute the command from buffer string
int execute(const char *buffer, const int newsockfd){   
    char buffer_cp [strlen(buffer)];
    strcpy(buffer_cp, buffer);  //make a copy of buffer

    // split the string to get the command and its arguments, with delemiter space or new line
    char *token = strtok(buffer_cp, " \n");  
    char *args[10];     //store split string in args array; args[0] stores the command and the arguments from args[1]
    int i = 0;
    while (token != NULL) { //loop to split buffer
        args[i] = token;
        token = strtok(NULL, " \n");
        i++;
        if(i==9)
            break; 
    }
    const int argcount = i; //store the arguments count

    if(strcmp(args[0], "findfile")==0){ //findfile command
        return findfile(args, newsockfd);  //sfindfile returns -1 if error encountered and 0 for success
    }else if(strcmp(args[0], "sgetfiles")==0){  //sgetfiles
        return sgetfiles(args, newsockfd);
    }else if(strcmp(args[0], "dgetfiles")==0){  //dgetfiles
        return dgetfiles(args, newsockfd);
    }else if(strcmp(args[0], "getfiles")==0){   //getfiles
        return getfiles(args, argcount, newsockfd);
    }else if(strcmp(args[0], "gettargz")==0){   //gettargz
        return gettargz(args, argcount, newsockfd);
    }else if(strcmp(args[0], "quit")==0){   //quit
        printf("Quiting\n");
        close(newsockfd);
        exit(EXIT_SUCCESS);
    }else{  //incorect command; client won't send a buffer which will reach this condition
        printf("No command found\n");
        exit(EXIT_FAILURE);
    }
}

//function to fork and serve client
int processclient(int newsockfd){
    int f = fork(); //fork
    if(f<0) { //error
        perror("Fork error");
        return -1;
    }         
    else if(f>0){       //parent
        return 0;
    }

    //child serving the client
    //sleep(1);
    char *msg = "Connected to server  ";  //send message to client that it is connected to server
    write(newsockfd, msg, 20); 
    char buffer[1024];
    int n;
    //loop to get commands from client
    while(1){   
        bzero(buffer, 1024); 
        readsocket(buffer, newsockfd);  //read command from client
        printf("Recieved from client : %s\n", buffer);
        execute(buffer, newsockfd); //execute the command
    }
    close(newsockfd);
    exit(EXIT_SUCCESS);
}


int main(int argc, char  *argv[]){

    
    if(argc != 2){                  //need to input portno argument
        printf("invalid args\n Usage ./server [port number]\n");   //incorrect args
        exit(EXIT_FAILURE);
    }

    int sockfd, newsockfd, portno, n;   //variable declaration
    char buffer[1024];

    //socket variables assignment and binding and listening for client 
    struct sockaddr_in serv_addr, cli_addr;      
    socklen_t clilen;
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if(sockfd< 0)
        fprintf(stderr,"Error opening Socket\n");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atol(argv[1]);     
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //bind socket
        error("Binding Failed\n");  
    listen(sockfd, 5);  //listen to for client
    clilen = sizeof(cli_addr); 

    //client handling
    int clientcounter = 0;  //keep the count of client
    while(1){   //loop to accept clients
        clientcounter++;
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //accept client
        if(newsockfd<0){
            perror("Error on Accept\n");
        }
        if(clientcounter>=1 && clientcounter<=4){           //first 4 clients(1-4)
            processclient(newsockfd);   //serve client
        }else if(clientcounter>=5 && clientcounter<=8){     //next 4 clients(5-8)
            close(newsockfd);   //close the socket
            //mirror will handle
        }else if(clientcounter%2==1){                       //all odd number clients after 8 
            processclient(newsockfd);   //serve client
        }else{                                              //all even number clients after 8 
            close(newsockfd);   //close the socket
            //mirror will handle
        }
    }
    
    close(sockfd);
    return 0;
}