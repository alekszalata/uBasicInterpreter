#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <mem.h>
#include <ctype.h>

#include "analyzer.h"

#define size 2000

char *program;
char *p_buf; //Указатель начала буфера программы

int load(char*, char*); //Считывает программу

int main(int argc, char *argv[]) {
    char *file_name = argv[1]; //Имя файла программы

    if (argc != 2) {
        printf("Use format: <executable file>.exe <program file>.txt");
        exit(1);
    }

    //Выделение памяти для программы
    if (!(p_buf = (char *) malloc(size))) { //вместо 500 можно поставить другое значение
        printf("memory error");
        exit(1);
    }

    //Загрузка программы
    if (!load(p_buf, file_name)) exit(1);

    program = p_buf;
    start(program);
    return 0;
}

int load(char *p, char *fname) {
    FILE *file;

    if (!(file = fopen(fname, "r"))) //Открываем только на чтение
        return 0;

    //Считываем текст программы в память
    int i = 0;
    do {
        *p = (char) getc(file);
        p++;
        i++;
        if (i == size)
            p_buf = (char*) realloc(p_buf, (size_t) (i + size));
    } while (!feof(file));
    *(p - 1) = '\0';
    fclose(file);
    return 1;
}

