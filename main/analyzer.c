#include <stdio.h>
#include <ctype.h>
#include <mem.h>
#include <setjmp.h>
#include <stdlib.h>

//Типы лексем
#define DELIMITER  1 //Разделитель
#define VARIABLE   2 //Переменная
#define NUMBER     3 //Число
#define COMMAND    4 //Команда
#define QUOTE      5 //Кавычки

//Внутренние представления лексем
#define PRINT 10
#define INPUT 11
#define IF 12
#define THEN 13
#define ELSE 14
#define GOTO 15
#define GOSUB 16
#define RETURN 17
#define EOL 18 //Конец строки файла
#define END 19
#define FINISHED 20 //Конец программы
#define FI 21

//-------------------------------------------------------------

//Объявление переменных
#define LAB_LEN 2  //длинна имени метки
#define NUM_LAB 100 //длинна массива меток
#define SUB_NEST 25 //длинна стека для GOSUB

char *program;
char token[80]; //Строковое представление лексемы
int token_int; //Внутреннее представление лексемы
int token_type; //Тип лексемы
jmp_buf e_buf;

char *gStack[SUB_NEST]; //Стек подпрограмм GOSUB
int gIndex; //Индекс верхней части стека

struct command {
    char name[10];
    int token_int;
} tableCommand[] = {
        "PRINT", PRINT,
        "INPUT", INPUT,
        "IF", IF,
        "THEN", THEN,
        "ELSE", ELSE,
        "FI", FI,
        "GOTO", GOTO,
        "GOSUB", GOSUB,
        "RETURN", RETURN,
        "END", END};

struct label {
    char name[LAB_LEN]; //Имя метки
    char *p; //Указатель на место размещения в программе
};
struct label labels[NUM_LAB];

int numberOfValues = 0;
struct variable {
    char name[80]; //имя переменной
    int value; //зачение переменной
} *p_variable;

//Объявление функций
int getToken(); //Достает очередную лексему

void putBack(); //Возвращает лексему во входной поток
void findEol(); //Переходит на следующую строку
int isDelimetr(char); //Проверяет является ли символ разделителем
void sError(int); //Сделать ошибки более информативными
int getCommandNumber(char *); //Возвращает внутреннее представление команды

void assignment(); //Присваивает значение переменной
void mainCount(int *), helpCount_1(int *), helpCount_2(int *), helpCount_3(int *); //Уровни анализа арифметической операции
void value(int *); //Определение значения переменной
void unary(char, int *); //Изменение знака
void arithmetic(char, int *, int *); //Примитивные операции
void getExpression(int *); //Начало анализа арифметического выражения
struct variable *findV(char[]); //Поиск переменной по имени

int getNextLabel(char *); //Возвращает следующую метку
void scanLabels(); //Находит все метки
void labelInit(); //Заполняет массив с метками нулями
char *findLabel(char *); //Возвращает метку

void ifStatement();
void notElse();
void gotoStatement();
void gosubStatement();
void returnStatement();
void gosubPush(char *);
char *gosubPop();
void printStatement();
void inputStatement();

void start(char *p) {
    program = p;

    if (setjmp(e_buf)) exit(1); //Инициализация буфера нелокальных переходов

    scanLabels();

    do {
        token_type = (char) getToken();

        //Проверка на присваивание
        if (token_type == VARIABLE) { //если это перемнная
            putBack();   //откатиться на 1 лексему
            assignment(); //сделать присваивание
        }

        //Проверка на команду
        if (token_type == COMMAND) {
            switch (token_int) {
                case PRINT:
                    printStatement();
                    break;
                case INPUT:
                    inputStatement();
                    break;
                case IF:
                    ifStatement();
                    break;
                case ELSE:
                    notElse();
                    break;
                case GOTO:
                    gotoStatement();
                    break;
                case GOSUB:
                    gosubStatement();
                    break;
                case RETURN:
                    returnStatement();
                    break;
                case END:
                    notElse();
                default:
                    break;
            }
        }
    } while (token_int != FINISHED);
}

