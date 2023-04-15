#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h>
#include<netdb.h>
#include <fcntl.h>
#include <regex.h>

//Usage: ./client [ip address] [server port number] [mirror port number]


void error(const char *msg){    //function to handle errors
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
    int n = read(fd, buffer, 1024); //read into buffer from fd
    if(n<0){
        printf("Error on reading, please try again\n");
        return -1;
    }
    return n;
}

//function to handle findfile command
int servefindfile(const char *buffer, const int sockfd){
    writesocket(buffer, strlen(buffer), sockfd);    //send command to server
    char buff[1024];
    readsocket(buff, sockfd);   //read message sent by server
    printf("%s\n", buff);
    return 0;
}

//function to recieve temp.tar.gz file from server/mirror
int recievetar(const int sockfd){
    int source_file, dest_file, bytes_read;
    char buffer[1024];
    
    //destination file to write on => temp.tar.gz
    dest_file = open("temp.tar.gz", O_CREAT | O_WRONLY | O_TRUNC, 0777);
    if(dest_file == -1) {
        perror("open dest file");
        exit(EXIT_FAILURE);
    }
    bytes_read = readsocket(buffer, sockfd);    //read from sockfd
    if(bytes_read<0){   //error
        perror("Error reading tar\n");
        exit(EXIT_FAILURE);
    }
    if(bytes_read==1 && strcmp(buffer, "\0")){  //nothing for server to send
        close(dest_file);
        return 0;
    }
    while(bytes_read>0){    //loop to keep reading
        int n=write(dest_file, buffer, bytes_read);    //write on buffer read from sockfd in temp.tar.gz
        bytes_read = readsocket(buffer, sockfd);   //read from sockfd 
        if(bytes_read<0){
            perror("Error reading tar\n");
            exit(EXIT_FAILURE);
        }
        if(bytes_read==1 && buffer[0]=='\0'){   //loop exit condition; server sends '/0' to client
            break;
        }
    }
    close(dest_file);
    printf("recieved temp.tar.gz\n");
    return 0;
}

int servegetfiles(const char *buffer, const int argcount, const int uflag, const int sockfd){
    writesocket(buffer, strlen(buffer), sockfd);    //send the getfile command to server/mirror

    char filesfound[1024];  
    readsocket(filesfound, sockfd); //First server sends True of False, for files found
    if(strcmp(filesfound,"False")==0){  //if recieved False
        printf("No File Found\n");      //means no file found
        return 0;                       //exit
    }

    recievetar(sockfd); //recieve tar from server/mirror

    if(uflag==0){   //no need to unzip
        return 0;   //exit
    }
   
    printf("Unzipping\n");
    if(system("tar -xzf temp.tar.gz")<0){   //unzip temp.tar.gz to pwd
        printf("Error uzipping\n");
        return -1;
    }else{
        printf("Unzip success\n");
    }
    return 0;
}

//function to handile sgetfiles
int servesgetfiles(const char *buffer, const int uflag, const int sockfd){
    writesocket(buffer, strlen(buffer), sockfd);    //send sgetfiles command to server/mirror
    recievetar(sockfd);     //recieve temp.tar.gz from server/mirror
    
    if(uflag==0){   //no unzipping
        return 0;   //exit
    }
   
    printf("Unzipping\n");  
    if(system("tar -xzf temp.tar.gz")<0){  //unzip temp.tar.gz to pwd 
        printf("Error uzipping\n");
        return -1;
    }else{
        printf("Unzip success\n");
    }
    return 0;
}

//function to handle dgetfiles command
int servedgetfiles(const char *buffer, const int uflag, const int sockfd){
    writesocket(buffer, strlen(buffer), sockfd);    //send dgetfiles command to server/mirror
    recievetar(sockfd); //recieve temp.tar.gz
    
    if(uflag==0){   //no unzipping
        return 0;
    }
   
    printf("Unzipping\n");
    if(system("tar -xzf temp.tar.gz")<0){   //unzip temp.tar.gz to pwd
        printf("Error uzipping\n");
        return -1;
    }else{
        printf("Unzip success\n");
    }
    return 0;
}

