#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
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
    /*
    //send error when input parameter number < 5
    if (argc < 5) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    */
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
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = port;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(ip);
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
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // this array to store data size
    char sizebuffer[128];
    bzero(sizebuffer, 128);

    //ask sender to send data size and read it
    n = write(sockfd, "give me the file size", strlen("give me the file size"));
    n = read(sockfd, sizebuffer, 128);
    if (n <= 0)
        printf("recv: error receive data size");

    int filesize = atoi(sizebuffer);

    char recvbuffer[filesize];
    bzero(recvbuffer, filesize);

    //ask sender to send data and read it
    n = write(sockfd, "give me the file", strlen("give me the file"));
    n = read(sockfd, recvbuffer, filesize);
    if (n <= 0)
        printf("recv: error receive data size");

    //reply to sender "i get the file"
    n = write(sockfd, "i get the file", strlen("i get the file"));

    //create a new file to store
    int to = creat("output.txt", 0777);
    if (to < 0)
    {
        cout << "Error creating destination file\n";
    }
    n = write(to, recvbuffer, filesize); //write data to the new file

    /*
    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    
    if (n < 0)
        error("ERROR writing to socket");
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s\n", buffer);
    */
    close(to);
    close(sockfd);
}
void tcp_send(char *ip, int port, char *filename)
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = port;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr,
             sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *)&cli_addr,
                       &clilen);
    if (newsockfd < 0)
        error("ERROR on accept");

    //receive buffer
    char recvbuffer[256];
    bzero(recvbuffer, 256);

    //read data from receiver
    n = read(newsockfd, recvbuffer, 255);

    //open file
    FILE *File = fopen(filename, "rb");

    //get datasize
    fseek(File, 0, SEEK_END);
    int datasize = ftell(File);
    fseek(File, 0, SEEK_SET);

    //array to store file
    char sendbuffer[datasize];
    bzero(sendbuffer, datasize);
    n = fread(sendbuffer, 1, datasize, File);
    if (n <= 0)
    {
        printf("error read file\n");
    }

    //array to sastoreve data size
    char sizebuffer[128];
    bzero(sizebuffer, 128);

    //send data size to receiver
    sprintf(sizebuffer, "%d", datasize);
    n = write(newsockfd, sizebuffer, strlen(sizebuffer));
    if (n <= 0)
    {
        printf("error send datasize\n");
    }

    //read data from receiver
    n = read(newsockfd, recvbuffer, 255);

    //send file to receiver
    n = write(newsockfd, sendbuffer, datasize);

    //receive reply from receiver "i get the file"
    n = read(newsockfd, recvbuffer, 255);
    printf("TCP data transfer is over");
    /*
    bzero(buffer, 256);
    
    if (n < 0)
        error("ERROR reading from socket");
    printf("Here is the message: %s\n", buffer);
    n = write(newsockfd, "I got your message", 18);
    if (n < 0)
        error("ERROR writing to socket");
    */
    fclose(File);
    close(newsockfd);
    close(sockfd);
}
void udp_recv(char *ip, int port)
{
    int sock;
    if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    //ask sender to send file size
    sendto(sock, "give me the file size", strlen("give me the file size"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //receive the file size from sender
    char sizebuffer[256];
    memset(sizebuffer, 0, 256);
    int n = recvfrom(sock, sizebuffer, sizeof(sizebuffer), 0, NULL, NULL);
    int filesize = atoi(sizebuffer);

    //ask sender to send file
    sendto(sock, "give me the file", strlen("give me the file"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //create the buffer to get file
    char recvbuffer[filesize];
    memset(recvbuffer, 0, filesize);
    n = recvfrom(sock, recvbuffer, sizeof(recvbuffer), 0, NULL, NULL);

    //sent "i got the file" to sender
    n = sendto(sock, "i got the file", strlen("i got the file"), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

    //create a new file to store
    int to = creat("output.txt", 0777);
    if (to < 0)
    {
        cout << "Error creating destination file\n";
    }
    n = write(to, recvbuffer, filesize); //write data to the new file

    /*
    int ret;
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {

        sendto(sock, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

        
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            ERR_EXIT("recvfrom");
        }

        fputs(recvbuf, stdout);
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }
    */
    close(to);
    close(sock);
}
void udp_send(char *ip, int port, char *filename)
{
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

    char recvbuf[1024] = {0};
    char sizebuffer[256] = {0};
    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int n;

    //open file
    FILE *File = fopen(filename, "rb");

    //get filesize
    fseek(File, 0, SEEK_END);
    int datasize = ftell(File);
    fseek(File, 0, SEEK_SET);
    //store filesize
    memset(sizebuffer, 0, sizeof(sizebuffer));
    sprintf(sizebuffer, "%d", datasize);

    //read file to sendbuffer
    char sendbuffer[datasize];
    memset(sendbuffer, 0, datasize);
    n = fread(sendbuffer, 1, datasize, File);
    if (n <= 0)
    {
        printf("error read file\n");
    }

    peerlen = sizeof(peeraddr);
    memset(recvbuf, 0, sizeof(recvbuf));

    //receive the file ask from receiver
    n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                 (struct sockaddr *)&peeraddr, &peerlen);

    //send file size to receiver
    n = sendto(sock, sizebuffer, sizeof(sizebuffer), 0,
               (struct sockaddr *)&peeraddr, peerlen);
    printf("udp send filesize to receiver\n");

    //receive the reply from receiver
    n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                 (struct sockaddr *)&peeraddr, &peerlen);

    //send file to receiver
    n = sendto(sock, sendbuffer, sizeof(sendbuffer), 0,
               (struct sockaddr *)&peeraddr, peerlen);
    printf("udp send file to receiver\n");

    //receiver the reply from receiver
    n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                 (struct sockaddr *)&peeraddr, &peerlen);

    printf("udp send file is over\n");
    /*
        n = recvfrom(sock, recvbuf, sizeof(recvbuf), 0,
                     (struct sockaddr *)&peeraddr, &peerlen);
        if (n == -1)
        {

            if (errno == EINTR)
                continue;

            ERR_EXIT("recvfrom error");
        }
        else if (n > 0)
        {

            fputs(recvbuf, stdout);
            sendto(sock, recvbuf, n, 0,
                   (struct sockaddr *)&peeraddr, peerlen);
        }
        */

    fclose(File);
    close(sock);
}