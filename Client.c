#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define BUF_SIZE 1024
#define BOARD_SIZE 5
#define CHECKED -1

void error_handling(char *message);
void printBingo();
void createBingoBoard();
void insertBingo(int xIndex, int yIndex);
int checkBingo();

int bingoBoard[BOARD_SIZE][BOARD_SIZE];


int main(int argc, char *argv[])
{
    int sock;
    //빙고판
    int bingoBoard[25];
    //입력 숫자
    int input;
    int init = 0;
    struct sockaddr_in serv_adr;

    //gcc clint.c -o clint          
    //./client 127.0.0.1 9091 //temp ip address

    // if (argc != 3)
    // {
    //     printf("Usage: %s <IP> <PORT>\n", argv[0]);
    // }

    // sock = socket(PF_INET, SOCK_STREAM, 0);
    // if (sock == -1)
    // {
    //     error_handling("socket() error");
    // }
 
    // memset(&serv_adr, 0, sizeof(serv_adr));
    // serv_adr.sin_family = AF_INET; //tcpip
    // serv_adr.sin_addr.s_addr = inet_addr(argv[1]); //save ip저장
    // serv_adr.sin_port = htons(atoi(argv[2]));   //port 저장

    // if (connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
    // {
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
    createBingoBoard();
    printBingo();
    insertBingo(2,3);
    printBingo();
    printf("%d\n", checkBingo());
    return 0;
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