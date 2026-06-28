#pragma once

#include <stdint.h>
#include <stddef.h>

void terminal_init(void);
void terminal_task(void);
void terminal_process_line(char *line);
void terminal_print_prompt(void);