int getToken() {
    char *temp; //Указатель на лексему

    token_type = 0;
    token_int = 0;
    temp = token;

    //Проверка закончился ли файл интерпретируемой программы
    if (*program == '\0') {
        *token = '\0';
        token_int = FINISHED;
        return (token_type = DELIMITER);
    }

    //Проверка на конец строки программы
    if (*program == '\n') {
        program++;
        token_int = EOL;
        *token = '\n';
        temp++;
        *temp = 0;
        return (token_type = DELIMITER);
    }

    while (isspace(*program))
        program++; //Пропускаем пробелы; Перенести в начало, реализовав свою функцию, чтоб \n не удалялся TODO

    //Проверка на разделитель
    if (strchr("+-*/%=:,()><", *program)) {
        *temp = *program;
        program++;
        temp++;
        *temp = 0;
        return (token_type = DELIMITER);
    }

    //Проверяем на кавычки
    if (*program == '"') {
        program++;
        while (*program != '"' && *program != '\n') *temp++ = *program++;
        if (*program == '\n') sError(0);
        program++;
        *temp = 0;
        return (token_type = QUOTE);
    }

    //Проверка на число
    if (isdigit(*program)) {
        while (!isDelimetr(*program)) {
            *temp++ = *program++;
        }
        *temp = 0;
        return (token_type = NUMBER);
    }

    //Переменная или команда?
    if (isalpha(*program)) {
        while (!isDelimetr(*program))
            *temp++ = *program++;
        *temp = 0;
        token_int = getCommandNumber(token); //Получение внутреннего представления команды
        if (!token_int) {
            token_type = VARIABLE;
        } else
            token_type = COMMAND;
        return token_type;
    }
    sError(0);
}

int isDelimetr(char c) {
    if (strchr(" !;,+-<>\'/*%=()\"", c) || c == 9 || c == '\r' || c == 0 || c == '\n')
        return 1;
    return 0;
}

//Чем я думал? Можно же просто аргументом строку подавать...
//Обязательно когда-нибудь перепишу
void sError(int error) {
    static char *e[] = {
            "error",
    };
    printf("%s\n", e[error]);
    longjmp(e_buf, 1); //Возврат в точку сохранения
}

int getCommandNumber(char *t) {

    //Поиск лексемы в таблице операторов
    for (int i = 0; *tableCommand[i].name; i++) {
        if (!strcmp(tableCommand[i].name, t))
            return tableCommand[i].token_int;
    }

    return 0; //Незнакомый оператор
}

void putBack() {
    char *t;
    t = token;
    for (; *t; t++) program--;
}

//Сложение или вычитание
void mainCount(int *result) {
    char op;
    int hold;

    helpCount_1(result);
    while ((op = *token) == '+' || op == '-') {
        getToken();
        helpCount_1(&hold);
        arithmetic(op, result, &hold);
    }
}

//Вычисление произведения или частного
void helpCount_1(int *result) {
    char operation;
    int hold;

    helpCount_2(result);

    while ((operation = *token) == '/' || operation == '%' || operation == '*') {
        getToken();
        helpCount_2(&hold);
        arithmetic(operation, result, &hold);
    }
}

//Унарный + или -
void helpCount_2(int *result) {
    char operation;
    operation = 0;
    if ((token_type == DELIMITER) && *token == '+' || *token == '-') {
        operation = *token;
        getToken();
    }
    helpCount_3(result);
    if (operation)
        unary(operation, result);
}

//Обработка выражения в круглых скобках
void helpCount_3(int *result) {
    if ((*token == '(') && (token_type == DELIMITER)) {
        getToken();
        mainCount(result);
        if (*token != ')')
            sError(0);
        getToken();
    } else
        value(result);
}

//Определение значения переменной по ее имени
void value(int *result) {
    switch (token_type) {
        case VARIABLE:
            *result = findV(token)->value;
            getToken();
            return;
        case NUMBER:
            *result = atoi(token);
            getToken();
            return;
        default:
            sError(0);
    }
}