//finction to check if string is in yyyy-mm-dd date fromat
int checkdate(char *date){
    regex_t regex;
    int reti;

    reti = regcomp(&regex, "^[0-9]{4}-[0-9]{2}-[0-9]{2}$", REG_EXTENDED);   //regular expresion for date
    if (reti) {
        printf("Could not compile regex\n");    //error
        return 1;
    }
   
    reti = regexec(&regex, date, 0, NULL, 0);   // Execute regular expression
    if (!reti) {
        regfree(&regex);    //matched date format
        return 0;
    } else if (reti == REG_NOMATCH) {
        regfree(&regex);    //did not match date format
        return -1;
    } else {     //error
        printf("Date match failed\n"); 
        regfree(&regex);
        return -1;
    }
    regfree(&regex);//free regex
    return 0;
}

//function to check dat1<=date2
int compare_dates(const char* date1, const char* date2) {
    int year1, month1, day1;
    int year2, month2, day2;

    sscanf(date1, "%d-%d-%d", &year1, &month1, &day1);  //get year, month date from date1
    sscanf(date2, "%d-%d-%d", &year2, &month2, &day2);  //get year, month date from date2

    //convert date to int, so that they can be compared
    int date_int1 = year1 * 10000 + month1 * 100 + day1;
    int date_int2 = year2 * 10000 + month2 * 100 + day2;

    if (date_int1 <= date_int2) {   //compare dates
        return 0;   //correct
    } else {
        return -1;  //incorrect
    }
}

//function to handke gettargz command
int servegettargz(const char *buffer, const int argcount, const int uflag, const int sockfd){
    writesocket(buffer, strlen(buffer), sockfd);    //send gettargz command to server/mirror
    
    char filesfound[1024];  
    readsocket(filesfound, sockfd); //First server sends True of False, for files found
    if(strcmp(filesfound,"False")==0){  //if recieved False
        printf("No File Found\n");      //means no file found
        return 0;                       //exit
    }
    
    recievetar(sockfd); //recieve temp.tar.gz
    
    if(uflag==0){   //no unzipping
        return 0;
    }
   
    printf("Unzipping\n");
    if(system("tar -xzf temp.tar.gz")<0){   //unzip temp.tar.gz to pwd
        printf("Error uzipping\n");
        return -1;
    }else{
        printf("Unzip success\n");
    }
    return 0;
}

