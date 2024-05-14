#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
 
// 07.10.2023 [2wn]
// В данной версии содержиться базовый функционал, и при корректных данных вполне 
// корректно отрабатывает (особо не проверялось, но вроде и для белеберды разумно
// делает, согласно используемым обозначениям)
// Основой скипа скобок служит OVERDRIVE - по сути одна из базовых функций
// Вторую такую можно кое-как назвать wbi
// 
// В данной реализации считаются индексы левее равно (для подсчитывания только безымянных 
// индексов надо подставить ALF=1) и циклично подставляются в безымянные индексы правее равно. 

#define SECOND(a1, a2, ...) a2
#define THIRD(a1, a2, a3, ...) a3

#define CYCLE_SCROLLER  "i_"
#define EXTENSION       "txt"           /* расширение файла который перерабытавается в .c */

#define MaxLineFor  10      /* максимальное количество for на одной линии                   */
#define MaxLineLenght 256   /* максимальное число символов на одной строчки в файле         */
#define MaxColonCount 10    /* сколько : в одних скобках (например a[i:0:7:-1] - тут их 3)  */
#define MaxNameLenght 64    /* максимальная длинна названия массива, его индекса (i_k), ... */

#define ALF 1               /* учитывать ли имееные индексы левее равно */

// текущая строка
int line_in_file = 1;
char sensible[MaxLineLenght];

// для каждого for-а хранится информация о:  [ ... : ... : ...]
int bracket_pos[MaxLineFor][MaxColonCount+2];
int for_count;

// названия массивов и соответствующих им индексам для которых расширяется for
char masiv_name[MaxLineFor][MaxNameLenght];
bool need_index[MaxLineFor];
char index_name[MaxLineFor][MaxNameLenght];
int index_count[MaxLineFor];

int empty_index[MaxLineFor];
int empty_count;


/* C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.                */
char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

bool check_file(char* filename, FILE** code, FILE** modernize){
    char* after_dot = filename;
    
    for(char* pointer = filename; *pointer; pointer++) {
        if(*pointer == '.') after_dot = pointer+1;
    }
    
    if(EXTENSION[0] && strcmp(after_dot, EXTENSION)) {printf("Incorrect file name extension\n"); return 0;}
    
    if(!(*code = fopen(filename, "r"))) {printf("File didn't exist\n"); return 0;}
    
    *after_dot = 'c';
    *(after_dot+1) = '\0';
    if(!(*modernize = fopen(filename, "w"))) {fclose(*code); printf("Can't create file\n"); return 0;}
    return 1;
}

#define QUOTE "''\"\""
// controller[3] = {quantity, open_bracket, close_bracket}
int in_quote_synchronize(char a, const char* bracket, int* controller, int otr) {
    if(*controller) {
        int prev = *controller, add = (a == controller[2] ? -1 : a == controller[1]);
        return (controller[0] += add) || prev;
    }

    for(; *bracket; bracket += 2)
        if(a == bracket[otr]) {
            controller[1] = bracket[ otr];
            controller[2] = bracket[!otr];
            return (*controller) = 1;
        }

    return *controller;
}

#define rawOVERDRIVE(bracket, i, otr, serch, ...) fix_text_alternative(sensible, i, otr, serch, bracket)
#define spaceOVERDRIVE(i, otr, ...) OVERDRIVE(i, otr, no_void_space, SECOND(,##__VA_ARGS__,  ""))
#define OVERDRIVE(i, ...) rawOVERDRIVE(THIRD(__VA_ARGS__, QUOTE "[]()", ""), i, __VA_ARGS__, no_void_space)

int fix_text_alternative(char* s, int l, bool otr, bool (*serch_symbol)(char), char* bracket) {
    if(otr ? l < 0 : !s[l]) return -1;

    int quote[3] = {0}, add = otr ? -1 : +1;
    for(; in_quote_synchronize(s[l], bracket, quote, otr) || !serch_symbol(s[l]); l += add) 
        if(otr ? l == 0 : !s[l]) return -1;
        
    return l;
}

