#ifndef _LINE_DISCIPLINE_H
#define _LINE_DISCIPLINE_H

#define LINE_DISCIPLINE_MODE_RAW 0
#define LINE_DISCIPLINE_MODE_CANONICAL 1
#define LINE_DISCIPLINE_MODE_ECHO_ON 0
#define LINE_DISCIPLINE_MODE_ECHO_OFF 1

#define LD_NULL 0
#define LD_START_OF_HEADING 1
#define LD_START_OF_TEXT 2
#define LD_END_OF_TEXT 3
#define LD_END_OF_TRANSMISSION 4
#define LD_ENQUIRY 5
#define LD_ACKNOWLEDGE 6
#define LD_BELL 7
#define LD_BACKSPACE 8
#define LD_HORIZONTAL_TABULATION 9
#define LD_LINE_FEED 10
#define LD_VERTICAL_TABULATION 11
#define LD_FORM_FEED 12
#define LD_CARRIAGE_RETURN 13
#define LD_SHIFT_OUT 14
#define LD_SHIFT_IN 15
#define LD_DATA_LINK_ESCAPE 16
#define LD_DEVICE_CONTROL_ONE 17
#define LD_DEVICE_CONTROL_TWO 18
#define LD_DEVICE_CONTROL_THREE 19
#define LD_DEVICE_CONTROL_FOUR 20
#define LD_NEGATIVE_ACKNOWLEDGE 21
#define LD_SYNCHRONOUS_IDLE 22
#define LD_END_OF_TRANSMISSION_BLOCK 23
#define LD_CANCEL 24
#define LD_END_OF_MEDIUM 25
#define LD_SUBSTITUTE 26
#define LD_ESCAPE 27
#define LD_FILE_SEPARATOR 28
#define LD_GROUP_SEPARATOR 29
#define LD_RECORD_SEPARATOR 30
#define LD_UNIT_SEPARATOR 31
#define LD_DEL 127
#define LD_NEWLINE 10

#define LD_DEFAULT_TABLE 0
 
#define LD_INSERT_SIZE 4
#define LD_OUTPUT_SIZE 4

struct line_discipline {
    int mode;
    int echo;
    int valid;

    char *buffer;
    int size;
    int head;
    int tail;

    struct line_discipline_action_table_entry *action_table;

    void* parent;
    void (*flush_cb)(void* parent, char* buffer, int size);
    void (*echo_cb)(void* parent, char c);
};

struct line_discipline_action_table_entry {
    char output[LD_OUTPUT_SIZE];
    char inserti[LD_INSERT_SIZE];
    char inserto[LD_OUTPUT_SIZE];

    int (*action)(struct line_discipline*, char);
};

struct line_discipline * line_discipline_create(int mode, int echo, int table, int buffer_size, void* parent, void (*flush_cb)(void* parent, char *buffer, int size), void (*echo_cb)(void* parent, char c));
void line_discipline_destroy(struct line_discipline *ld);
void line_discipline_set_mode(struct line_discipline *ld, int mode);
void line_discipline_set_echo(struct line_discipline *ld, int echo);
void line_discipline_debug();

void line_discipline_read(struct line_discipline *ld, char character);
int line_discipline_translate(struct line_discipline *ld, char original_character, char* translated_characters);
void line_discipline_force_flush(struct line_discipline *ld);

#endif