#include "../include/string.h"
#include "../include/stdio.h"

#include "../Keyboard.h"

char* Ady_nano_editor(char* previous_data) {
    VGA_clrscr();
    printf("You are editing file. Use CAPSLOCK to exit.\r\n\r\n");

    char* text = keyboard_edit(previous_data);
    return text;    
}