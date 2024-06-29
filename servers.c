#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <pthread.h>

//#define MYPORT 5555
#define BACKLOG 10


int cli_sockfd, ser_sockfd;
struct sockaddr_in cli_addr, ser_addr;
int sin_size;
char receive_data[1024];
char* method;
char* file_name;
char* http;

// get the size of the requested file
void get_size(char* path, int* file, unsigned long* file_size){
	(*file) = open(path, O_RDONLY);
	off_t end_pos = lseek((*file), 0, SEEK_END);
	lseek((*file), 0, SEEK_SET);
	(*file_size) = (unsigned long)end_pos;
	close(*file);
}

// send the file to the client
void send_to(char* path, int* file){
	char buff[1024];
	memset(buff, 0, sizeof(buff));
	size_t read_byte;
	FILE *fp = NULL;
	if((fp = fopen(path, "rb")) == NULL){
		perror("open file");
		exit(1);
	}
	while((read_byte = fread(buff, 1, sizeof(buff), fp)) > 0){
		send(cli_sockfd, buff, read_byte, 0);
	}
	signal(SIGPIPE, SIG_IGN);
	fclose(fp);
}

// return the type of the requested file
char* file_type(){
	if(strstr(file_name, ".html") != NULL)
		return "text/html";
	else if(strstr(file_name, ".jpg") != NULL)
		return "image/jpeg";
	else if(strstr(file_name, ".pdf") != NULL)
		return "application/pdf";
	else if(strstr(file_name, ".gif") != NULL)
		return "image/gif";
	else if(strstr(file_name, ".mp3") != NULL)
		return "audio/mpeg";
	return "application/octet-stream";
}

// create a header of the file to be sent to the client to provide the necessary information before sending the file to the client.
void make_header(const unsigned long* file_size, char* path){
	char header[1024];
	memset(header, 0, sizeof(header));
	struct stat st;
	if(lstat(path, &st)){
		perror("stat error");
		exit(1);
	}
	struct tm *tm = gmtime(&st.st_mtime);
	char buf[64];
	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm);
	sprintf(header, "HTTP/1.1 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %lu\r\n"
			"Accept-Ranges: bytes\r\n"
			"Server: http\r\n"
			"Last-Modified: %s\r\n\r\n", file_type(), (*file_size), buf);
    	send(cli_sockfd, header, strlen(header), 0);
	
}

// check the permissions of the file requested by the client, and sends the appropriate response to the client.
void check(char* path, int* file, unsigned long* file_size){
	int permission = R_OK | W_OK;
	if(access(path, permission) == 0){
		get_size(path, file, file_size);
		make_header(file_size, path);
		send_to(path, file);
	}
	else{
		send(cli_sockfd, "HTTP/1.1 404 Not Found\r\n", 24, 0);
	}
}

	
int main(int argc, char* argv[]){
	if(argc != 2){
		fprintf(stderr, "Usage: %s <port #>\n", argv[0]);
		return 1;
	}
	//FILE* file;
	printf("%%myserver <%s>\n", argv[1]);
	
	//server
	// socket
	if((ser_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
		perror("socket");
		exit(1);
	}
	
	bzero(&ser_addr, sizeof(struct sockaddr_in));
	ser_addr.sin_family = AF_INET;
	//ser_addr.sin_port = htons(MYPORT);
	ser_addr.sin_port = htons(atoi(argv[1]));
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind
	if(bind(ser_sockfd, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) == -1){
		perror("bind");
		exit(1);
	}
	
	//listen
	if(listen(ser_sockfd, BACKLOG) == -1){
		perror("listen");
		exit(1);
	}
	
	//accept
	while(1) {
		sin_size = sizeof(cli_addr);
		if((cli_sockfd = accept(ser_sockfd, (struct sockaddr *)&cli_addr, &sin_size)) == -1){
			perror("accept");
			continue;
		}
		// receive data
		if(recv(cli_sockfd, receive_data, sizeof(receive_data)-1, 0) == -1){
			perror("receive");
			exit(1);
		}
		printf("server : got connection from %s\n", receive_data);
		
		// read request message
		method = strtok(receive_data, " ");
		file_name = strtok(NULL, " ");
		http = strtok(NULL, " ");
		printf("method : %s\n", method);
		printf("file : %s\n", file_name);
		printf("http : %s\n", http);
		
		if(file_name != NULL)
			if(strncmp(method, "GET", 3) == 0){
				if(strcmp(file_name,"/") != 0){
					char path[1024];
					int file = 0;
					memset(path, 0, sizeof(path));
					unsigned long file_size = 0;
					if(strcmp(file_name,"/") != 0){
						strcat(path, getenv("PWD"));
						strcat(path, file_name);
						check(path, &file, &file_size);
					}
				}
			}
	}
	close(cli_sockfd);
	close(ser_sockfd);
	return 0;

}