//Изменение знака
void unary(char o, int *r) {
    if (o == '-') *r = -(*r);
}

//Выполнение специфицированной арифметики
void arithmetic(char o, int *r, int *h) {
    int t;

    switch (o) {
        case '-':
            *r = *r - *h;
            break;
        case '+':
            *r = *r + *h;
            break;
        case '*':
            *r = *r * *h;
            break;
        case '/':
            *r = (*r) / (*h);
            break;
        case '%':
            t = (*r) / (*h);
            *r = *r - (t * (*h));
            break;
        default:
            break;
    }
}

void getExpression(int *result) {
    getToken();
//    if (!*token) {
//        sError(0);
//        return;
//    }
    mainCount(result);
    putBack();
}

struct variable *findV(char n[]) {
    int i = 1;
    struct variable *t = p_variable;
    while (i <= numberOfValues) {
        if (!strcmp(n, t->name)) {
            return t;
        }
        i++;
        t++;
    }
    return NULL;
}

void setValue(struct variable *var, int v) {
    var->value = v;
}

struct variable *addV(char n[]) {
    struct variable *t = NULL;
    if (numberOfValues != 0)
        t = p_variable;

    numberOfValues++;
    p_variable = (struct variable *) realloc(p_variable, sizeof(struct variable) * numberOfValues);
    if (t != NULL) {
        p_variable = t;
    }
    int i = 1;
    while (i < numberOfValues) {
        p_variable++;
        i++;
    }
    struct variable *r = p_variable;
    strcpy(p_variable->name, n);
    i = 1;
    while (i < numberOfValues) {
        p_variable--;
        i++;
    }
    return r;
}

//Присваивание значения переменной
void assignment() {

    int value;
    getToken(); //Получаем имя переменной

    struct variable *var;
    if ((var = findV(token)) != NULL) {
        getToken(); //Считываем равно
        getExpression(&value);         //вычисляет значение переменной
        setValue(var, value);
    } else {
        var = addV(token);
        getToken(); // Считываем равно
        getExpression(&value);
        setValue(var, value);
    }
}

//Переход на следующую строку программы
void findEol() {
    while (*program != '\n' && *program != '\0')
        program++;
    if (*program)
        program++;
}

void printStatement() {
    int answer;
    char lastDelim = 0;

    do {
        getToken(); //Получаем следующий элемент
        if (token_int == EOL || token_int == FINISHED) break;

        if (token_type == QUOTE) {
            printf(token);
            getToken();
        } else { //Значит выражение
            putBack(); //Возвращает лексему обратно во входной поток
            getExpression(&answer);
            getToken();
            printf("%d", answer);
        }
        lastDelim = *token;

        if (*token != ',' && token_int != EOL && token_int != FINISHED)
            sError(0);
    } while (*token == ',');

    if (token_int == EOL || token_int == FINISHED) {
        if (lastDelim != ';' && lastDelim != ',') printf("\n");
        else sError(0);
    } else sError(0); //Отсутствует ',' или ';'
}

void ifStatement() {
    int x, y, cond;
    char operation;
    char operationSecond;
    getExpression(&x); //Получаем левое выражение
    getToken(); //Получаем оператор
    if (!strchr("=<>", *token)) {
        sError(0);      //Недопустимый оператор
        return;
    }
    operation = *token;
    //Определяем результат
    cond = 0;
    switch (operation) {
        case '=':
            getExpression(&y);
            if (x == y) cond = 1;
            break;
        case '<':
           getToken();
           if (strchr("=<>",*token)) {
               operationSecond = *token;
               switch (operationSecond){
                   case '=':
                       getExpression(&y);
                       if (x <= y) cond = 1;
                       break;
                   case '>' :
                       getExpression(&y);
                       if (x != y) cond = 1;
                       break;
                   case '<':
                       sError(0);
                       return;
               }
           } else {
               putBack();
               getExpression(&y);
               if (x < y) cond = 1;
               break;
           }
           break;
        case '>':
            getToken();
            if (strchr("=<>",*token)) {
                operationSecond = *token;
                switch (operationSecond){
                    case '=':
                        getExpression(&y);
                        if (x >= y) cond = 1;
                        break;
                    case '>' :
                        sError(0);
                        return;
                    case '<':
                        sError(0);
                        return;
                }
            } else {
                putBack();
                getExpression(&y);
                if (x > y) cond = 1;
                break;
            }
            break;
        default:
            break;
    }
    if (cond) {  //Если значение IF "истина"
        getToken();
        if (token_int != THEN) {
            sError(0);
            return;
        }
    } else {
        getToken(); //Пропускаем THEN
        getToken();
        if (strchr("\n", *token)) {
            do {
                getToken();
                if (token_int == END) {
                    sError(0); //Прописать ошибку (Ожидался ELSE) TODO
                }
            } while (token_int != ELSE);
        } else findEol(); //Если ложь - переходим на следующую строку
    }
}

