#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <ctype.h>

#define BUF_SIZE 1024
#define BOARD_SIZE 5
#define CHECKED -1
#define HEARTBEAT_INTERVAL 5

void error_handling(char *message);
void printBingo();
void createManualBingoBoard();
void createBingoBoard();
void insertBingo(int number);
int checkBingo();
void* getUserInput(void* arg);
void* receiveData(void* arg);

int bingoBoard[BOARD_SIZE][BOARD_SIZE];
int usedNumber[100];
int usedCnt = 0;
int turn = 999999;
int server_alive = 0;

void sigalarm_handler(int signal) {
    server_alive = 0;
}

int main(int argc, char *argv[]) {
    pthread_t snd_thread, rcv_thread;
    int sock;
    //입력 숫자
    int input;
    int init = 0;
    struct sockaddr_in serv_adr;
    char msg[BUF_SIZE];

    //gcc clint.c -o clint          
    //./client 127.0.0.1 9091 //temp ip address

    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error_handling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET; 
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("socket() error");
        return 0;
    }
    else 
    {
        puts("Connected--------->");
        createBingoBoard();
        printBingo();
    }

    pthread_create(&snd_thread, NULL, getUserInput, (void*)&sock);
    pthread_create(&rcv_thread, NULL, receiveData, (void*)&sock);

    pthread_join(rcv_thread, NULL);

    signal(SIGALRM, sigalarm_handler);

    alarm(HEARTBEAT_INTERVAL);
    
    close(sock);
    return 0;
}

void* getUserInput(void* arg) {
    int sock = *((int*) arg);
    char msg[BUF_SIZE];

    while (1) {
        int flag = 0; 
        fgets(msg, BUF_SIZE, stdin);

        if (turn != 0) {
            continue;
        }

        size_t len = strlen(msg);
        if (len > 0 && msg[len - 1] == '\n') {
            msg[len - 1] = '\0';
        }

        int isValidInput = 1;
        for (int i = 0; msg[i] != '\0'; i++) {
            if (!isdigit(msg[i])) {
                isValidInput = 0;
                break;
            }
        }
        int num = atoi(msg);

        if (!isValidInput || num < 1 || num > 50) {
            printf("1에서 50 사이의 숫자 입력해주세요\n");
            continue;
        }

        for (int i = 0; i < usedCnt; i++) {
            if (num == usedNumber[i] && turn == 0) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            printf("%d은 이미 입력하셨습니다\n", num);
        } else if (turn == 0) {
            usedNumber[usedCnt++] = num;
            char numStr[BUF_SIZE];
            sprintf(numStr, "%d", num);
            write(sock, numStr, strlen(numStr));
        } else {
            printf("상대턴입니다\n");
            fflush(stdin);
        }
    }

    return NULL;
}

void *receiveData(void *arg) {
    int sock = *((int *)arg);
    char msg[BUF_SIZE];

    while (1) {
        int str_len = read(sock, msg, BUF_SIZE);
        if (str_len == -1) {
            error_handling("read() error");
        } else if (str_len == 0) {
            printf("서버 문제 발생, 종료합니다...\n");
            exit(1);
        }

        msg[str_len] = '\0';


        int isNumber = 1;
        for (int i = 0; msg[i] < str_len; i++) {
            if (!isdigit(msg[i]) && (msg[i] != '-' || i != 0) && (msg[i] != '0' || i != 0)) {
                isNumber = 0;
                break;
            }
        }

        if (isNumber) {
            int number = atoi(msg);

            if(number <= 0) {
                turn = number;
                if (turn == 0) {
                    printf("선공입니다\n"); 
                    printf("당신턴입니다: "); 
                    fflush(stdout);
                } else if (turn == -1){
                    printf("후공입니다\n");
                }

                if (number == -2) {
                    turn = 0;
                    printf("당신이 이겼습니다\n");
                    close(sock);
                    exit(0);
                } 
                else if (number == -3) {
                    printf("당신은 졌습니다\n");
                    close(sock);
                    exit(0);
                } else if (number == -4) {
                    printf("무승부입니다\n");
                    close(sock);
                    exit(0);
                } else if (number == -5) {
                    printf("방이 가득 찼습니다");
                    close(sock);
                    exit(0);
                }
            } else {
                insertBingo(number); 
                int check = checkBingo();
                printBingo();

                if (check >= 3) {
                    write(sock, "-1", sizeof("-1"));
                    turn = 99999;
                } else {
                    write(sock, "-2", sizeof("-2"));
                }

                if (turn == 0) {
                    printf("상대턴입니다\n");
                    turn = -1;
                } else if(turn == -1) {
                    printf("당신턴입니다: "); 
                    fflush(stdout);
                    turn = 0;
                }    
            }
        } else {
            printf("%s\n", msg);
        }
    }

    return NULL;
}

//make 5*5 manual bingo board to draw test
void createManualBingoBoard()
{
    //check dup
    int randomNums[25];
    int dup = 0;

    //create 25 non-dup int
    for (int i = 0; i < 25; i++) {
        randomNums[i] = i + 1;
    }
    int index = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            bingoBoard[i][j] = randomNums[index];
            index++;
        }   
    }
    return;
}

//make 5*5 random bingoboard
void createBingoBoard()
{
    //check dup
    int randomNums[25];
    int dup = 0;

    srand((unsigned int )time(NULL));
    //create 25 non-dup int
    for (int i = 0; i < 25;) {
        int num = (rand() % 50) + 1;
        for (int j = 0; j < 25; j++) {
            //check dup
            if (randomNums[j] == num) {
                dup = 1;
                break;
            }
        }
         //no dup insert
        if (dup == 0) {
            randomNums[i] = num;
            i++;
        }
        dup = 0;
    }
    
    int index = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            bingoBoard[i][j] = randomNums[index];
            index++;
        }   
    }
    return;
}

//print bingoboard
void printBingo() {
    printf("+===============Bingo===============+\n");
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (bingoBoard[i][j] <= 0) {
                printf("| %4s ", "X");
            }
            else {
                printf("| %4d ", bingoBoard[i][j]);
            }            
        }   
        printf("|");
        printf("\n+===================================+\n");
    }
    printf("\n");
    return;
}

int checkBingo() {
    int i, j;
    int result = 0;
    int cnt = 0;

    for(i = 0; i < BOARD_SIZE; i++) {
        cnt = 0;
        for(j = 0; j < BOARD_SIZE; j++) {
            if(bingoBoard[i][j] == CHECKED) {
                cnt++;
            }
        }
        if(cnt == BOARD_SIZE) {
            result++;
        }
    }
    for(j = 0; j < BOARD_SIZE; j++) {
        cnt = 0;
        for(i = 0; i < BOARD_SIZE; i++) {
            if(bingoBoard[i][j] == CHECKED) {
                cnt++;
            }
        }
        if(cnt == BOARD_SIZE) {
            result++;
        }
    }

    cnt = 0;
    for(i = 0; i < BOARD_SIZE; i++) {
        if(bingoBoard[i][i] == CHECKED) {
            cnt++;
        }
    }
    if(cnt == BOARD_SIZE) {
        result++;
    }

    cnt = 0;
    for(i = 0; i < BOARD_SIZE; i++) {
        if(bingoBoard[i][BOARD_SIZE - i - 1] == CHECKED) {
            cnt++;
        }
    }
    if(cnt == BOARD_SIZE) {
        result++;
    }

    return result;
}

void insertBingo(int number) {
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if (bingoBoard[i][j] == number) {
                bingoBoard[i][j] = -1;
                return;
            }
        }
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
