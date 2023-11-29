//
// Created by carson on 11/15/21.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "assembler.h"

char *ASM_ERROR_UNKNOWN_INSTRUCTION = "Unknown Assembly Instruction";
char *ASM_ERROR_ARG_REQUIRED = "Argument Required";
char *ASM_ERROR_BAD_LABEL = "Bad Label";
char *ASM_ERROR_OUT_OF_RANGE = "Number is out of range";

//=========================================================
//  All the instructions available on the LMSM architecture
//=========================================================
const char *INSTRUCTIONS[28] =
        {"ADD", "SUB", "LDA", "STA", "BRA", "BRZ", "BRP", "INP", "OUT", "HLT", "COB", "DAT",
         "LDI",
         "JAL", "CALL", "RET",
         "SPUSH", "SPUSHI", "SPOP", "SDUP", "SDROP", "SSWAP", "SADD", "SSUB", "SMAX", "SMIN", "SMUL", "SDIV"
        };
const int INSTRUCTION_COUNT = 28;

//===================================================================
//  All the instructions that require an arg on the LMSM architecture
//===================================================================
const char *ARG_INSTRUCTIONS[11] =
        {"ADD", "SUB", "LDA", "STA", "BRA", "BRZ", "BRP", "DAT",
         "LDI",
         "CALL",
         "SPUSHI"
        };
const int ARG_INSTRUCTION_COUNT = 11;

//======================================================
// Constructors/Destructors
//======================================================

asm_instruction * asm_make_instruction(char* type, char *label, char *label_reference, int value, asm_instruction * predecessor) {
    asm_instruction *new_instruction = calloc(1, sizeof(asm_instruction));
    new_instruction->instruction = type;
    new_instruction->label = label;
    new_instruction->label_reference = label_reference;
    new_instruction->value = value;
    new_instruction->next = NULL;
    if (predecessor != NULL) {
        predecessor->next = new_instruction;
        new_instruction->offset = predecessor->offset + predecessor->slots;
    } else {
        new_instruction->offset = 0;
    }
    if(strcmp(type,"CALL") == 0){
        new_instruction->slots = 3;
    }else if(strcmp(type, "SPUSHI") == 0){
        new_instruction->slots = 2;
    }else{
        new_instruction->slots = 1;
    }
    return new_instruction;
}

void asm_delete_instruction(asm_instruction *instruction) {
    if (instruction == NULL) {
        return;
    }
    asm_delete_instruction(instruction->next);
    free(instruction);
}

asm_compilation_result * asm_make_compilation_result() {
    asm_compilation_result *result = calloc(1, sizeof(asm_compilation_result));
    return result;
}

void asm_delete_compilation_result(asm_compilation_result *result) {
    asm_delete_instruction(result->root);
    free(result);
}