void notElse() {
    do {
        getToken();
        if (token_int == END) {
            getToken();
            if (token_int != FI)
                sError(0); //Прописать ошибку (Ожидался END FI) TODO
        }
    } while (token_int != FI);
}

void gotoStatement() {
    char *location;

    getToken(); //Получаем метку перехода
    location = findLabel(token);  //Сделать ошибку TODO
    program = location; //Старт программы с указанной точки
}

//Инициализация массива хранения меток
void labelInit() {
    for (int i = 0; i < NUM_LAB; i++)
        labels[i].name[0] = '\0';
}

//Поиск всех меток
void scanLabels() {
    int location;
    char *temp;

    labelInit();  //Инициализация массива меток
    temp = program;   //Указатель на начало программы

    getToken();
    //Если лексема является меткой
    if (token_type == NUMBER) {
        strcpy(labels[0].name, token);
        labels[0].p = program;
    }

    findEol();
    do {
        getToken();
        if (token_type == NUMBER) {
            location = getNextLabel(token);
            if (location == -1 || location == -2) {
                if (location == -1)
                    sError(0);
                else
                    sError(0);
            }
            strcpy(labels[location].name, token);
            labels[location].p = program; //Текущий указатель программы
        }
        //Если строка не помечена, переход к следующей
        if (token_int != EOL) findEol();
    } while (token_int != FINISHED);
    program = temp; //Восстанавливаем начальное значение
}

char *findLabel(char *s) {
    for (int i = 0; i < NUM_LAB; i++)
        if (!strcmp(labels[i].name, s)) {
            return labels[i].p;
        }
    return '\0'; //Ошибка
}


int getNextLabel(char *s) {

    for (int i = 0; i < NUM_LAB; i++) {
        if (labels[i].name[0] == 0) return i;
        if (!strcmp(labels[i].name, s)) return -2;
    }
    return -1;
}

void gosubStatement() {
    char *location;
    getToken();

    //Поиск метки вызова
    location = findLabel(token); //Сделать ошибку TODO
    gosubPush(program); //Запомним место, куда вернемся
    program = location; //Старт программы с указанной точки
}

//Возврат из подпрограммы
void returnStatement() {
    program = gosubPop();
}

//Помещает данные в стек GOSUB
void gosubPush(char *s) {
    gIndex++;
    if (gIndex == SUB_NEST) {
        sError(0);
        return;
    }
    gStack[gIndex] = s;
}

//Достает данные из стека GOSUB
char *gosubPop() {
    if (gIndex == 0) {
        sError(0);
        return '\0';
    }
    return (gStack[gIndex--]);
}

void inputStatement() {
    int i;
    struct variable *var;

    getToken(); //Анализ наличия символьной строки
    if (token_type == QUOTE) {
        printf(token); //Если строка есть, проверка запятой
        getToken();
        if (*token != ',') sError(0);
        getToken();
    } else printf("Write data: ");
    if ((var = findV(token)) == NULL)
        var = addV(token);
    scanf("%d", &i);   //Чтение входных данных
    setValue(var, i);  //Сохранение данных
}



