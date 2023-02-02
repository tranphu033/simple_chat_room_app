#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h> 
#include "define.h"

#define TIMEOUT 5
#define N 26 //tong so ky tu bang chu cai tieng Anh

void ceasar_encrypt(char *str, int key); //ma hoa Ceasar
void ceasar_decrypt(char *str, int key); //giai ma Ceasar

int main(int argc, char * argv[]) {
    int net_sock;
    char buff[BUFF_SIZE], uname[60];
    message msg, fmsg;
    int bytes_recv;
    int key=0; //khoa Ceasar

    if(argc!=3) {
		printf("Usage: ./client IPAddress PortNumber\n");
		exit(0);
	}
    //Construct socket:
    net_sock=socket(AF_INET,SOCK_STREAM,0);
    
    //specify an address for the socket:
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2]));
    server_address.sin_addr.s_addr=inet_addr(argv[1]);
    
    int connection_status = connect(net_sock, (struct sockaddr *) & server_address, sizeof(server_address));
    //check for connection_status:
    if(connection_status==-1){
        printf("The connection has error\n\n");
        close(net_sock);
        return 0;
    }

    fd_set readfds;
    int maxfd, i, len, key_t; 
    char uname_t[60];
    
    FD_ZERO(&readfds); 
    /*FD_ZERO(&writefds);
    FD_SET(STDOUT_FILENO, &writefds);*/
    
    strcpy(buff, " You have joined the chat!\n>Enter your nickname: ");
    write(STDOUT_FILENO, buff, strlen(buff));
    //select(STDOUT_FILENO+1, NULL, &writefds, NULL, NULL); 

    fgets(uname, sizeof(uname), stdin);
    uname[strlen(uname)-1]='\0';

    strcpy(buff,">Let's type message to chat:\n");
    write(STDOUT_FILENO, buff, strlen(buff));
    
    //hien thi noi dung lich su chat:
    while(1){
    if(recv(net_sock, &fmsg, sizeof(message), 0) <= 0){
        perror("\nError: ");
        close(net_sock);
        return 0;
    }
    if(fmsg.code == 'f') break; //ket thuc nhan du lieu tu file
    else if(fmsg.code == 't'){
        len=strlen(fmsg.payld);
        sscanf(fmsg.payld, "%d %s", &key_t, uname_t); //lay khoa va ten nguoi tham gia chat

        for(i=0;i<len;i++){
            if(fmsg.payld[i] == '>') break;
        }
        strcpy(buff, fmsg.payld+i+1); //lay noi dung chat(o dang ma hoa)
        ceasar_decrypt(buff, key_t); //giai ma noi dung chat theo key tuong ung
        //hien thi:
        printf("   %s:%s", uname_t, buff);
    }
    }
    while (1){
        
        FD_SET(STDIN_FILENO, &readfds); //=1
        FD_SET(net_sock, &readfds); //maxfd
        maxfd=net_sock;

        select(maxfd+1, &readfds, NULL, NULL, NULL);
        
        if(FD_ISSET(net_sock, &readfds)){ //co du lieu tu server gui den
            bytes_recv=recv(net_sock, &msg, sizeof(message), 0);
            if(bytes_recv <= 0){
                perror("\nError: "); break;
            }
            if(msg.code=='0'){
            ceasar_decrypt(msg.payld, key); //giai ma  
            if(strcmp(msg.uname, uname) == 0) strcpy(msg.uname,"You");           
            printf("   %s:%s\n", msg.uname, msg.payld); //hien thi noi dung chat            
            }
            else if(msg.code=='1'){
                key=msg.key; //nhan key moi
            }
        }
        if (FD_ISSET(STDIN_FILENO, &readfds)){ //co du lieu nhap vao tu ban phim 
            len = read(STDIN_FILENO, buff, sizeof(buff)); //lay du lieu nhap vao
            buff[len-1] = '\0';
            ceasar_encrypt(buff, key); //ma hoa 

            strcpy(msg.uname, uname);
            strcpy(msg.payld,buff);            
            msg.code='0';

            if(send(net_sock, &msg, sizeof(msg), 0) < 0){ //gui noi dung chat cho server
                perror("\nError: "); break;
            }
        }   
        FD_ZERO(&readfds);
    }
    // close the socket
    close(net_sock);
    return 0;
}

void ceasar_encrypt(char *str, int key){
    int i,num;
    for(i=0;i<strlen(str);i++){
        if(str[i]>='a' && str[i]<='z'){
            num=str[i]-97;
            str[i]= (char)((num+key) % N) + 97;
        }
        else if(str[i]>='A' && str[i]<='Z'){
            num=str[i]-65;
            str[i]= (char)((num+key) % N) + 65;
        }
    }
}

void ceasar_decrypt(char *str, int key){
    int i,num, tmp;
    for(i=0;i<strlen(str);i++){
        if(str[i]>='a' && str[i]<='z'){
            num=str[i]-97;
            tmp=num-key;
            if(tmp<0) tmp=tmp+N;
            str[i]= (char)(tmp % N) + 97;
        }
        else if(str[i]>='A' && str[i]<='Z'){
            num=str[i]-65;
            tmp=num-key;
            if(tmp<0) tmp=tmp+N;
            str[i]= (char)(tmp % N) + 65;
        }
    }
}