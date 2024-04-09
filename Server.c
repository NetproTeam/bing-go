#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<netinet/in.h>
#include<stdbool.h>

#define BUF_SIZE 100
#define MAX_CLNT 2

int serv_sock;

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int cur = 0;

volatile bool is_locked = false;
pthread_mutex_t mutx;

// statuses: -2: 빙고 안됨, -1: 빙고 성공, -3: 상대방 나감
int statuses[2] = {-2,-2};

int turns[2] = {0,0};

void lock_mutex();
void unlock_mutex();

void * handle_clnt(void *arg);

void error_handling(char *message);

int main(int argc,char *argv[]){
    int clnt_sock;
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
    }
    if(listen(serv_sock, 5) == -1){
        error_handling("listen() error");
    }
    pthread_mutex_init(&mutx, NULL);

    while(1){
        clnt_addr_sz = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_sz);
        if (clnt_cnt == MAX_CLNT){
            write(clnt_sock, "-5", 3);
            close(clnt_sock);
            continue;
        }
        clnt_socks[clnt_cnt++] = clnt_sock;

        if (clnt_cnt == 2){
            write(clnt_socks[0], "0", 2);
            write(clnt_socks[1], "-1", 3);
        }

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    }

    pthread_mutex_destroy(&mutx);
    printf("Server closed\n");
    close(serv_sock);
    return 0;
}   

void send_msg(char *msg,int len){
    int i;
    char temp[BUF_SIZE];
    
    for(i=0; i<clnt_cnt; i++){
        write(clnt_socks[i], msg, len);
    }
}

void * handle_clnt(void *arg){
    int clnt_sock = *((int*) arg);

	// find cur idx
    int idx = 0;
    for (int i = 0; i < clnt_cnt; i++){
        if(clnt_sock == clnt_socks[i]){
            idx = i;
            break;
        }
    }

    int str_len = 0;
    char msg[BUF_SIZE];
    while((str_len=read(clnt_sock,msg,sizeof(msg)))!= 0){
        int i = atoi(msg);
        
        lock_mutex();
        if(statuses[idx] == -1){
            close(clnt_sock);
            break;
        }

        // 부전승 시에 클라이언트에 -4 보냄
        if(statuses[idx] == -3){
            write(clnt_sock, "-4", 3);
            close(clnt_sock);
            break;
        }

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
            // 승리 시 -2, 패배 시 -3
            turns[idx] += 1;
            if(i == -1){
                write(clnt_sock, "-2", 3);
                if (statuses[(idx+1)%2] == -2 && turns[idx] == turns[(idx+1)%2]){
                    write(clnt_socks[(idx+1)%2], "-3", 3);
                    statuses[(idx+1)%2] = -1;
                }
                
                statuses[idx] = -1;
                break;
            }
            else{
                if (statuses[(idx+1)%2] == -1){
                    write(clnt_sock, "-3", 3);
                    break;
                }
            }
        }
        unlock_mutex();
    }
    unlock_mutex();
    
    // 상대방 클라이언트 부전승 메세지 전송
    int curStatus = statuses[idx];
    close(clnt_sock);
    clnt_cnt -= 1;
    if (clnt_cnt == 1 && curStatus != -1){
        statuses[(idx+1)%2] = -3;
        return NULL;
    }
    else if (clnt_cnt == 0){
        pthread_mutex_destroy(&mutx);
        printf("Server closed\n");
        close(serv_sock);
        exit(0);
    }
}

void lock_mutex() {
    pthread_mutex_lock(&mutx);
    is_locked = true;
}

void unlock_mutex() {
    if (__sync_bool_compare_and_swap(&is_locked, true, false)) {
        pthread_mutex_unlock(&mutx);
    }
}

//message를 stderr 형태로 나타냄
void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}
