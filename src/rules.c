/*
    leitura do arquivo de configuracao, lendo apenas as linhas que tenham uma estrutura valida ( sem #, /r,/n)
    retorno: lista dos ips presentes no arquivo
*/

#include "rules.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

Rule* load_rules(const char *filename, int *total_rules){
    FILE* f = fopen(filename, "r");
    if (f == NULL){
        printf("error in open the file");
        *total_rules = -1;
        return NULL;
    }
    printf(" [+] iniciando carregamento das regras\n");
    int capacidade = 2;
    int count = 0;

    Rule *list = malloc(capacidade * sizeof(Rule));
    if(list == NULL){
        printf("[X] erro de memoria inicial");
        fclose(f);
        *total_rules = 0;
        return NULL;
    }

    char buffer[256];
    char ip_str[INET_ADDRSTRLEN];
    while (fgets(buffer, sizeof(buffer), f)){

        if (strchr(buffer, '\n') == NULL && !feof(f)) {     // verifica se tem alguma linha truncada, se tiver ela sera descartada
        int c;
        while ((c = fgetc(f)) != '\n' && c != EOF);
        continue; 
        }

        if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r') continue; 

        int mask = 32;                                      
        char *token1 = strtok(buffer, " :/\t\n\r");
        char *token2 = strtok(NULL, " :/\t\n\r");                                                 // variaveis presentes naquela determinada linha (ip, tipo de regra etc)
        char *token3 = strtok(NULL, " :/\t\n\r");                 
        char *token4 = strtok(NULL, " :/\t\n\r");

        if (token1 == NULL) continue;

        Rule r;
        r.verbose = false;
        
        if(strcmp(token1, "deny") == 0) r.act = DENY;
        else if(strcmp(token1, "tarpit") == 0) r.act = TARPIT;
        else if(strcmp(token1, "allow") == 0) r.act = ALLOW;
        else continue;

        if(token3 == NULL && token2 != NULL){     // caso onde so temos o tipo de regra e o ip

            if(inet_pton(AF_INET, token2, &r.ip) != 1){
                printf("[X] erro na conversao do ip {%s}", token2);
                continue;
            }
            inet_ntop(AF_INET, &r.ip, ip_str, INET_ADDRSTRLEN);
            printf("[RULE %d] act=%d verbose=%d ip=%s mask=%d\n", count, r.act, r.verbose, ip_str, mask);

        } else if (token3 != NULL){             // caso em que tem o verbose

            if(strcmp(token2, "verbose") == 0){ 
                r.verbose = true;

                if (token4 != NULL) mask = atoi(token4);

                if(inet_pton(AF_INET, token3, &r.ip) != 1){
                    printf("[X] erro na conversao do ip {%s}", token3);
                    continue;
                }
            inet_ntop(AF_INET, &r.ip, ip_str, INET_ADDRSTRLEN);
            printf("[RULE %d] act=%d verbose=%d ip=%s mask=%d\n", count, r.act, r.verbose, ip_str, mask);

            } else {
                mask = atoi(token3);
                if(inet_pton(AF_INET, token2, &r.ip) != 1){
                printf("[X] erro na conversao do ip {%s}", token2);
                continue;
                }
                inet_ntop(AF_INET, &r.ip, ip_str, INET_ADDRSTRLEN);
                printf("[RULE %d] act=%d verbose=%d ip=%s mask=%d\n", count, r.act, r.verbose, ip_str, mask);
            }

        } else {
            continue;
        }

        r.mask = mask;
        list[count] = r;
        count++;

        if(count == capacidade){                  // atualizacao do buffer da lista de acordo com a quantidade de regras
            capacidade *= 2;

            Rule *temp = realloc(list, capacidade * sizeof(Rule));
            if(temp == NULL){
                printf("[X] Erro de memoria ao expandir regras (realloc)\n");
                free(list);
                fclose(f);
                *total_rules = 0;
                return NULL;
            }
            list = temp;
        }
    }
    fclose(f);
    *total_rules = count;
    return list;
}