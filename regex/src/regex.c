#include "regex.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


static int AddState(Regex* r) {
    if (r->num_states + 1 > r->state_cap) {
        r->state_cap = r->state_cap ? r->state_cap * 2 : 1;
        r->states = (State*)realloc(r->states, sizeof(State) * r->state_cap);
    }
    r->states[r->num_states] = (State){0};
    return r->num_states++;
}

static int AddRule(State* s) {
    if (s->numRules + 1 > s->capRules) {
        s->capRules = s->capRules ? s->capRules * 2 : 1;
        s->rules = (Rule*)realloc(s->rules, sizeof(Rule) * s->capRules);
    }
    s->rules[s->numRules] = (Rule){0};
    return s->numRules++;
}

void FreeRegex(Regex* r) {
    for (int i = 0; i < r->num_states; i++) {
        free(r->states[i].rules);
    }
    free(r->states);
}

typedef enum NodeType {
    CONCAT = 128,
} NodeType;

typedef struct ParseNode {
    NodeType c;
} ParseNode;

typedef struct ParseTree {
    uint32_t size;
    uint32_t top;
    ParseNode* nodes;
} ParseTree;

typedef struct Tokenizer {
    const char* str;
    char* At;
} Tokenizer;

static void PrintTree(ParseTree* t) {
    for (int i = 0; i < t->top; i++) {
        switch (t->nodes[i].c) {
            case CONCAT:
                {
                    printf("Concat ");
                    break;
                }
            default:
                {
                    printf("%c ", t->nodes[i].c);
                }
        }
    }
    printf("\n");
}

static void PushNode(ParseTree* t, NodeType c) {
    if (t->top + 1 > t->size) {
        t->size = t->size ? t->size * 2 : 1;
        t->nodes = (ParseNode*)realloc(t->nodes, sizeof(ParseNode) * t->size);
    }

    t->nodes[t->top++] = (ParseNode) {
        .c = c
    };
}

static ParseNode TopNode(ParseTree* t) {
    return t->nodes[t->top - 1];
}
static void ParseModifier(ParseTree* t, Tokenizer* tok) {
    switch (tok->At[0]) {
        case '*':
            {
                PushNode(t, '*');
                tok->At++;
                return;
            }
        case '+':
            {
                PushNode(t, '+');
                tok->At++;
                return;
            }
        default:
            {
                return;
            }
    }
}

static void ParseString(ParseTree* t, Tokenizer* tok) {
    //treat start different
    if (isalnum(tok->At[0]) || tok->At[0] == '.') {
        PushNode(t, tok->At[0]);
        tok->At++;
        ParseModifier(t, tok);
    } else return;

    while (isalnum(tok->At[0]) || tok->At[0] == '.') {
        PushNode(t, tok->At[0]);
        tok->At++;
        ParseModifier(t, tok);
        PushNode(t, CONCAT);
    }
}


static int ParseExpr(ParseTree* t, Tokenizer* tok) {
    PrintTree(t);
    if (!tok->At[0]) {
        printf("EOF\n");
        return 0;
    }
    switch (tok->At[0]) {
        case '|':
            {
                printf("or\n");
                tok->At++;
                ParseExpr(t, tok);
                PushNode(t, '|'); 
                return 0;
            }
        case '*':
            {
                printf("star\n");
                PushNode(t, '*');
                tok->At++;
                int valid = ParseExpr(t, tok);
                //if (valid) PushNode(t, CONCAT);
                return 1;
            }
        case ')':
            {
                printf("end group\n");
                tok->At++;
                return 0;
            }
        case '(':
            {
                printf("start group\n");
                tok->At++;
                ParseExpr(t, tok);
                ParseModifier(t, tok);
                int i = ParseExpr(t, tok);
                if (i) PushNode(t, CONCAT);
                return 1;
            }
        default:
            {
                printf("string\n");
                ParseString(t, tok);
                int i = ParseExpr(t, tok);
                if (i) PushNode(t, CONCAT);
                return 1;
            }
    }
}

typedef struct SubMachine {
    uint32_t start;
    uint32_t end;
} SubMachine;

