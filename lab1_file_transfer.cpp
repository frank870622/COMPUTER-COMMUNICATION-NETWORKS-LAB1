#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <iostream>
using namespace std;

#define ERR_EXIT(m)         \
    do                      \
    {                       \
        perror(m);          \
        exit(EXIT_FAILURE); \
    } while (0)

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void tcp_recv(char *, int);
void tcp_send(char *, int, char *);
void udp_recv(char *, int);
void udp_send(char *, int, char *);

int main(int argc, char *argv[])
{
    string protocal = argv[1]; //tcp or udp
    string type = argv[2];     //send or recv

    if (protocal == "tcp")
    {
        if (type == "send")
        { //tcp sender (client)
            tcp_send(argv[3], atoi(argv[4]), argv[5]);
        }
        else if (type == "recv")
        { //tcp receiver (server)
            tcp_recv(argv[3], atoi(argv[4]));
        }
        else
        {
            cout << "type error:" << type << endl;
        }
    }
    else if (protocal == "udp")
    {
        if (type == "send")
        { //udp sender (client)
            udp_send(argv[3], atoi(argv[4]), argv[5]);
        }
        else if (type == "recv")
        { //udp sender (server)
            udp_recv(argv[3], atoi(argv[4]));
        }
        else
        {
            cout << "type error:" << type << endl;
        }
    }
    else
    {
        cout << "protocal error: " << protocal << endl;
    }
    return 0;
}