//======================================================
// Helpers
//======================================================
int asm_is_instruction(char * token) {
    for (int i = 0; i < INSTRUCTION_COUNT; ++i) {
        if (strcmp(token, INSTRUCTIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int asm_instruction_requires_arg(char * token) {
    for (int i = 0; i < ARG_INSTRUCTION_COUNT; ++i) {
        if (strcmp(token, ARG_INSTRUCTIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int asm_is_num(char * token){
    if (*token == '-') { // allow a leading negative
        token++;
        if (*token == '\0') {
            return 0;
        }
    }
    while (*token != '\0') {
        if (*token < '0' || '9' < *token) {
            return 0;
        }
        token++;
    }
    return 1;
}

int asm_find_label(asm_instruction *root, char *label) {
    int out = -1;
    asm_instruction *inst = root;
    while (inst != NULL){
        if (inst->label!=NULL && (strcmp(inst->label, label) == 0)){
            out = inst->offset;
            break;
        }else{
            inst = inst->next;
        }
    }
    return out;
}


//======================================================
// Assembly Parsing/Scanning
//======================================================

void asm_parse_src(asm_compilation_result * result, char * original_src) {

    // copy over so strtok can mutate
    char *src = calloc(strlen(original_src) + 1, sizeof(char));
    strcat(src, original_src);
    asm_instruction *last_instruction = NULL;
    asm_instruction *current_instruction = NULL;

    char *tok = strtok(src, " \n");

    while (tok != NULL) {

        char *type = NULL;
        char *label = NULL;
        char *label_ref = NULL;
        int value = 0;

        if(!asm_is_instruction(tok)){//either label or inst
            label = tok;
            tok = strtok(NULL, " \n");
        }
        if(asm_is_instruction(tok)){
            type = tok;
            tok = strtok(NULL, " \n");
        }else{
            result->error = ASM_ERROR_UNKNOWN_INSTRUCTION;
            return;
        }
        if(asm_instruction_requires_arg(type)){
            if(tok == NULL){
                result->error = ASM_ERROR_ARG_REQUIRED;
                return;
            }

            if(asm_is_num(tok)){
                for(int i=0; i<strlen(tok); i++){
                    int digit = tok[i]-'0';
                    value = digit + value*10;
                }
                if(value>999 || value<-999){
                    result->error = ASM_ERROR_OUT_OF_RANGE;
                    return;
                }
            }else{
                label_ref = tok;
            }
            tok = strtok(NULL, " \n");
        }

        /*
        char *token = strtok(line, " ");
        while (token != NULL){
            char *type = token;
            token = strtok(NULL, " ");
            int value = 0;
            if(token != Null(token[0] < 10 || token[0] == 45)){//"value"'s ascii value is less than the ascii value of 'a', therefore "value" is a number
                for(int i=0; i<sizeof(token); i++){
                    int digit = token[i]-'0';
                    value = digit + value*10;
                }
                token = strtok(NULL, " ");
            }
        }
        */

        asm_instruction *inst = asm_make_instruction(type, label, label_ref, value, last_instruction);
        last_instruction = inst;

        if(result->root == NULL){
            result->root = inst;
        }

    }

}


//======================================================
// Machine Code Generation
//======================================================

void asm_gen_code_for_instruction(asm_compilation_result  * result, asm_instruction *instruction) {
    if (strcmp("ADD", instruction->instruction) == 0) {
        result->code[instruction->offset] = 100 + instruction->value;
    } else if (strcmp("SUB", instruction->instruction) == 0) {
        result->code[instruction->offset] = 200 +instruction->value;
    }else if (strcmp("STA", instruction->instruction) == 0) {
        int loc = instruction->value;
        if(instruction->label_reference!=NULL){
            loc = asm_find_label(result->root, instruction->label_reference);
            if(loc ==-1){
                result->error = ASM_ERROR_BAD_LABEL;
                return;
            }
        }
        result->code[instruction->offset] = 300 + loc;
    }else if (strcmp("LDA", instruction->instruction) == 0) {
        int loc = instruction->value;
        if(instruction->label_reference!=NULL){
            loc = asm_find_label(result->root,instruction->label_reference);
            if(loc ==-1){
                result->error = ASM_ERROR_BAD_LABEL;
                return;
            }
        }
        result->code[instruction->offset] = 500 + loc;
    }else if (strcmp("BRA", instruction->instruction) == 0) {
        int loc = instruction->value;
        if(instruction->label_reference!=NULL){
            loc = asm_find_label(result->root, instruction->label_reference);
            if(loc ==-1){
                result->error = ASM_ERROR_BAD_LABEL;
                return;
            }
        }
        result->code[instruction->offset] = 600 + loc;
    }else if (strcmp("BRZ", instruction->instruction) == 0) {
        int loc = instruction->value;
        if(instruction->label_reference!=NULL){
            loc = asm_find_label(result->root, instruction->label_reference);
            if(loc ==-1){
                result->error = ASM_ERROR_BAD_LABEL;
                return;
            }
        }
        result->code[instruction->offset] = 700 + loc;
    }else if (strcmp("BRP", instruction->instruction) == 0) {
        int loc = instruction->value;
        if(instruction->label_reference!=NULL){
            loc = asm_find_label(result->root, instruction->label_reference);
            if(loc ==-1){
                result->error = ASM_ERROR_BAD_LABEL;
                return;
            }
        }
        result->code[instruction->offset] = 800 + loc;
    }else if(strcmp("INP",instruction->instruction)==0){
        result->code[instruction->offset] = 901;
    }else if(strcmp("OUT",instruction->instruction)==0){
        result->code[instruction->offset] = 902;
    }else if(strcmp("HALT",instruction->instruction)==0 || strcmp("COB",instruction->instruction)==0){
        result->code[instruction->offset] = 0;
    }else if(strcmp("DAT",instruction->instruction)==0){
        result->code[instruction->offset] = instruction->value;
    }else if (strcmp("LDI", instruction->instruction) == 0) {
        result->code[instruction->offset] = 400 +instruction->value;
    }else if(strcmp("SPUSH",instruction->instruction)==0){
        result->code[instruction->offset] = 920;
    }else if(strcmp("SPOP",instruction->instruction)==0){
        result->code[instruction->offset] = 921;
    }else if(strcmp("SDUP",instruction->instruction)==0){
        result->code[instruction->offset] = 922;
    }else if(strcmp("SDROP",instruction->instruction)==0){
        result->code[instruction->offset] = 923;
    }else if(strcmp("SSWAP",instruction->instruction)==0){
        result->code[instruction->offset] = 924;
    }else if(strcmp("SADD",instruction->instruction)==0){
        result->code[instruction->offset] = 930;
    }else if(strcmp("SSUB",instruction->instruction)==0){
        result->code[instruction->offset] = 931;
    }else if(strcmp("SMUL",instruction->instruction)==0){
        result->code[instruction->offset] = 932;
    }else if(strcmp("SDIV",instruction->instruction)==0){
        result->code[instruction->offset] = 933;
    }else if(strcmp("SMAX",instruction->instruction)==0){
        result->code[instruction->offset] = 934;
    }else if(strcmp("SMIN",instruction->instruction)==0){
        result->code[instruction->offset] = 935;
    }else if(strcmp("SPUSHI",instruction->instruction)==0){
        result->code[instruction->offset] = 400+ instruction->value;
        result->code[instruction->offset+1] = 920;
    }else if(strcmp("JAL",instruction->instruction)==0){
        result->code[instruction->offset] = 910;
    }else if(strcmp("RET",instruction->instruction)==0){
        result->code[instruction->offset] = 911;
    }else if(strcmp("CALL",instruction->instruction)==0){
        result->code[instruction->offset] = 400+instruction->value;
        result->code[instruction->offset+1] = 920;
        result->code[instruction->offset+2] = 910;
    }else{
        result->code[instruction->offset] = 0;
    }

}

void asm_gen_code(asm_compilation_result * result) {
    asm_instruction * current = result->root;
    while (current != NULL) {
        asm_gen_code_for_instruction(result, current);
        current = current->next;
    }
}

//======================================================
// Main API
//======================================================

asm_compilation_result * asm_assemble(char *src) {
    asm_compilation_result * result = asm_make_compilation_result();
    asm_parse_src(result, src);
    asm_gen_code(result);
    return result;
}
