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

void error_handling(char *message);
void printBingo();
void createBingoBoard();
void insertBingo(int xIndex, int yIndex);
int checkBingo();
void* getUserInput(void* arg);
void* receiveData(void* arg);

int bingoBoard[BOARD_SIZE][BOARD_SIZE];
int usedNumber[100];
int usedCnt = 0;


int main(int argc, char *argv[]) {
    pthread_t snd_thread, rcv_thread;
    int sock;
    //빙고판
    int bingoBoard[25];
    //입력 숫자
    int input;
    int init = 0;
    struct sockaddr_in serv_adr;

    //gcc clint.c -o clint          
    //./client 127.0.0.1 9091 //temp ip address

    // if (argc != 3) {
    //     printf("Usage: %s <IP> <PORT>\n", argv[0]);
    // }

    // sock = socket(PF_INET, SOCK_STREAM, 0);
    // if (sock == -1) {
    //     error_handling("socket() error");
    // }
 
    // memset(&serv_adr, 0, sizeof(serv_adr));
    // serv_adr.sin_family = AF_INET; 
    // serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    // serv_adr.sin_port = htons(atoi(argv[2]));

    // if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
    //     error_handling("socket() error");
    //     return 0;
    // }
    // else 
    // {
    //     puts("Connected--------->");
    // }

    // while (1)
    // {
    //     if (init == 0)
    //     {
    //         for (int i = 0; i < 25; i++)
    //         {
    //             scanf("%d", &input);
    //             bingoBoard[i] = input;
    //         }
    //         init = 1;
    //         printBingo(bingoBoard);
    //     }
        
    // }
    
    // close(sock);

    pthread_create(&snd_thread, NULL, getUserInput, (void*)&sock);
    pthread_create(&rcv_thread, NULL, receiveData, (void*)&sock);

    createBingoBoard();
    printBingo();
    insertBingo(2,3);
    printBingo();
    printf("%d\n", checkBingo());
    pthread_join(snd_thread, NULL);
    return 0;
}


void* getUserInput(void* arg) {
    int sock = *((int*) arg);
    char msg[BUF_SIZE];

    while (1) {
        int flag = 0;
        printf("Your turn: ");  
        fgets(msg, BUF_SIZE, stdin);

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

        if (!isValidInput || num < 1 || num > 99) {
            printf("Enter a number between 1 and 99\n");
            continue;
        }

        for (int i = 0; i < usedCnt; i++) {
            if (num == usedNumber[i]) {
                flag = 1;
                break;
            }
        }
        if (flag) {
            printf("Number %d is already entered\n", num);
        } else {
            usedNumber[usedCnt++] = num;
            char numStr[BUF_SIZE];
            sprintf(numStr, "%d", num);
            write(sock, numStr, strlen(numStr));
        }
    }

    return NULL;
}

void* receiveData(void* arg) {
    return NULL;
}

//make 5*5 random bingoboard
void createBingoBoard()
{
    //check dup
    int randomNums[25];
    int dup = 0;

    srand((unsigned int )time(NULL));
    //create 25 non-dup int
    for (int i = 0; i < 25;)
    {
        int num = (rand() % 50) + 1;
        for (int j = 0; j < 25; j++)
        {
            //check dup
            if (randomNums[j] == num)
            {
                dup = 1;
                break;
            }
        }
         //no dup insert
         if (dup == 0)
         {
            randomNums[i] = num;
            i++;
         }
         dup = 0;
    }
    int index = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            bingoBoard[i][j] = randomNums[index];
            index++;

            if(i == j) {
                bingoBoard[i][j] = -1;
            }
        }   
    }
    return;
}

//print bingoboard
void printBingo()
{
    printf("+===============Bingo===============+\n");
    
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (bingoBoard[i][j] <= 0)
            {
                printf("| %4s ", "X");
            }
            else
            {
                printf("| %4d ", bingoBoard[i][j]);
            }            
        }   
        printf("|");
        printf("\n+===================================+\n");
    }
    printf("\n");
    return;
}

int checkBingo() 
{
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

//set bingo number to -1
void insertBingo(int xIndex, int yIndex)
{
    //check out of index
    if (xIndex > BOARD_SIZE || yIndex > BOARD_SIZE)
    {
        return;
    }

    //set to -1
    bingoBoard[xIndex][yIndex] = CHECKED;
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}