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

// statuses: -2: 빙고 안됨, -1: 빙고 성공, -3: 상대방 나감, -4: 게임 종료
#define NO_BINGO_STATUS -2
#define BINGO_STATUS -1
#define OPPONENT_EXIT_STATUS -3
#define GAME_OVER_STATUS -4

// signals => 빙고 안됨: 0, 승리: -2, 패배: -3, 무승부: -4
#define WIN_SIGNAL -2
#define LOSE_SIGNAL -3
#define DRAW_SIGNAL -4

int serv_sock;

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
int cur = 0;

volatile bool is_locked = false;
pthread_mutex_t mutx;

int statuses[2] = { NO_BINGO_STATUS, NO_BINGO_STATUS };

int turns[2] = { 0, 0 };

void open_server_socket(char *port);
int connect_client();
void send_num(int clnt_sock, int num);
void send_msg_to_all_clnt(char *msg,int len);

void lock_mutex();
void unlock_mutex();
void clean_up();

void *handle_clnt(void *arg);
int check_winner_and_send_signal(int idx);
int get_idx_by_sock(int clnt_sock);
int get_opponent(int idx);

void error_handling(char *message);

int main(int argc,char *argv[]){
    int clnt_sock;
    pthread_t t_id;

    pthread_mutex_init(&mutx, NULL);

    if (argc != 2){
        clean_up();
        printf("Usage: %s <PORT>\n",argv[0]);
        exit(1);
    }
    open_server_socket(argv[1]);

    while(1){
        if ((clnt_sock = connect_client()) == -1) {
            continue;
        }

        if (clnt_cnt == 2){
            send_num(clnt_socks[0], 0);
            send_num(clnt_socks[1], -1);
        }

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
    }

    clean_up();
    printf("Server closed\n");
    return 0;
}   

void * handle_clnt(void *arg){
    int clnt_sock = *((int*) arg);
    int idx = get_idx_by_sock(clnt_sock);
    int opponent_idx = get_opponent(idx);

    int str_len = 0;
    char msg[BUF_SIZE];
    while((str_len = read(clnt_sock, msg, sizeof(msg))) !=  0){
        int i = atoi(msg);
        
        lock_mutex();
        if(statuses[idx] == GAME_OVER_STATUS) {
            break;
        } else if(statuses[idx] == OPPONENT_EXIT_STATUS) {
            send_num(clnt_sock, WIN_SIGNAL);
            break;
        }

        if (i > 0) {
            if (cur == idx) {
                send_msg_to_all_clnt(msg, str_len);
                cur = (cur + 1) % clnt_cnt;
            } else {
                write(clnt_sock, "상대방의 턴입니다\n", 27);
            }
        } else if (i < 0) {
            turns[idx] += 1;
            if (i == -1) {
                statuses[idx] = BINGO_STATUS;
            }

            if (turns[idx] == turns[opponent_idx] && check_winner_and_send_signal(idx) != 0) {
                statuses[idx] = GAME_OVER_STATUS;
                statuses[opponent_idx] = GAME_OVER_STATUS;
                break;
            }
        }
        unlock_mutex();
    }
    unlock_mutex();
    
    close(clnt_sock);
    clnt_cnt -= 1;
    if (clnt_cnt == 0) {
        clean_up();
        printf("Server closed\n");
        exit(0);
    } else if (statuses[idx] != BINGO_STATUS){
        statuses[opponent_idx] = OPPONENT_EXIT_STATUS;
    }
    return NULL;
}

int check_winner_and_send_signal(int idx) {
    int opponent_idx = get_opponent(idx);
    if (statuses[idx] != BINGO_STATUS && statuses[opponent_idx] != BINGO_STATUS) {
        return 0;
    }

    if (statuses[idx] == BINGO_STATUS && statuses[opponent_idx] == BINGO_STATUS) {
        send_num(clnt_socks[idx], DRAW_SIGNAL);
        send_num(clnt_socks[opponent_idx], DRAW_SIGNAL);
    } else if (statuses[idx] == BINGO_STATUS) {
        send_num(clnt_socks[idx], WIN_SIGNAL);
        send_num(clnt_socks[opponent_idx], LOSE_SIGNAL);
    } else if (statuses[opponent_idx] == BINGO_STATUS) {
        send_num(clnt_socks[idx], LOSE_SIGNAL);
        send_num(clnt_socks[opponent_idx], WIN_SIGNAL);
    }
    return 1;
}

int get_idx_by_sock(int clnt_sock){
    for (int i = 0; i < clnt_cnt; i++){
        if(clnt_sock == clnt_socks[i]){
            return i;
        }
    }
    return -1;
}

int get_opponent(int idx) {
    if (idx < 0) return -1;
    return (idx + 1) % 2;
}

void open_server_socket(char *port){
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(port));

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("bind() error");
    }
    if(listen(serv_sock, 5) == -1){
        error_handling("listen() error");
    }
}

int connect_client() {
    struct sockaddr_in clnt_addr;
    int clnt_addr_sz = sizeof(clnt_addr);
    int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_sz);

    if(clnt_sock == -1) {
        return -1;
    }

    if (clnt_cnt == MAX_CLNT){
        send_num(clnt_sock, -5);
        close(clnt_sock);
        return -1;
    }
    
    clnt_socks[clnt_cnt] = clnt_sock;
    clnt_cnt += 1;
    
    printf("Connected client IP: %s \n", inet_ntoa(clnt_addr.sin_addr));
    return clnt_sock;
}

void send_num(int clnt_sock, int num) {
    char msg[100];
    sprintf(msg, "%d", num);
    write(clnt_sock, msg, strlen(msg));
}

void send_msg_to_all_clnt(char *msg,int len) {
    int i;
    char temp[BUF_SIZE];
    
    for(i=0; i<clnt_cnt; i++){
        write(clnt_socks[i], msg, len);
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

void clean_up() {
    for (int i = 0; i < MAX_CLNT; i++) {
        close(clnt_socks[i]);
    }
    close(serv_sock);
    pthread_mutex_destroy(&mutx);
}

//message를 stderr 형태로 나타냄
void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n',stderr);
    exit(1);
}
