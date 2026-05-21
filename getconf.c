#include "rules.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MAXRULES 50

Rule* load_rules(const char *filename, int *total_rules){
    FILE* f = fopen(filename, "r");
    if (f == NULL){
        printf("error in open the file");
        *total_rules = -1;
        return NULL;
    }

    Rule *list = malloc(MAXRULES * sizeof(Rule));
    if(list == NULL){
        printf("[X] erro de memoria");
        fclose(f);
        return NULL;
    }

    int count = 0;
    char buffer[256];

    while (fgets(buffer, sizeof(buffer), f)){
        if(count >= MAXRULES)break;
        if(buffer[0] == '#' || buffer[0] == '\n') continue;

        char *token1 = strtok(buffer, ": \t\n");
        char *token2 = strtok(NULL, ": \t\n");
        char *token3 = strtok(NULL, ": \t\n");

        if (token1 == NULL) continue;
        Rule r;
        r.verbose = false;

        if(strcmp(token1, "deny") == 0) r.act = DENY;
        else if(strcmp(token1, "tarpit") == 0) r.act = TARPIT;
        else if(strcmp(token1, "allow") == 0) r.act = ALLOW;
        else continue;

        if(token3 == NULL && token2 != NULL){

            if(inet_pton(AF_INET, token2, &r.ip) != 1){
                printf("[X]erro na conversao do ip {%s}", token2);
                continue;
            }

        } else if (token3 != NULL){

            if(strcmp(token2, "verbose") == 0) r.verbose = true;

            if(inet_pton(AF_INET, token2, &r.ip) != 1){
                printf("[X]erro na conversao do ip {%s}", token2);
                continue;
            }

        } else {
            continue;
        }

        list[count] = r;
        count++;
    }
    fclose(f);
    *total_rules = count;
    return list;
}