#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/file.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>
#include<time.h>

#define MAXLINE 511
#define MAX_SOCK 1024   //솔라리스 경우 64
#define MAX_TEXT 3000

char *EXIT_STRING = "exit";
char *START_STRING = "Connected to chat_server \n"; //클라이언트의 환영 메세지
char *CLAP = "**********쿵쿵따***********\n"; //게임 차례(턴) 단어
char *GAMEOVER = "GAMEOVER\n";


int maxfdp1;    //최대 소켓 번호 +1
int num_chat = 0;       //채팅 참가자 수
int clisock_list[MAX_SOCK];     //채팅에 참가한 소켓번호 목록
int listen_sock;        //서버의 리슨 소켓
char *text_list[MAX_TEXT];
int max=0;

//새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr);
//채팅 탈퇴 처리 함수
void removeClient(int s);
// 소켓 생성 및 listen 함수
int tcp_listen(int host, int port, int backlog);
// 에러 출력 함수
void errquit(char *mesg) { perror(mesg); exit(1); }
// 소켓을 넌블록 모드로 변경
int set_nonblock(int sockfd);
// 소켓 블록모드, 넌블록모드 판단
int is_nonblock(int sockfd);
//게임 시작 단어 전송
int send_startword(int num_chat); 
//차례를 CLAP(쿵쿵따메시지) 으로 알림
void send_CLAP(int i); 
// 단어 게임규칙 판단 함수
int compare(char *word, int who, int w);

// 단어 끝글자 저장
void saveEnd(char *mem, char *text);
// 단어 첫글자 저장
void saveStart(char *mem, char *text);

char *text[5] ={ "얼룩말", "안경집", "마우스", "콘센트", "기차표"}; //시작 단어 배열



int main(int argc, char *argv[1]) {

        struct sockaddr_in cliaddr;
        char buf[MAXLINE+1];
        int i, j, nbyte,count, accp_sock, clilen, addrlen;
        int next = 0;
        int word=0;
        int r=1;


        if(argc != 2) {
                printf("사용법 : %s port\n", argv[0]);
                exit(0);
        }

        listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);

        if(listen_sock == -1)
            errquit("tcp_listen fail");

        if(set_nonblock(listen_sock) == -1)
                        errquit("set_nonblock fail");

        for(count=0; ; count++){
            if(count==100000){
                putchar(',');
                fflush(stdout);
                count=0;
            }
            addrlen = sizeof(cliaddr);
            accp_sock = accept(listen_sock, (struct sockaddr *)&cliaddr, &clilen);

            if(accp_sock == -1 && errno != EWOULDBLOCK)
                errquit("accept fail");
            else if(accp_sock > 0){
                clisock_list[num_chat] = accp_sock;

                if(is_nonblock(accp_sock) != 0 && set_nonblock(accp_sock) < 0)
                    errquit("set_nonblock fail");
                addClient(accp_sock, &cliaddr);
                send(accp_sock, START_STRING, strlen(START_STRING), 0);
                printf("%d번째 사용자 ready to game.\n",num_chat);	

		  //첫 단어 보내주기
		   if(num_chat == 2){
	  		    word=send_startword(num_chat);
			    send_CLAP(0);
			
		    }
		
			
        }
	 
         //클라이언트가 보낸 메세지를 모든 클라이언트에게 방송
        for(i=0; i<num_chat; i++) {
             errno = 0;

            nbyte = recv(clisock_list[i], buf, MAXLINE, 0);
                
            if(nbyte == 0) {
                removeClient(i);
                continue;
           }
            else if(nbyte == -1 && errno == EWOULDBLOCK)
                continue;

            //종료문자 처리
            if(strstr(buf, EXIT_STRING) != NULL){
                removeClient(i);
                continue;
             }

            buf[nbyte] = 0;
	
            for(j=0; j<num_chat; j++) {
              send(clisock_list[j], buf, nbyte, 0);
            }
            printf("%s\n", buf);

			
			//규칙판별
			
			next = compare(buf, i,word);
			
			send_CLAP(next);
			
			if(next < 0){
				puts("GAMEOVER");
				
                send(clisock_list[i], "LOSE", strlen("LOSE"), 0);
				send(clisock_list[r-i], "WIN", strlen("WIN"), 0);
				exit(1);
       		}
			
			
			
			
        }
    }

}

int send_startword(int num_chat){
	int word = 0;
	int i, j;
	//char *text[5] ={ "얼룩말", "안경집", "마우스", "콘센트", "기차표"}; //시작 단어 배열
	char s[30], e[30];

	srand((int)time(NULL)); 
   	word = rand()%5+1;
		
	puts("\n");
	printf("**********GAME START***********\n");
	printf("게임 시작 제시어: %s\n", text[word]);

	saveEnd(e, text[word]);
	saveStart(s,text[word]);

	//printf("startmax: %d\n", max);
	printf("시작 제시어의 첫번째 문자: %s\n", s);
	//printf("시작 제시어의 마지막 문자: %s\n", e);

	for(i=0; i<num_chat; i++){
		send(clisock_list[i], text[word], strlen(text[word]), 0 );
	}

	return word;

	
}
void saveEnd(char *mem, char *text){
	snprintf(mem, sizeof(mem), "%c%c%c", text[6], text[7], text[8]);
}

void saveStart(char *mem, char *text){
	snprintf(mem, sizeof(mem), "%c%c%c", text[0], text[1], text[2]);
}

void send_CLAP(int i){
	send(clisock_list[i], CLAP, strlen(CLAP), 0);
}



//문자열 buf -> 이름, 메세지 분리해서 분석
int compare(char *word, int who, int w){
	char ss[30] , ee[30];
	char *copyWord;
	char *msg;
	
	
	strcpy(copyWord, word);

	msg = strtok(copyWord, ":");
	msg = strtok(NULL, ":");	
	
	saveStart(ss, text[w]);
	saveStart(ee, msg);	
	
	if( (ss[0]==ee[0])&&(ss[1]==ee[1])&&(ss[2]==ee[2])){
		printf("입력문자 첫글자>>%s\n", ss);
		printf("서버문자 첫글자>>%s\n", ee);
		printf("맞음\n");	

		if(who == 0)
			return 1;
		else
			return 0;
		
	}		
	else{
		printf("start>>%s\n", ss);
		printf("buf>>%s\n", ee);
		printf("틀림\n");	
		
		return -1;
		
	}
	
	
}


void addClient(int s, struct sockaddr_in *newcliaddr) {
        char buf[20];
        inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
        printf("new client : %s\n",buf);
        clisock_list[num_chat] = s;
        num_chat++;
}

void removeClient(int s) {
        close(clisock_list[s]);
        if(s != num_chat-1) {
                clisock_list[s] = clisock_list[num_chat-1];
        }
        num_chat--;
        printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 = %d\n", num_chat);
}

int is_nonblock(int sockfd){
        int val;
        val = fcntl(sockfd, F_GETFL, 0);

        if(val & O_NONBLOCK)
                return 0;
        return -1;
}

int set_nonblock(int sockfd){
        int val;
        val = fcntl(sockfd, F_GETFL, 0);
        if(fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1)
                return -1;
        return 0;
}

int tcp_listen(int host, int port, int backlog) {
        int sd;
        struct sockaddr_in servaddr;

        sd = socket(AF_INET, SOCK_STREAM, 0);
        if(sd == -1) {
                perror("socket fail");
                exit(1);
        }

        bzero((char *)&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(host);
        servaddr.sin_port = htons(port);
        if(bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                perror("bind fail");
                exit(1);
        }

        listen(sd, backlog);
        return sd;
}