bool masiv_framing(char a) {return a == '[' || a == ':';}
bool no_void_space(char a) {return a != ' ' && a != '\t';}
bool is_number(char a) {return '0' <= a && a <= '9';}
bool no_masiv_name(char c) {return (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && (c < '0' || c > '9') && c != '_';}

int is_noname(int start, int end) {
    for(; start <= end; start++)
        if(no_void_space(sensible[start]) && !is_number(sensible[start])) return 0;      
    return 1;
}

// write bracket incorporated
// Переписывает sensible[start]..[end] в dst
// Если попадаются расширяемые for то расписыват их
void wbi(char* dst, int start, int end) {
    for(; start <= end; start++) {
        if(!no_void_space(sensible[start])) continue;
        
        *dst++ = sensible[start];
        if(sensible[start] == '[') { //возможно расширяемый for
            int k = 0;
            while(k < for_count && bracket_pos[k][0] != start) k++;

            // эти скобки - это какой-то рядовой a[i]
            if(k == for_count) continue;

            // эти скобки - это какой-то расширяемый for
            int colapse_bracket = !masiv_name[k][0]; // попался масив без именем
            dst -= colapse_bracket; // "стираем" поставленую "[" 

            for(int j = 0; index_name[k][j]; j++)
                *dst++ = index_name[k][j];

            start = bracket_pos[k][index_count[k]] - !colapse_bracket; // чтоб в следующей итерации распечаталось "]"
        }
    }
        
    *dst = 0;
}

int record_for_info(int* cycled_index) {
    // пока считаем что всякие найденные скобки дают корректные данные
    { // записываем имя
        //  находим имя
        int masiv_start = 0, masiv_end = OVERDRIVE(bracket_pos[for_count][0]-1, 1); //пропускаем пробелы
        int temp = spaceOVERDRIVE(masiv_end, 1, "[]");  //пропускаем []
        if(temp != -1) {
            masiv_start = OVERDRIVE(temp, 1, no_masiv_name, "") + 1;
            while(is_number(sensible[masiv_start])) masiv_start++;
        }

        wbi(masiv_name[for_count], masiv_start, masiv_end);
    }

    { // записываем название итерирующей переменной
        need_index[for_count] = 1;
        if(!ALF && !cycled_index[1]) empty_index[empty_count++] = for_count;

        // кринж по-меньше (видели бы что было..)
        if(is_noname(bracket_pos[for_count][0]+1, bracket_pos[for_count][1]-1)) { // безымяная переменная
            if(cycled_index[1]) {
                cycled_index[0] = (cycled_index[0] + 1) % empty_count;
                need_index[for_count] = 0;
            } else if(ALF) empty_index[empty_count++] = for_count;

            char reletative_name[100] = CYCLE_SCROLLER;
            itoa(for_count, reletative_name + strlen(reletative_name), 10);
            strcpy(index_name[for_count], cycled_index[1] ? index_name[empty_index[*cycled_index]] : reletative_name);
        } else // с именем, возможно сложным
            wbi(index_name[for_count], bracket_pos[for_count][0]+1, bracket_pos[for_count][1]-1);
    }

    for_count++;
    return 1;
} 

int prognoze(char* s, int n, int* cycled_index) {
    int pos = n-1;
    int colon_data[MaxColonCount], quantity = 0;
    while(pos = OVERDRIVE(pos, 1, masiv_framing)) {
        if(pos == -1) {printf("Somethink incorrect with brackets..\n"); return -1;}
        if(s[pos] == '[') break;
        colon_data[quantity++] = pos--;
    }

    if(!quantity && OVERDRIVE(pos+1, 0) != n) return 0;
    // Это коректный расширяемый for, начинаем записывать информацию о нём

    {// записываем информацию о скобках (по идее это перенести в record_for_info, но это ведь кринж каждый раз массив передавать туда)
        bracket_pos[for_count][0] = pos;
        for(int i = 1; i <= quantity; i++)
            bracket_pos[for_count][i] = colon_data[quantity-i];
        bracket_pos[for_count][quantity+1] = n;
        index_count[for_count] = quantity+1;
    }
    return record_for_info(cycled_index);
}

int analyze_line(FILE* code, int* new_line){
    *new_line = line_in_file+1;
    if(fscanf(code, "%c", sensible) == EOF) return 0;
    if(*sensible == '\n') {*sensible = 0; return 1;}

    int i = 1, quote[3] = {0}, cycled_index[2] = {-1, 0};
    for_count = 0, empty_count = 0;
    for(char a; fscanf(code, "%c", &a) != EOF && a != '\n'; i++) {
        if(!in_quote_synchronize(a, QUOTE, quote, 0) && sensible[i-1] == '/' && strchr("/*", a)) { // начался коментарий
            int enter_count = 0;
            char skip[3] = {0};
            while(fscanf(code, "%c", skip+1) != EOF) {
                enter_count += skip[1] == '\n';
                if((a == '*' && !strcmp(skip, "*/")) || (a == '/' && skip[1] == '\n')) break;
                skip[0] = skip[1];
            }

            i -= 1 + !enter_count;
            if(enter_count) {*new_line += enter_count-1; break;}
            continue;
        }

        sensible[i] = a;
        if(!*quote && a == ']') prognoze(sensible, i, cycled_index);

        if(!*quote && a == '=') cycled_index[1] = 1;
    }

    sensible[i] = 0;
    return 1 + for_count;
}

bool open_braket(char a) {return a == '[';}
bool close_braket(char a) {return a == ']';}
// пользовательская функция, "выдающая" размер масива по его имени
// например:  name.size() или size(name) или т.д.
void write_size(char* dst, char* name) {
    int dive_depth = 0, before_first_bracket = strlen(name), end_bracket = 0;
    while(end_bracket != -1) {
        int start_bracket = fix_text_alternative(name, end_bracket, 0, open_braket, "");
        if(start_bracket != -1) {
            end_bracket = fix_text_alternative(name, start_bracket+1, 0, close_braket, QUOTE"()[]");
        } else break;

        if(!dive_depth) {
            if(start_bracket > 0) before_first_bracket = start_bracket;
            else{
                name++;
                before_first_bracket = end_bracket-1;
            }
        }
        dive_depth++;
    }
    sprintf(dst, "k_size(%.*s, %d)", before_first_bracket, name, dive_depth);
}

bool not_only_space(int start, int end){
    for(; start <= end; start++)
        if(no_void_space(sensible[start])) return 1;
    return 0;
}

int write_for(FILE* file, int k) {
    bool exist_index = !is_noname(bracket_pos[k][0]+1, bracket_pos[k][1]-1);
    int beg_loc, end_loc; // начало/конец расположения "объекта" в sensible

    char cringe_start[MaxNameLenght] = "0";
    beg_loc = bracket_pos[k][exist_index  ]+1;
    end_loc = bracket_pos[k][exist_index+1]-1;
    if(exist_index + 1 < index_count[k] &&  not_only_space(beg_loc, end_loc))
        wbi(cringe_start, beg_loc, end_loc);

    char cringe_end[MaxNameLenght];
    strcpy(cringe_end, cringe_start);   
    strcpy(cringe_end + strlen(cringe_end), "+1");
    beg_loc = bracket_pos[k][index_count[k]-1]+1;
    end_loc = bracket_pos[k][index_count[k]  ]-1;
    if(not_only_space(beg_loc, end_loc)) 
        wbi(cringe_end, beg_loc, end_loc);
    else if(masiv_name[k][0])
        write_size(cringe_end, masiv_name[k]);

    fprintf(file, "for(int %s = %s; %s < %s; %s++)", index_name[k], cringe_start,
            index_name[k], cringe_end, index_name[k]);

    // кол-во напечатанных символов:
    return 21 + 3*strlen(index_name[k]) + strlen(cringe_start) + strlen(cringe_end);
}

void print_without_last_enter(FILE* modernize) {
    int l = strlen(sensible);
    bool t = sensible[l-1] == '\r';
    if(t)    sensible[l-1] = 0;
    fprintf(modernize, "// input:  >%s<\n", sensible);
    if(t)    sensible[l-1] = '\r';
}

int main(int argc, char* argv[]) {
    if(argc != 2) {printf("Incorrect arguments\n"); return 0;}
    FILE* code;
    FILE* modernize;
    if(!check_file(argv[1], &code, &modernize)) return 0;

    printf("Strat reading ...\n");
    int detailed, new_line;
    for(; detailed = analyze_line(code, &new_line); line_in_file = new_line) {
        printf("[%2d] line:  %*s>%s< (detailed %d   for_count %d)\n", line_in_file, 32, "", sensible, detailed, for_count);
        if(detailed == 1 || detailed == -1) {
            printf("\n"); 
            fprintf(modernize, "%s\n", sensible);
            continue;
        }
        
        print_without_last_enter(modernize);
        for(int k = 0; k < for_count; k++) {
            if(need_index[k]) {
                int wfl = write_for(modernize, k);
                fprintf(modernize, "%*s // line: [%2d]\n", 70-wfl, "", line_in_file);
            }

            printf(" %2d:%*s>%s<  ~ %*s>%s<  |", k, 15 - strlen(masiv_name[k]), " ", masiv_name[k],
                                                    15 - strlen(index_name[k]), " ", index_name[k]);
         
            for(int j = 0, i = 0; !j || sensible[bracket_pos[k][j-1]] != ']'; j++) {
                for(; i < bracket_pos[k][j]; i++) printf(" ");
                printf("%c", sensible[i++]);
            }
            printf("\n");
        }
        printf("\n");

        char cringe_buffer[MaxLineLenght];
        wbi(cringe_buffer, 0, strlen(sensible)-1);
        fprintf(modernize, "\t%s\n", cringe_buffer);
    }
}