int checkcmd(const char *buffer, const int sockfd){
    char buffer_cp [strlen(buffer)];
    strcpy(buffer_cp, buffer);

    // split the string to get the command and its arguments
    char *token = strtok(buffer_cp, " \n");     //split with delimites blankspace or new line
    char *args[10]; //store split string of buffer in args array; args[0] stores the command and the arguments from args[1]
    int i = 0;  //i to keep count of how many tokens split
    while (token != NULL) { //loop to split rest of the string
        args[i] = token;    //ass token to args array
        token = strtok(NULL, " \n");    //split
        i++;
        if(i==9)
            break; 
    }
    const int argcount = i; //set argument count = i

    if(strcmp(args[0], "findfile")==0){ //findfile command
        if(argcount!=2){    //arguments for findfile need to be 2 only
            printf("Incorrect arguments for findfile\n Usage: findfile [filename]\n");
            return -1;
        }else{ 
            //command input is correct      
            servefindfile(buffer, sockfd);  //serve the filename command
        }
    }else if(strcmp(args[0], "sgetfiles")==0){
        if(argcount<3 || argcount>4){   //check that args are either 3 or 4
            printf("Incorrect arguments for sgetfiles\nUsage: sgetfiles [size1] [size2] <-u>\n");
            return -1;
        }
        if(argcount==4 && strcmp(args[3],"-u")!=0){ //if 4 args check if last arg is "-u"
            printf("Incorrect arguments for sgetfiles\nUsage: sgetfiles [size1] [size2] <-u>\n");
            return -1;
        } 
        long size1 = atol(args[1]); //get size1
        long size2 = atol(args[2]); //get size2
        //check if sizes input are correct
        if (size1 < 0 || size2 < 0 || size1 > size2) {
            printf("Invalid sizes\nUsage: sgetfiles [size1] [size2] <-u>\n");
            return -1;
        }
        if (size1 == 0 && strcmp(args[1], "0") != 0) {
            printf("Invalid sizes\nUsage: sgetfiles [size1] [size2] <-u>\n");
            return -1;
        }
        if (size2 == 0 && strcmp(args[2], "0") != 0) {
            printf("Invalid sizes\nUsage: sgetfiles [size1] [size2] <-u>\n");
            return -1;
        }
        int uflag=0;    //uflag set to 0 if "-u" not present
        if(argcount==4){
            uflag = 1;  //uflag set to 0 if "-u" present
        }
        //command input is correct  
        servesgetfiles(buffer, uflag,sockfd);   //server sgetfiles command
        return 0;
    }else if(strcmp(args[0], "dgetfiles")==0){
        if(argcount<3 || argcount>4){   //check that args are either 3 or 4
            printf("Incorrect arguments for sgetfiles\nUsage: dgetfiles [date1] [date2] <-u>\n");
            return -1;
        }
        if(argcount==4 && strcmp(args[3],"-u")!=0){ //if 4 args check if last arg is "-u"
            printf("Incorrect arguments for sgetfiles\nUsage: dgetfiles [date1] [date2] <-u>\n");
            return -1;
        } 
        char *date1 = args[1];
        char *date2 = args[2];
        //check if input dates are correct
        if(checkdate(date1)<0 || checkdate(date2)<0){
             printf("Incorrect date, please input date in format yyyy-mm-dd\nUsage: dgetfiles [date1] [date2] <-u>\n");
            return -1;
        }
        //check if daite1<=date
        if(compare_dates(date1, date2)<0){
             printf("Incorrect dates, please input date1<=date2\nUsage: dgetfiles [date1] [date2] <-u>\n");
            return -1;
        }
        
        int uflag=0;    //uflag set to 0 if "-u" not present
        if(argcount==4){
            uflag = 1;  //uflag set to 0 if "-u" present
        }
        //command input is correct  
        servedgetfiles(buffer, uflag,sockfd);   //serve dgerfiles command
        return 0;
    }else if(strcmp(args[0], "getfiles")==0){
        if(argcount == 8 && strcmp(args[argcount-1], "-u")!=0){ //if 8 args check if last one is "-u"
            printf("Incorrect arguments for getfiles\n Usage: getfiles file1 file2 .. file6 <-u>\n");
            return -1;
        }else if(argcount < 2 || argcount > 8){ //args between 2 to 7
            printf("Incorrect arguments for getfiles\n Usage: getfiles file1 file2 .. file6 <-u>\n");
            return -1;
        }else if(argcount == 2 && strcmp(args[argcount-1], "-u")==0){   //if args 2, 2nd arg should not be "-u"
            printf("Incorrect arguments for getfiles\n Usage: getfiles file1 file2 .. file6 <-u>\n");
            return -1;
        }
        int uflag = 0;  //uflag set to 0 if "-u" not present
        if(strcmp(args[argcount-1], "-u")==0){
            uflag = 1;  //uflag set to 0 if "-u" present
        }
        //command input is correct  
        servegetfiles(buffer, argcount, uflag, sockfd); //serve getfiles command
        return 0;
    }else if(strcmp(args[0], "gettargz")==0){
        if(argcount == 8 && strcmp(args[argcount-1], "-u")!=0){ //if 8 args check if last one is "-u"
            printf("Incorrect arguments for getfiles\n Usage: gettargz <extension list> <-u>\n");
            return -1;
        }else if(argcount < 2 || argcount > 8){ //args between 2 to 7
            printf("Incorrect arguments for getfiles\n Usage: gettargz <extension list> <-u>\n");
            return -1;
        }else if(argcount == 2 && strcmp(args[argcount-1], "-u")==0){   //if args 2, 2nd arg should not be "-u"
            printf("Incorrect arguments for getfiles\n Usage: gettargz <extension list> <-u>\n");
            return -1;
        }
        int uflag = 0;  //uflag set to 0 if "-u" not present
        if(strcmp(args[argcount-1], "-u")==0){
            uflag = 1;  //uflag set to 0 if "-u" present
        }
        //command input is correct  
        servegettargz(buffer, argcount, uflag, sockfd); //serve gettargz command
        return 0;
    }else if(strcmp(args[0], "quit")==0){
        writesocket("quit", 4, sockfd); //let server know client quitting
        close(sockfd);  //close fd
        exit(EXIT_SUCCESS);
    }else{  //Incorrect command
        printf("Incorrect command\n");
        printf("Please use one of the following commands:\nfindfile, sgetfiles, dgetfiles, getfiles, gettargz\n");
        return -1;
    }
}

