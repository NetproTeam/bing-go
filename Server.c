#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<netinet/in.h>

#define BUF_SIZE 100
#define MAX_CLNT 2

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int cur = 0;

int statuses[2] = {-2,-2};
int turns[2] = {0,0};

void * handle_clnt(void *arg);

void error_handling(char *message);

int main(int argc,char *argv[]){
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    int clnt_addr_sz;
    pthread_t t_id;

    if (argc != 2){
        printf("Usage: %s <PORT>\n",argv[0]);
        exit(1);
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("bind() error");
        exit(1);
    }
    if(listen(serv_sock, 5) == -1){
        error_handling("listen() error");
        exit(1);
    }

    while(1){
        clnt_addr_sz = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_sz);
        if (clnt_cnt == MAX_CLNT){
            write(clnt_sock, "Sorry, the room is full. Please press q or Q to out\n", 25);
            close(clnt_sock);
            continue;
        }
        clnt_socks[clnt_cnt++] = clnt_sock;
        if (clnt_cnt == 1){
            write(clnt_sock, "당신은 선공입니다\n", 27);
        }
        else{
            write(clnt_sock, "당신은 후공입니다\n", 27);
        }
        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }
    printf("Server closed\n");
    close(serv_sock);
    return 0;
}   

void send_msg(char *msg,int len){
    int i;
    char temp[BUF_SIZE];
    sprintf(temp, "turn: %d %s", cur, msg);
    for(i=0; i<clnt_cnt; i++){
        write(clnt_socks[i], temp, strlen(temp));
    }
}

void * handle_clnt(void *arg){
    int clnt_sock = *((int*) arg);
    int idx = 0;
    for (int i = 0; i < clnt_cnt; i++){
        if(clnt_sock == clnt_socks[i]){
            idx = i;
            break;
        }
    }
    if(statuses[idx] == -1){
        close(clnt_sock);
        return NULL;
    }
    int str_len = 0, i;
    char msg[BUF_SIZE];

    while((str_len=read(clnt_sock,msg,sizeof(msg)))!= 0){
        int i = atoi(msg);
        
        if(i > 0){
            if (cur == idx) {
                send_msg(msg, str_len);
                cur = (cur + 1) % clnt_cnt;
            } else {
                write(clnt_sock, "상대방의 턴입니다\n", 27);
            }
        }
        else if (i < 0) {
            // 빙고 완성 시에 결과 각 클라이언트에게 전송
            turns[idx] += 1;
            if(i == -1){
                write(clnt_sock, "당신이 이겼습니다\n", 27);
                if (statuses[(idx+1)%2] == -2 && turns[idx] == turns[(idx+1)%2]){
                    write(clnt_socks[(idx+1)%2], "당신이 졌습니다\n", 27);
                }
                close(clnt_sock);
                statuses[idx] = -1;
                return NULL;
            }
            else{
                if (statuses[(idx+1)%2] == -1){
                    write(clnt_sock, "당신이 졌습니다\n", 27);
                    close(clnt_sock);
                    return NULL;
                }
            }
        }
    }
    // 클라이언트 종료 시에 해당 클라이언트 소켓을 제거
    // 상대방 클라이언트 부전승 메세지 전송
    for (i=0; i < clnt_cnt; i++){
        if(clnt_sock==clnt_socks[i]){
            while(i < clnt_cnt -1){
                clnt_socks[i] = clnt_socks[i+1];
                statuses[i] = statuses[i+1];
                turns[i] = turns[i+1];
                i += 1;
            }
            break;
        }
    }
    clnt_cnt -= 1;
    if (clnt_cnt == 1){
        write(clnt_socks[0], "상대방이 나가게 되어 당신이 이겼습니다!\n", 58);
        statuses[0] = -1;
        return NULL;
    }
    else if (clnt_cnt == 0){
        printf("Server closed\n");
        return NULL;
    }
}

//message를 stderr 형태로 나타냄
void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}