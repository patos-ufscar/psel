/*
    leitura do arquivo de configuracao, lendo apenas as linhas que tenham uma estrutura valida ( sem #, /r,/n)
    retorno: lista dos ips presentes no arquivo
*/
#define _DEFAULT_SOURCE
#include "rules.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

Rule* load_rules(const char *filename, int *total_rules, char ***forbidden_words, int *forbidden_count){
    *forbidden_words = NULL;
    *forbidden_count = 0;
    
    FILE* f = fopen(filename, "r");
    if (f == NULL){
        printf("[X] Erro ao abir o arquivo de regras!");
        *total_rules = -1;
        return NULL;
    }
    printf(" [+] Iniciando carregamento das regras\n");
    int capacidade = 2;                    //  variavel para a alocação dinâmica das regras
    int count = 0;                      // contador de regras válidas cadastradas

    Rule *list = malloc(capacidade * sizeof(Rule));   // alocação inicial das regras
    if(list == NULL){
        printf("[X] Erro de memoria na lista de Rules");
        fclose(f);
        *total_rules = 0;
        return NULL;
    }

    int fw_capacidade = 4;
    int fw_count = 0;

    char **fw_list = malloc(fw_capacidade * sizeof(char *)); // alocação da lista de palavras proibidas
    if(fw_list == NULL){
        printf("[X] Erro de memoria na lista de forbidden words");

        free(list);
        fclose(f);
        
        *total_rules = 0;
        return NULL;
    }  


    char buffer[256];
    char ip_str[INET_ADDRSTRLEN];
    while (fgets(buffer, sizeof(buffer), f)){    // lê o arquivo linha por lnha

        //proteção contra buffer overflow, caso a linha tenha mais de 256 bytes e não tenha '\n'
        if (strchr(buffer, '\n') == NULL && !feof(f)) { 
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF);       // vai descartando cada caractere dessa linha truncada até chegar no final dela
            continue; 
        }
    
        if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r') continue;  // ignora linhas em branco, quebras de linhas ou comentários
        
        char *token1 = strtok(buffer, " :/\t\n\r");
        if(token1 == NULL) continue;        // erro ao ler o buffer
        
        if(strcmp(token1, "forbidden_words") == 0){
          char *word;
          while((word = strtok(NULL, " :/\t\n\r")) != NULL){   // pega as palavras da lista
              char *word_copy = strdup(word);                   // copia a palavra para um novo buffer

              if(word_copy == NULL){
                  printf("[X] Erro ao copiar a word");
                  continue;
              }
              fw_list[fw_count] = word_copy;            // atualiza a lista
              fw_count++;

              if(fw_count == fw_capacidade){            // realocação de memoria (caso necessario)
                  fw_capacidade *= 2;
                  char **temp = realloc(fw_list, fw_capacidade * sizeof(char *));

                  if(temp == NULL){
                      printf("[X] Erro ao realocar memoria para a forbidden_words");
                      free(word_copy);
                      fw_count--;
                      break;
                  }
                  fw_list = temp;
              }
          }
          continue;  // proxima palavra
        }

        int mask = 32;         // máscara padrão do ipv4 (caso ela não seja especificada)                                 
        char *token2 = strtok(NULL, " :/\t\n\r");                                                 // variáveis presentes naquela determinada linha (ip, tipo de regra, máscara e verbose)
        char *token3 = strtok(NULL, " :/\t\n\r");                 
        char *token4 = strtok(NULL, " :/\t\n\r");

        Rule r;
        r.verbose = false;                    // por padrão o log detalhado começa desligado 

        // verifica qual o tipo da regra
        if(strcmp(token1, "deny") == 0) r.act = DENY;
        else if(strcmp(token1, "tarpit") == 0) r.act = TARPIT;                        
        else if(strcmp(token1, "allow") == 0) r.act = ALLOW;
        else continue;

        if(token3 == NULL && token2 != NULL){     // caso onde só temos o tipo de regra e o ip

            if(inet_pton(AF_INET, token2, &r.ip) != 1){        // converte a string ip para o formato binário
                printf("[X] Erro na conversao do ip {%s}", token2);
                continue;
            }
            inet_ntop(AF_INET, &r.ip, ip_str, INET_ADDRSTRLEN);   // reconverte para string apenas para fins de log e console
            printf("[RULE %d] act=%d verbose=%d ip=%s mask=%d\n", count, r.act, r.verbose, ip_str, mask);

        } else if (token3 != NULL){             // caso em que tem o verbose

            if(strcmp(token2, "verbose") == 0){ 
                r.verbose = true;

                // se tiver um quarto token, vai ser a máscara de sub-rede
                if (token4 != NULL) mask = atoi(token4);

                // o ip vai ser o terceiro token nesse caso
                if(inet_pton(AF_INET, token3, &r.ip) != 1){
                    printf("[X] Erro na conversao do ip {%s}", token3);
                    continue;
                }
            inet_ntop(AF_INET, &r.ip, ip_str, INET_ADDRSTRLEN);
            printf("[RULE %d] act=%d verbose=%d ip=%s mask=%d\n", count, r.act, r.verbose, ip_str, mask);

            } else {  // caso não tenha verbose
                mask = atoi(token3);        //  o token 3 vai ser a máscara de sub-rede
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

        r.mask = mask;         // atribui a máscara final na struct Rule
        list[count] = r;        // adiciona a regra obtida na lista de regras
        count++;

        if(count == capacidade){                  // atualizacao do buffer da lista de acordo com a quantidade de regras( se atinge a capacidade máxima dele, duplicamos seu tamanho)
            capacidade *= 2;

            Rule *temp = realloc(list, capacidade * sizeof(Rule));
            if(temp == NULL){
                printf("[X] Erro de memoria ao expandir regras (realloc)\n");
                free(list);
                for(int i = 0; i < fw_count; i++){
                    free(fw_list[i]);
                }

                free(fw_list);
                fclose(f);

                *total_rules = 0;
                *forbidden_words = NULL;
                *forbidden_count = 0;

                return NULL;
            }
            list = temp;
        }
    }
    fclose(f);
    // variaveis que vao ser retornadas para a main
    *total_rules = count;
    *forbidden_words = fw_list;
    *forbidden_count = fw_count;
    return list;    // retorna a lista com todas as regras obtidas
}