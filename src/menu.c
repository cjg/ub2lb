/* menu.c */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define DEBUG 1
#include "debug.h"
#include "menu.h"

/*
typedef struct menu {
	char *title;
	char *kernel;
	char *append;
	char *initrd;
	struct menu *next;
} menu_t;
*/

#define ISBLANK(C) ((C) == ' ' || (C) == '\n' || (C) == '\r' || (C) == '\t')
#define ISSPACE(C) ((C) == ' ' || (C) == '\t')

static inline char *get_next_word(char *buffer, int buffer_length, 
				  int *buffer_offset)
{
	char *word;
	int offset;

	offset = *buffer_offset;

	while (offset < buffer_length && !ISBLANK(buffer[offset]))
		offset++;

	word = malloc(offset - *buffer_offset + 1);
	strncpy(word, buffer + *buffer_offset, offset - *buffer_offset);
	word[offset - *buffer_offset] = 0;

	*buffer_offset = offset;

	return word;
}

static inline char *get_next_line(char *buffer, int buffer_length, 
				  int *buffer_offset)
{
	char *word;
	int offset;

	offset = *buffer_offset;

	while (offset < buffer_length && buffer[offset] != '\n')
		offset++;

	word = malloc(offset - *buffer_offset + 1);
	strncpy(word, buffer + *buffer_offset, offset - *buffer_offset);
	word[offset - *buffer_offset] = 0;

	*buffer_offset = offset + 1;

	return word;
}

static menu_t *entry_alloc(void)
{
	menu_t *entry;
	entry = malloc(sizeof(menu_t));
	entry->title = NULL;
	entry->kernel = NULL;
	entry->append = NULL;
	entry->initrd = NULL;
	entry->device_type = DEFAULT_TYPE;
	entry->device_num = 0;
	entry->partition = 0;
	entry->server_ip = 0;
	entry->next = NULL;
	return entry;
}

static void entry_free(menu_t * entry)
{
	if (entry->title != NULL)
		free(entry->title);
	if (entry->kernel != NULL)
		free(entry->kernel);
	if (entry->append != NULL)
		free(entry->append);
	if (entry->initrd != NULL)
		free(entry->initrd);
	free(entry);
}

menu_t *menu_load(boot_dev_t * boot)
{
	menu_t *self, *entry;
	char *buffer;
	char *word;
	int buffer_length;
	int buffer_offset = 0;
	int current_state;

	buffer = malloc(0x1000);

	if (buffer == NULL)
		return NULL;

	buffer_length = boot->load_file(boot, MENU_FILE, buffer);

	if (buffer_length <= 0) {
		free(buffer);
		return NULL;
	}

/* 	char *buffer = "title linux\nkernel vmlinux\ntitle  "; */
/* 	buffer_length = strlen(buffer); */
	/* parse! */

#define STATE_S  0
#define STATE_T1 1
#define STATE_T2 2
#define STATE_K1 3
#define STATE_K2 4
#define STATE_K3 5
#define STATE_I1 6
#define STATE_I2 7
#define STATE_A1  8
#define STATE_A2  9
#define STATE_C1 10
#define STATE_C2 11
#define STATE_C3 12
#define STATE_C4 13
#define STATE_C5 14

	current_state = STATE_S;
	buffer_offset = 0;
	self = NULL;
	while (buffer_offset < buffer_length) {
		while (buffer_offset < buffer_length
		       && ISSPACE(buffer[buffer_offset]))
			buffer_offset++;
		switch (current_state) {
		case STATE_S:
			entry = entry_alloc();
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C1;
				break;
			}
			word = get_next_word(buffer, buffer_length,
					     &buffer_offset);
			if (!strcasecmp(word, "title"))
				current_state = STATE_T1;
			else
				current_state = -1;
			free(word);
			break;
		case STATE_T1:
			word = get_next_line(buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) != 0) {
				entry->title = word;
				current_state = STATE_T2;
			} else {
				current_state = -1;
				free(word);
			}
			break;
		case STATE_T2:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C2;
				break;
			}
			word = get_next_word(buffer, buffer_length,
					     &buffer_offset);
			if (!strcasecmp(word, "kernel"))
				current_state = STATE_K1;
			else
				current_state = -1;
			free(word);
			break;
		case STATE_K1:
			word = get_next_word(buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) != 0) {
				entry->kernel =
				    malloc(strlen(word) + 1);
				strcpy(entry->kernel, word);
				current_state = STATE_K2;
			} else
				current_state = -1;
			free(word);
			break;
		case STATE_K2:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C3;
				break;
			}
			word = get_next_line(buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) > 0) {
				entry->append =
				    malloc(strlen(word) + 1);
				strcpy(entry->append, word);

			}
			free(word);
			current_state = STATE_I1;
			break;
		case STATE_I1:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C3;
				break;
			}
			word = get_next_word(buffer, buffer_length,
					     &buffer_offset);
			if (!strcasecmp(word, "initrd"))
				current_state = STATE_I2;
			else if (!strcasecmp(word, "title"))
				current_state = STATE_A2;
			else if (!strlen(word))
				current_state = STATE_I1;
			else
				current_state = -1;
			free(word);
			break;
		case STATE_I2:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C4;
				break;
			}
			word = get_next_line(buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) > 0) {
				entry->initrd =
				    malloc(strlen(word) + 1);
				strcpy(entry->initrd, word);
			}
			free(word);
			current_state = STATE_A1;
			break;
		case STATE_C1:
			while (buffer[buffer_offset++] != '\n') ;
			current_state = STATE_S;
			break;
		case STATE_C2:
			while (buffer[buffer_offset++] != '\n') ;
			current_state = STATE_T2;
			break;
		case STATE_C3:
			while (buffer[buffer_offset++] != '\n') ;
			current_state = STATE_I1;
			break;
		case STATE_C4:
			while (buffer[buffer_offset++] != '\n') ;
			current_state = -1;
			break;
		case STATE_A1:
			while (buffer[buffer_offset++] != '\n') ;
			entry->next = self;
			self = entry;
			current_state = STATE_S;
			break;
		case STATE_A2:
			entry->next = self;
			self = entry;
			entry = entry_alloc();
			current_state = STATE_T1;
			break;
		default:
			while (buffer_offset < buffer_length
			       && buffer[buffer_offset++] != '\n') ;
			entry_free(entry);
			current_state = STATE_S;
			break;
		}
	}

	free(buffer);
	return self;
}

void display(menu_t * self, int selected)
{
	int i = 0;
	menu_t *entry;
	char buff[100];

	for (entry = self; entry != NULL; entry = entry->next, i++) {
		sprintf(buff, "%d. %s", i, entry->title);
		video_draw_text(0, 5 + i, (i == selected ? 2 : 0), buff,
				80);
	}
}

int menu_display(menu_t * self)
{
	int key, max, selected = 0;
	menu_t *entry;
	for(max = -1, entry = self; entry != NULL; entry = entry->next, max++);
	
	while (1) {
		display(self, selected);
		key = video_get_key();
		if (key == 4 && selected > 0)
			selected--;
		if (key == 3 && selected < max)
			selected++;
		if (key == 1 || key == 6)
			return selected;
	}

	return 0;
}

void menu_free(menu_t * self)
{
	menu_t *entry;
	while (self != NULL) {
		entry = self;
		self = self->next;
		entry_free(self);
	}
}