int main(int argc, char *argv[]){
    if(argc != 4){  //args need ip address and port no of server and mirror
        fprintf(stderr,"invalid args\nUsage: ./client [ip address] [server port number] [mirror port number]\n");
        exit(EXIT_FAILURE);
    }

    //server socket
    //the below lines of code set all the variables need for a socket connection and make a connection request to server
    int sockfd, newsockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[1024];
    portno = atoi(argv[2]);
    sockfd = socket (AF_INET, SOCK_STREAM, 0);
    if(sockfd< 0)
        error("Error opening Socket\n");
    server = gethostbyname(argv[1]);
    if(server == NULL)
    {
        fprintf(stderr, "Error, no such host");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)    //connectect request to server
        error("Connection failed");

    //mirror socket
    //the below lines of code set all the variables need for a socket connection and make a connection request to mirror
    int sockfd2, newsockfd2, portno2, n2;
    struct sockaddr_in serv_addr2;
    struct hostent *server2;
    char buffer2[1024];
    portno2 = atoi(argv[3]);
    sockfd2 = socket (AF_INET, SOCK_STREAM, 0);
    if(sockfd2< 0)
        error("Error opening Socket\n");
    server2 = gethostbyname(argv[1]);
    if(server2 == NULL)
    {
        fprintf(stderr, "Error, no such host");
    }
    bzero((char *) &serv_addr2, sizeof(serv_addr2));
    serv_addr2.sin_family = AF_INET;
    bcopy((char *) server2->h_addr, (char *) &serv_addr2.sin_addr.s_addr, server2->h_length);
    serv_addr2.sin_port = htons(portno2);
    if(connect(sockfd2, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2))<0)    //connectect request to mirror
        error("Connection failed");


    int sockfd_final;
    sleep(1);
    if(read(sockfd, buffer, 1024) > 0){ //this condition means that server has accepted the client
        sockfd_final = sockfd;
        printf("%s\n", buffer); //priniting where the client got connected
        close(sockfd2); //close fd of mirror
    }else if(read(sockfd2, buffer2, 1024) > 0){ //this condition if server didnt accept the client, but mirror accepted
        sockfd_final = sockfd2;
        printf("%s\n", buffer2);    //priniting where the client got connected
        close(sockfd);  //close fd of server
    }else{  //this condition if neither server or mirror accepted client
        printf("Error connecting\n");
        exit(EXIT_FAILURE);
    }
    
    //while loop for client to input commands
    while(1){   
        bzero(buffer, 1024);
        printf("C$ ");
        fgets(buffer, 1024, stdin); //store imput command in buffer string 
        checkcmd(buffer, sockfd_final);   //check if the command is correct
    }

    close(sockfd);
    return 0;
}