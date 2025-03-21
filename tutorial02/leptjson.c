#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>
#include <errno.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v) {
    switch(*c->json){
        case 'n':
            EXPECT(c,'n');
            if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
                return LEPT_PARSE_INVALID_VALUE;
            c->json += 3;
            v->type = LEPT_NULL;
            break;
        case 't':
            EXPECT(c,'t');
            if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
                return LEPT_PARSE_INVALID_VALUE;
            c->json += 3;
            v->type = LEPT_TRUE;
            break;
        case 'f':
            EXPECT(c,'f');
            if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
                return LEPT_PARSE_INVALID_VALUE;
            c->json += 4;
            v->type = LEPT_FALSE;
            break;
        default:return LEPT_PARSE_INVALID_VALUE;
    }
    return LEPT_PARSE_OK;
}

/* static int lept_parse_true(lept_context* c, lept_value* v) {
//     EXPECT(c, 't');
//     if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 3;
//     v->type = LEPT_TRUE;
//     return LEPT_PARSE_OK;
// }

// static int lept_parse_false(lept_context* c, lept_value* v) {
//     EXPECT(c, 'f');
//     if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 4;
//     v->type = LEPT_FALSE;
//     return LEPT_PARSE_OK;
// }

// static int lept_parse_null(lept_context* c, lept_value* v) {
//     EXPECT(c, 'n');
//     if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 3;
//     v->type = LEPT_NULL;
//     return LEPT_PARSE_OK;
// }*/

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* \TODO validate number */
    int str_len=strlen(c->json);
    int stage_flag=0,stage_int_zeros_num=0,stage_int_num=0,stage_frac_num=0,stage_exp_num=0;
    int i=0;
    switch(c->json[0]){
        case '+': return LEPT_PARSE_INVALID_VALUE;
        case '.': return LEPT_PARSE_INVALID_VALUE;
        case '-': i=1;break;
        default: break;
    }
    stage_flag=1;
    for(;i<str_len;i++){
        switch(stage_flag){
            case 1: 
                if(ISDIGIT(c->json[i])){
                    stage_int_num++;
                    if(stage_int_num == 1){
                        if(c->json[i] == '0')   stage_int_zeros_num++;
                    }
                    else{
                        if(stage_int_zeros_num>0)   return LEPT_PARSE_INVALID_VALUE;
                    }
                }
                else{
                    if(stage_int_num>0){
                        if(c->json[i] == '.')   stage_flag=2;
                        else if(c->json[i] == 'E' || c->json[i] == 'e') stage_flag=3;
                        else    return LEPT_PARSE_INVALID_VALUE;
                    }
                    else    return LEPT_PARSE_INVALID_VALUE;   
                }
                break;
            case 2:
                if(ISDIGIT(c->json[i])){
                    stage_frac_num++;
                }
                else{
                    if(stage_frac_num == 0) return LEPT_PARSE_INVALID_VALUE;
                    if(c->json[i] == 'E' || c->json[i] == 'e')  stage_flag=3;
                    else    return LEPT_PARSE_INVALID_VALUE;
                }
                break;
            case 3:
                if(ISDIGIT(c->json[i])){
                    stage_exp_num++;
                }
                else{
                    if(!((stage_exp_num == 0) && (c->json[i] == '+' || c->json[i] == '-'))){
                        return  LEPT_PARSE_INVALID_VALUE;
                    }
                }
                break;
            default: return  LEPT_PARSE_INVALID_VALUE;
                
        }
    }
    switch(stage_flag){
        case 2:
            if(stage_frac_num == 0) return LEPT_PARSE_INVALID_VALUE;
        default:break;
    }
    v->n = strtod(c->json, &end);
    if(v->n != 0 && errno == ERANGE) return  LEPT_PARSE_NUMBER_TOO_BIG;
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v);
        case 'f':  return lept_parse_literal(c, v);
        case 'n':  return lept_parse_literal(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);
        
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
