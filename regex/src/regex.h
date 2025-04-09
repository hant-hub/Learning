#ifndef REGEX_H
#define REGEX_H

#include <stdint.h>


//most characters can be themselves
typedef enum TransitionType {
    EPSILON = 128,
    MATCH_ALL
} TransitionType;

typedef struct Rule {
    int32_t end;
    TransitionType t;
} Rule;

typedef struct State {
    uint32_t numRules;
    uint32_t capRules;
    Rule* rules;
} State;

// states 0, and -1 are the start and end state
// respectively
typedef struct Regex {
    uint32_t num_states;
    uint32_t state_cap;
    State* states;
} Regex;


Regex CreateRegex(char* const str);
void FreeRegex(Regex* r);
int Match(Regex r, const char* str);





#endif
