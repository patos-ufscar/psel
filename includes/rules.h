#ifndef RULES_H
#define RULES_H

#include <stdint.h>
#include <stdbool.h>


typedef enum {
    ALLOW,
    DENY,
    TARPIT
} TypeAct;


typedef struct
{
    bool verbose;
    TypeAct act;
    int mask;
    uint32_t ip;
} Rule;

Rule* load_rules(const char *filename, int *total_rules);

#endif