typedef struct StateStack {
    uint32_t top;
    uint32_t cap;
    SubMachine* stack;
} StateStack;

static void PushStack(StateStack* s, SubMachine m) {
    printf("Push(%d, %d)\n", m.start, m.end);
    if (s->top + 1 > s->cap) {
        s->cap = s->cap ? s->cap * 2 : 1;
        s->stack = (SubMachine*)realloc(s->stack, s->cap * sizeof(SubMachine));
    }
    s->stack[s->top++] = m;
}

static SubMachine PopStack(StateStack* s) {
    if (s->top) return s->stack[--s->top];
    return (SubMachine){0};
}

static void ConvertToAutomata(Regex* r, ParseTree* t) {
    
    //start state
    uint32_t start = AddState(r);
    StateStack stack = {0};

    for (int i = 0; i < t->top; i++) {
        ParseNode n = t->nodes[i];
        
        switch ((uint32_t)n.c) {
            case '|':
                {
                    SubMachine m2 = PopStack(&stack);
                    SubMachine m1 = PopStack(&stack);

                    SubMachine m = {0};
                    m.start = AddState(r);
                    m.end = AddState(r);

                    uint32_t rule = AddRule(&r->states[m.start]);
                    r->states[m.start].rules[rule] = (Rule) {
                        .end = m1.start,
                        .t = EPSILON,
                    };

                    rule = AddRule(&r->states[m.start]);
                    r->states[m.start].rules[rule] = (Rule) {
                        .end = m2.start,
                        .t = EPSILON,
                    };

                    rule = AddRule(&r->states[m1.end]);
                    r->states[m1.end].rules[rule] = (Rule) {
                        .end = m.end,
                        .t = EPSILON,
                    };

                    rule = AddRule(&r->states[m2.end]);
                    r->states[m2.end].rules[rule] = (Rule) {
                        .end = m.end,
                        .t = EPSILON,
                    };

                    printf("Union: (%d %d), (%d %d)\n", m1.start, m1.end, m2.start, m2.end);
                    PushStack(&stack, m);
                    
                    break;
                }
            case '+':
                {
                    SubMachine m = PopStack(&stack);

                    uint32_t more = AddRule(&r->states[m.end]);
                    r->states[m.end].rules[more] = (Rule) {
                        .end = m.start,
                        .t = EPSILON,
                    };
                    
                    SubMachine new = {
                        .start = m.start,
                        .end = m.end,
                    };
                    printf("Plus\n");
                    PushStack(&stack, new);
                    break;
                }
            case '*':
                {
                    SubMachine m = PopStack(&stack);

                    uint32_t zero = AddRule(&r->states[m.start]);
                    r->states[m.start].rules[zero] = (Rule) {
                        .end = m.end,
                        .t = EPSILON,
                    };

                    uint32_t more = AddRule(&r->states[m.end]);
                    r->states[m.end].rules[more] = (Rule) {
                        .end = m.start,
                        .t = EPSILON,
                    };
                    
                    SubMachine new = {
                        .start = m.start,
                        .end = m.end,
                    };
                    printf("Star\n");
                    PushStack(&stack, new);

                    break;
                }
            case CONCAT:
                {
                    SubMachine m2 = PopStack(&stack);
                    SubMachine m1 = PopStack(&stack);

                    uint32_t rule = AddRule(&r->states[m1.end]);
                    r->states[m1.end].rules[rule] = (Rule) {
                        .t = EPSILON,
                        .end = m2.start,
                    };

                    SubMachine m3 = {
                        .start = m1.start,
                        .end = m2.end,
                    };
                    printf("Push Concat: (%d %d) (%d %d)\n",
                            m1.start, m1.end, m2.start, m2.end);
                    PushStack(&stack, m3);
                    break;
                }
            case '.': 
                {
                    printf("Push Wildcard\n");
                    SubMachine ch = {0};
                    ch.start = AddState(r);
                    ch.end = AddState(r);

                    uint32_t rule = AddRule(&r->states[ch.start]);
                    r->states[ch.start].rules[rule] = (Rule) {
                        .t = MATCH_ALL,
                        .end = ch.end
                    };

                    PushStack(&stack, ch);
                    break;
                }
            default:
                {
                    printf("Push Char %c\n", n.c);
                    SubMachine ch = {0};
                    ch.start = AddState(r);
                    ch.end = AddState(r);

                    uint32_t rule = AddRule(&r->states[ch.start]);
                    r->states[ch.start].rules[rule] = (Rule) {
                        .t = (char)n.c,
                        .end = ch.end
                    };

                    PushStack(&stack, ch);
                }
        }

    }
    printf("Stack: %d\n", stack.top);

    SubMachine m = PopStack(&stack);
    uint32_t startrule = AddRule(&r->states[start]);
    r->states[start].rules[startrule] = (Rule) {
        .end = m.start,
        .t = EPSILON
    };

    uint32_t endrule = AddRule(&r->states[m.end]);
    r->states[m.end].rules[endrule] = (Rule) {
        .end = -1,
        .t = EPSILON
    };

    printf("final(%d, %d)\n", m.start, m.end);
    free(stack.stack);

}