void tcp_recv(char *ip, int port)
{
    //TCP receiver to ask sender to send file
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = port;                            //port number
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //initail socket
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(ip); //get the input ip address
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    //connect TCP
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    //timer declare
    time_t rawtime;
    struct tm *timeinfo;

    // this array to receive data size
    char sizebuffer[128];
    bzero(sizebuffer, 128);

    //this array to receive file name
    char namebuffer[128];
    memset(namebuffer, 0, 128);

    //this array to store file name without directory name
    char filename[128];
    memset(filename, 0, 128);

    //this array to receive file data
    char recvbuffer[1024];
    memset(recvbuffer, 0, 1024);

    int filesize;          //a integer to store the entire file size
    int nowrecvsize = 0;   //a integer to store the received file size
    float nowsendpart = 0; //a integer to store the sended % of file

    //create output directory
    mkdir("output", 0777);

    //ask sender to send file name
    n = write(sockfd, "give me the file name", strlen("give me the file name"));

    //receive the file name
    n = read(sockfd, namebuffer, 128);

    //ask sender to send file size
    n = write(sockfd, "give me the file size", strlen("give me the file size"));

    //receive file size
    n = read(sockfd, sizebuffer, 128);
    if (n <= 0)
        printf("recv: error receive data size");
    filesize = atoi(sizebuffer); //store file size into local integer

    //ask sender to send file data
    n = write(sockfd, "give me the file", strlen("give me the file"));

    //create a new file to store
    sprintf(filename, "output/%s", strrchr(namebuffer, '/') == nullptr ? namebuffer : strrchr(namebuffer, '/') + 1);
    int to = creat(filename, 0777); //create file at output/filename
    if (to < 0)
    {
        cout << "Error creating destination file\n";
    }

    //receive the file data
    while (nowrecvsize < filesize) //received file size < entire file size, it means the transport not finish
    {
        //every 5 % send system time
        if ((float)nowrecvsize / (float)filesize * 100 - nowsendpart >= 5)
        {
            //use timer to get the system time
            time(&rawtime);
            timeinfo = localtime(&rawtime);

            //print the transport % and system time
            cout << (float)nowrecvsize / (float)filesize * 100 << "\% " << asctime(timeinfo);
            nowsendpart = (float)nowrecvsize / (float)filesize * 100;
        }

        n = read(sockfd, recvbuffer, 1024);                                        //receiver file data
        n = write(to, recvbuffer, sizeof(recvbuffer));                             //write data to the new file
        n = write(sockfd, "i get a part of file", strlen("i get a part of file")); //send chech message to sender
        nowrecvsize += 1024;
    }

    //after receive file , print the 100% and system time
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    cout << "100\% " << asctime(timeinfo);

    //send check data to sender after file transport
    n = write(sockfd, "i get the file", strlen("i get the file"));

    close(to);
    close(sockfd);
}
void tcp_send(char *ip, int port, char *filename)
{
    //tcp sender to wait receiver to ask
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    bzero((char *)&serv_addr, sizeof(serv_addr));

    portno = port;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //initial socket
    if (sockfd < 0)
        error("ERROR opening socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);                                  //listen to connect from tcp
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *)&cli_addr,
                       &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    //this array to receive data
    char recvbuffer[256];
    bzero(recvbuffer, 256);

    //this array to send file data
    char sendbuffer[1024];
    bzero(sendbuffer, 1024);

    //this array to send file size
    char sizebuffer[128];
    bzero(sizebuffer, 128);

    //this array to save file name
    char namebuffer[128];
    memset(namebuffer, 0, 128);
    sprintf(namebuffer, "%s", filename); //store filename to local character array

    int filesize;        //a integer to store the entire file size
    int nowsendsize = 0; //a integer to store the sent file size

    //open file whick want to send
    FILE *File = fopen(filename, "rb");

    //get filesize and store in a local integer
    fseek(File, 0, SEEK_END);
    filesize = ftell(File);
    fseek(File, 0, SEEK_SET);

    //read file name ask from receiver
    int n = read(newsockfd, recvbuffer, 255);

    //send file name to recevier
    n = write(newsockfd, namebuffer, strlen(namebuffer));

    //read check data from receiver after sending file name
    n = read(newsockfd, recvbuffer, 255);

    //send data size to receiver
    sprintf(sizebuffer, "%d", filesize); //copy file size to sizebuffer
    n = write(newsockfd, sizebuffer, strlen(sizebuffer));
    if (n <= 0)
    {
        printf("error send filesize\n");
    }

    //read check data from receiver after sending file size
    n = read(newsockfd, recvbuffer, 255);

    //send file to receiver
    while (fread(sendbuffer, 1, 1024, File) >= 0 && nowsendsize < filesize)
    {
        n = write(newsockfd, sendbuffer, 1024); //send the file data to receiver
        n = read(newsockfd, recvbuffer, 255);   //receive the check data from receiver
        nowsendsize += 1024;
    }

    //read check data from receiver after sending entire file
    n = read(newsockfd, recvbuffer, 255);

    //TCP data transfer is over
    printf("TCP data transfer is over\n");

    fclose(File); //close file
    close(newsockfd);
    close(sockfd);
}
void udp_recv(char *ip, int port)
{
    //udp code given by TA
    int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    //timer initail
    time_t rawtime;
    struct tm *timeinfo;

    //this array to save the file name
    char namebuffer[128];
    memset(namebuffer, 0, 128);

    //this array to save the file size
    char sizebuffer[256];
    memset(sizebuffer, 0, 256);

    //this array to receive the file data
    char recvbuffer[1024];
    memset(recvbuffer, 0, 1024);

    //this array to store file name without directory name
    char filename[128];
    memset(filename, 0, 128);

    int filesize;          //a integer to store the entire file size
    int nowrecvsize = 0;   //a integer to store the received file size
    float nowsendpart = 0; //a integer to store the sended % of file

    //create output directory
    mkdir("output", 0777);

    //ask sender to send file name
    sendto(sock, "give me the file name", strlen("give me the file name"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //receive file name from sender
    int n = recvfrom(sock, namebuffer, sizeof(namebuffer), 0, NULL, NULL);

    //ask sender to send file size
    sendto(sock, "give me the file size", strlen("give me the file size"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //receive the file size from sender
    n = recvfrom(sock, sizebuffer, sizeof(sizebuffer), 0, NULL, NULL);
    filesize = atoi(sizebuffer); //store file size to a local integer

    //ask sender to send file
    sendto(sock, "give me the file", strlen("give me the file"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //create a new file to store
    sprintf(filename, "output/%s", strrchr(namebuffer, '/') == nullptr ? namebuffer : strrchr(namebuffer, '/') + 1);
    int to = creat(filename, 0777);
    if (to < 0)
    {
        cout << "Error creating destination file\n";
    }

    //receive file data
    while (nowrecvsize < filesize) //received file size < entire file size, it means the transport not finish
    {
        //every 5 % send system time
        if ((float)nowrecvsize / (float)filesize * 100 - nowsendpart >= 5)
        {
            //use timer to get system time
            time(&rawtime);
            timeinfo = localtime(&rawtime);

            //print the transport % and system time
            cout << (float)nowrecvsize / (float)filesize * 100 << "\% " << asctime(timeinfo);
            nowsendpart = (float)nowrecvsize / (float)filesize * 100;
        }

        n = recvfrom(sock, recvbuffer, 1024, 0, NULL, NULL);                                                                         //receive file data
        n = write(to, recvbuffer, sizeof(recvbuffer));                                                                               //write data to the new file
        n = sendto(sock, "i get a part of file", strlen("i get a part of file"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)); //send check data to sender
        nowrecvsize += 1024;
    }

    //after receive file , print the 100% and system time
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    cout << "100\% " << asctime(timeinfo);

    //send check data to sender after file transport
    n = sendto(sock, "i got the file", strlen("i got the file"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    close(to);
    close(sock);
}
void udp_send(char *ip, int port, char *filename)
{
    //udp code given by TA
    int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket error");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind error");

    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    peerlen = sizeof(peeraddr);

    //this array to receive data
    char recvbuffer[1024];
    memset(recvbuffer, 0, sizeof(recvbuffer));

    //this array to send file size
    char sizebuffer[256] = {0};
    memset(sizebuffer, 0, sizeof(sizebuffer));

    //this array to send file data
    char sendbuffer[1024];
    memset(sendbuffer, 0, 1024);

    //this array to send file name
    char namebuffer[128];
    memset(namebuffer, 0, 128);
    sprintf(namebuffer, "%s", filename);

    int filesize;        //a integer to store the entire file size
    int nowsendsize = 0; //a integer to store the sent file size

    //open file which want to send
    FILE *File = fopen(filename, "rb");

    //get filesize and save into a local integer
    fseek(File, 0, SEEK_END);
    filesize = ftell(File);
    fseek(File, 0, SEEK_SET);

    //receive the file name ask from reciever
    int n = recvfrom(sock, recvbuffer, sizeof(recvbuffer), 0,
                     (struct sockaddr *)&peeraddr, &peerlen);

    //send file name to receiver
    n = sendto(sock, namebuffer, sizeof(namebuffer), 0,
               (struct sockaddr *)&peeraddr, peerlen);

    //receive the file size ask from receiver
    n = recvfrom(sock, recvbuffer, sizeof(recvbuffer), 0,
                 (struct sockaddr *)&peeraddr, &peerlen);

    //send file size to receiver
    sprintf(sizebuffer, "%d", filesize);
    n = sendto(sock, sizebuffer, sizeof(sizebuffer), 0,
               (struct sockaddr *)&peeraddr, peerlen);

    //receive the file ask from receiver
    n = recvfrom(sock, recvbuffer, sizeof(recvbuffer), 0,
                 (struct sockaddr *)&peeraddr, &peerlen);

    //send file data
    while (fread(sendbuffer, 1, 1024, File) >= 0 && nowsendsize < filesize)
    {
        n = sendto(sock, sendbuffer, 1024, 0,
                   (struct sockaddr *)&peeraddr, peerlen); //send file data to receiver
        n = recvfrom(sock, recvbuffer, sizeof(recvbuffer), 0,
                     (struct sockaddr *)&peeraddr, &peerlen); //receive check data from receiver
        nowsendsize += 1024;
    }

    //receiver the reply from receiver after file transport
    n = recvfrom(sock, recvbuffer, sizeof(recvbuffer), 0,
                 (struct sockaddr *)&peeraddr, &peerlen);

    printf("udp send file is over\n");
    fclose(File);
    close(sock);
}
