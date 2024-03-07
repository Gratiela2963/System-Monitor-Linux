#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_LINES 500
#define MAX_CHAR_PER_LINE 2048

char* getInfofromLine (char infoLine[2048], int column){
    char* infoLineColumns[51];
    int infoLineColumns_length = 0;
    
    char* p = strtok(infoLine, " ");
    while (p != NULL){
        infoLineColumns[infoLineColumns_length] = strdup(p);
        infoLineColumns_length++;
        
        p = strtok(0, " ");
    }

    return infoLineColumns[column];
}

char* searchForLine(char* pid, char arrayToSearchIn[][2048], int arrayLength) {
    int left_margin = 3;
    int right_margin = arrayLength-1; 

    while (left_margin <= right_margin) {
        int middle = (left_margin + right_margin) / 2;
        char copy[2048];
        strcpy(copy, arrayToSearchIn[middle]);
        char* pidFromLine = getInfofromLine(copy, 0);
        int result = atoi(pid) - atoi(pidFromLine);

        if (result == 0) {
            return &arrayToSearchIn[middle];
        } else if (result < 0) {
            right_margin = middle - 1;
        } else {
            left_margin = middle + 1;
        }
    }

    return NULL;
}


int main(){
    int ps_pipe_descriptors[2];
    int iotop_pipe_descriptors[2];
    pid_t ps_pid;
    pid_t iotop_pid;

    char psOutput[MAX_LINES][MAX_CHAR_PER_LINE];
    int psOutputLines = 0;

    char iotopOutput[MAX_LINES][MAX_CHAR_PER_LINE];
    int iotopOutputLines = 0;

    if (pipe(ps_pipe_descriptors) == -1){
        perror("ps pipe");
        exit(EXIT_FAILURE);
    }

    if ((ps_pid = fork()) == -1){
        perror("ps fork failed");
        exit(EXIT_FAILURE);
    }

    if (ps_pid == 0){
        close(ps_pipe_descriptors[0]);

        dup2(ps_pipe_descriptors[1], STDOUT_FILENO);

        execlp("ps", "ps", "aux", (char *)NULL);
        perror("ps execlp");
        exit(EXIT_FAILURE);
    }else{
        close(ps_pipe_descriptors[1]);

        FILE *ps_pipe_stream = fdopen(ps_pipe_descriptors[0], "r");

        while (fgets(psOutput[psOutputLines], MAX_CHAR_PER_LINE, ps_pipe_stream) != NULL
        && psOutputLines < MAX_LINES){
            psOutputLines++;
        }
    }

    if (pipe(iotop_pipe_descriptors) == -1){
        perror("iotop pipe");
        exit(EXIT_FAILURE);
    }

    if ((iotop_pid = fork()) == -1){
        perror("ps fork failed");
        exit(EXIT_FAILURE);
    }

    if (iotop_pid == 0){
        close(iotop_pipe_descriptors[0]);
        
        dup2(iotop_pipe_descriptors[1], STDOUT_FILENO);

        execlp("sudo", "sudo", "iotop", "-P", "-b", "-n", "1", (char *)NULL);
        perror("iotop execlp");
        exit(EXIT_FAILURE);
    }else {
        close(iotop_pipe_descriptors[1]);

        FILE *iotop_pipe_stream = fdopen(iotop_pipe_descriptors[0], "r");

        while (fgets(iotopOutput[iotopOutputLines], MAX_CHAR_PER_LINE, iotop_pipe_stream) != NULL 
        && iotopOutputLines < MAX_LINES){
            iotopOutputLines++;
        }
    }


    printf("%-15s %-10s %-8s %-8s %-12s %-15s %-10s\n", "USER", "PID", "%CPU",
    "%MEM", "DISK READ", "DISK WRITE", "IO/CPU BOUND");  
    
    for (int i = 1; i < psOutputLines; i++){
        char* psLineInfo[51];
        char* iotopLineInfo[51];

        int psInfo_lines = 0;
        int iotopInfo_lines = 0;

        char* p;
        p = strtok(psOutput[i], " ");
        while (p){
            psLineInfo[psInfo_lines] = p;
            psInfo_lines++;
            p = strtok(0, " ");
        }

        char* searchResult = searchForLine(psLineInfo[1], iotopOutput, iotopOutputLines);

        p = strtok(searchResult, " ");
        while (p){
            iotopLineInfo[iotopInfo_lines] = p;
            iotopInfo_lines++;
            p = strtok(0, " ");
        }

        char bound[21] = "";

        if (atoi(psLineInfo[2]) > 90){
            strcpy(bound, "CPU BOUND");
        } else if (atoi(iotopLineInfo[3]) > 85 || atoi(iotopLineInfo[5]) > 85){
            strcpy(bound, "I/O BOUND");
        } else if (strcmp(bound, "") == 0){
            strcpy(bound, "IDLE");
        }

         printf("%-15s %-10s %-8s %-8s %-12s %-15s %-10s\n", psLineInfo[0],
         psLineInfo[1], psLineInfo[2], psLineInfo[3], iotopLineInfo[3],
         iotopLineInfo[5], bound);
    }
    return 0;
}