Regex CreateRegex(char* const str) {
    //states 0 and 1 are special
    Regex r = {0};
    uint32_t head = 0;
    uint32_t str_p = 0;

    Tokenizer tok = {
        .str = str,
        .At = str
    };
    
    ParseTree tree = {0};

    //parse into tree
    ParseExpr(&tree, &tok);
    PrintTree(&tree);

    ConvertToAutomata(&r, &tree);

    free(tree.nodes);

    return r;
}


int Match(Regex r, const char* str) {

    //for now just one state
    int8_t curr[r.num_states];
    int8_t next[r.num_states];

    uint8_t matched = 0;
    uint32_t str_p = 0;

    memset(curr, 0, r.num_states * sizeof(int8_t));
    memset(next, 0, r.num_states * sizeof(int8_t));

    //start
    curr[0] = 1;

    while (1) {
        //reset
        matched = 0;

        uint8_t set = 0;
        uint32_t found = 0;

        do {
            found = 0;
            for (int i = 0; i < r.num_states; i++) {        
                if (!curr[i]) continue;

                State s = r.states[i];

                //epsilon transitions first
                for (int j = 0; j < s.numRules; j++) {
                    Rule r1 = s.rules[j];
                    if (r1.t == EPSILON) {
                        int32_t e = r1.end;
                        if (e == -1) {
                            if (!matched) printf("epsilon(%d, %d)\n", i, e);
                            matched = 1;
                        } else if (!curr[e]) {
                            found = 1;
                            curr[e] = 1;
                            printf("epsilon(%d, %d)\n", i, e);
                        }
                    }            
                }

            }
        } while(found);

        //memcpy(curr, next, r.num_states * sizeof(int8_t));  
        memset(next, 0, r.num_states * sizeof(uint8_t));

        if (str[str_p]) {
            char c = str[str_p++];
            for (int i = 0; i < r.num_states; i++) {        
                if (!curr[i]) continue;

                State s = r.states[i];

                //non epsilon transitions
                for (int j = 0; j < s.numRules; j++) {
                    Rule r1 = s.rules[j];
                    if (r1.t == EPSILON) {
                        continue;
                    } else if (r1.t == MATCH_ALL) {
                        int32_t e = r1.end;
                        printf("wildcard(%d, %d)\n", i, e);
                        if (e == -1) matched = 1; 
                        else next[e] = 1;
                        set = 1;
                    } else {
                        int32_t e = r1.end;
                        printf("match(%d, %d)\n", i, e);
                        int passed = (c == r1.t);
                        if (!passed) continue;
                        if (e == -1) matched = 1; 
                        else next[e] = (c == r1.t);
                        set = 1;
                    }
                }
            }
        } else if (matched) {
            return 1;
        }

        //update states
        memcpy(curr, next, r.num_states * sizeof(int8_t));  

        //no valid states
        if (!set) {
            //if (matched) return 1;
            printf("\tOut of States\n");
            return 0;
        }
    }

    return matched;
}
