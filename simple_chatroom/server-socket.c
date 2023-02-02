#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h> 
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "define.h"

#define BACKLOG 20   /* Number of allowed connections */

typedef struct client_{
	int clifd[FD_SETSIZE];
	int maxi;
}client_set;

int process(client_set *clients, int cur_cli); //ham xu ly chinh
int receiveData(int s, message *msg, int size, int flags); //The recv() wrapper function
int sendData(int s, message *msg, int size, int flags); // The send() wrapper function
void *generate_key();

fd_set allset; //tap cac fd_set can check
int key; //khoa Ceasar
client_set clis; //danh sach cac fd cua client
char fname[]="chat_content.txt"; //file chua noi dung lich su chat

int main(int argc, char *argv[]){
	int i, maxfd, listenfd, connfd, sockfd;
	int nready;
	fd_set	readfds;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	pthread_t thr;
	message msg, fmsg;
	char buff[BUFF_SIZE];
	FILE *f;

	if(argc!=2) {
		printf("Usage: ./server PortNumber\n");
		exit(0);
	}

	//xoa noi dung cu cua file fname
	f=fopen(fname,"w");
	fclose(f);

	//Step 1: Construct a TCP socket to listen connection request
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  /* calls socket() */
		perror("\nError: ");
		return 0;
	}

	//Step 2: Bind address to socket
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//servaddr.sin_port        = htons(atoi(argv[1]));
	servaddr.sin_port        = htons(5500);

	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))==-1){ /* calls bind() */
		perror("\nError: ");
		return 0;
	} 

	//Step 3: Listen request from client
	if(listen(listenfd, BACKLOG) == -1){  /* calls listen() */
		perror("\nError: ");
		return 0;
	}

	maxfd = listenfd;			/* initialize */
	clis.maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		clis.clifd[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	
	//tao thread thuc hien: cu 20s gui khoa moi cho cac client
	pthread_create(&thr, NULL, generate_key, NULL);

	//Step 4: Communicate with clients
	while (1) {
		readfds = allset;		/* structure assignment */
		nready = select(maxfd+1, &readfds, NULL, NULL, NULL);
		if(nready < 0){
			perror("\nError: ");
			return 0;
		}
		if (FD_ISSET(listenfd, &readfds)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			if((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
				perror("\nError: ");
			else{
				//gui noi dung lich su chat:
				f=fopen(fname, "r");
				fmsg.code='t'; //dang trong qua trinh truyen du lieu file
				while(1){
					if(fgets(buff, BUFF_SIZE, f) == NULL){
						fmsg.code='f';
					}
					strcpy(fmsg.payld, buff);
					if(sendData(connfd, &fmsg, sizeof(fmsg), 0) < 0){
						close(connfd);
					}
					if(fmsg.code == 'f') break; //ket thuc truyen du lieu file
				}
				fclose(f);

				//gui khoa hien tai cho client moi ket noi den:
				msg.code='1'; //thong diep (msg) chua khoa
				msg.key=key;
				if(sendData(connfd, &msg, sizeof(msg), 0)<0){
					close(connfd);
				}

				printf("You got a connection from %s\n", inet_ntoa(cliaddr.sin_addr)); /* prints client's IP */
				for (i = 0; i < FD_SETSIZE; i++)
					if (clis.clifd[i] < 0) {
						clis.clifd[i] = connfd;	/* save descriptor */
						break;
					}
				if (i == FD_SETSIZE){
					printf("\nToo many clients");
					close(connfd);
				}

				FD_SET(connfd, &allset);/* add new descriptor to set */
				if (connfd > maxfd)
					maxfd = connfd;		/* for select */
				if (i > clis.maxi)
					clis.maxi = i;		/* max index in client[] array */

				//if (--nready <= 0)
				//	continue;
			}
		}

		for (i = 0; i <= clis.maxi; i++) {	/* check all clients for data */
			if ( (sockfd = clis.clifd[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &readfds)) { //co data o 1 sock				
				
				if(process(&clis, sockfd) == 0){
					FD_CLR(sockfd, &allset);
					clis.clifd[i]=-1;
					close(sockfd);
				}
				//if (--nready <= 0)
				//	break;
			}
		}
	}	
	return 0;
}
int process(client_set *clients, int cur_cli){
	message msg;
	int ret, i, tmp_sock;
	FILE *f;

	//nhan tin nhan tu client hien tai:
	ret = receiveData(cur_cli, &msg, sizeof(message), 0);
	if(ret<=0) return 0;
	printf("<recv from %s:'%s'>\n",msg.uname, msg.payld);

	//luu tin nhan vao file lich su chat:
	f=fopen(fname, "a");
	fprintf(f,"%d %s >%s\n", key, msg.uname, msg.payld);
	fclose(f);

	//gui cho cac client khac:
	for (i = 0; i < FD_SETSIZE; i++){
		tmp_sock=clients->clifd[i];
		if (tmp_sock > 0){
			//printf("gui toi clisockID %d\n\n", tmp_sock);
			msg.code='0'; //thong diep chua noi dung chat
			if (sendData(tmp_sock, &msg, sizeof(msg), 0) < 0){
					FD_CLR(tmp_sock, &allset);
					clis.clifd[i]=-1;
					close(tmp_sock);
			}
		}
	}
	return 1;
}

int receiveData(int s, message *msg, int size, int flags){
	int n;
	n = recv(s, msg, size, flags);
	if(n < 0)
		perror("Error: ");
	return n;
}

int sendData(int s, message *msg, int size, int flags){
	int n;
	n = send(s, msg, size, flags);
	if(n < 0)
		perror("Error: ");
	return n;
}

void *generate_key(){
	int i;
	message msg;
	int tmp_sock;
	srand((int)time(0));
    while(1){
        key=1+rand()%25; //sinh khoa ngau nhien
        printf("<New key:%d>\n",key);	
		msg.code='1';
		msg.key=key;	

		for(i=0;i<=clis.maxi;i++){
			tmp_sock=clis.clifd[i];
			if(tmp_sock < 0) continue;
			else{
				if(sendData(tmp_sock, &msg, sizeof(msg), 0) < 0){
					FD_CLR(tmp_sock, &allset);
					clis.clifd[i]=-1;
					close(tmp_sock);
				}
			}
		}
		sleep(20); //cu 20s sinh khoa moi		
    }
}
