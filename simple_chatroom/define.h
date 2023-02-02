#define BUFF_SIZE 1024
typedef struct message{
	char uname[60];
	char code;
	char payld[BUFF_SIZE];
	int key;
}message;