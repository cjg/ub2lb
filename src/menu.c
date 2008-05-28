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

static inline char *get_next_word(context_t * ctx, char *buffer,
				  int buffer_length, int *buffer_offset)
{
	char *word;
	int offset;

	offset = *buffer_offset;

	while (offset < buffer_length && !ISBLANK(buffer[offset]))
		offset++;

	word = alloc_mem(ctx, offset - *buffer_offset + 1);
	strncpy(word, buffer + *buffer_offset, offset - *buffer_offset);
	word[offset - *buffer_offset] = 0;

	*buffer_offset = offset;

	return word;
}

static inline char *get_next_line(context_t * ctx, char *buffer,
				  int buffer_length, int *buffer_offset)
{
	char *word;
	int offset;

	offset = *buffer_offset;

	while (offset < buffer_length && buffer[offset] != '\n')
		offset++;

	word = alloc_mem(ctx, offset - *buffer_offset + 1);
	strncpy(word, buffer + *buffer_offset, offset - *buffer_offset);
	word[offset - *buffer_offset] = 0;

	*buffer_offset = offset + 1;

	return word;
}

static menu_t *entry_alloc(context_t * ctx)
{
	menu_t *entry;
	entry = alloc_mem(ctx, sizeof(menu_t));
	entry->title = NULL;
	entry->kernel = NULL;
	entry->append = NULL;
	entry->initrd = NULL;
	entry->next = NULL;
	return entry;
}

static void entry_free(context_t * ctx, menu_t * self)
{
	menu_t *entry;
	entry = alloc_mem(ctx, sizeof(menu_t));
	if (entry->title != NULL)
		free_mem(ctx, entry->title);
	if (entry->kernel != NULL)
		free_mem(ctx, entry->kernel);
	if (entry->append != NULL)
		free_mem(ctx, entry->append);
	if (entry->initrd != NULL)
		free_mem(ctx, entry->initrd);
	free_mem(ctx, entry);
}

menu_t *menu_load(context_t * ctx, boot_dev_t * boot)
{
	menu_t *self, *entry;
	char *buffer;
	char *word;
	int buffer_length;
	int buffer_offset = 0;
	int current_state;

	buffer = alloc_mem(ctx, 0x1000);

	if (buffer == NULL)
		return NULL;

	buffer_length = boot->load_file(boot, MENU_FILE, buffer);

	if (buffer_length <= 0) {
		free_mem(ctx, buffer);
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
		ctx->c_printf("current_state %d\n", current_state);
/* 		ctx->c_printf("buffer: %s\n",  buffer + buffer_offset); */
/* 		break; */
		switch (current_state) {
		case STATE_S:
			entry = entry_alloc(ctx);
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C1;
				break;
			}
			word = get_next_word(ctx, buffer, buffer_length,
					     &buffer_offset);
/* 			ctx->c_printf("|%s|\n",  word); */
			if (!strcasecmp(word, "title"))
				current_state = STATE_T1;
			else
				current_state = -1;
			free_mem(ctx, word);
			break;
		case STATE_T1:
			word = get_next_line(ctx, buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) != 0) {
				entry->title = word;
				current_state = STATE_T2;
			} else {
				current_state = -1;
				free_mem(ctx, word);
			}
			break;
		case STATE_T2:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C2;
				break;
			}
			word = get_next_word(ctx, buffer, buffer_length,
					     &buffer_offset);
			if (!strcasecmp(word, "kernel"))
				current_state = STATE_K1;
			else
				current_state = -1;
			free_mem(ctx, word);
			break;
		case STATE_K1:
			word = get_next_word(ctx, buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) != 0) {
				entry->kernel =
				    alloc_mem(ctx, strlen(word) + 1);
				strcpy(entry->kernel, word);
				current_state = STATE_K2;
			} else
				current_state = -1;
			free_mem(ctx, word);
			break;
		case STATE_K2:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C3;
				break;
			}
			word = get_next_line(ctx, buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) > 0) {
				entry->append =
				    alloc_mem(ctx, strlen(word) + 1);
				strcpy(entry->append, word);

			}
			free_mem(ctx, word);
			current_state = STATE_I1;
			break;
		case STATE_I1:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C3;
				break;
			}
			word = get_next_word(ctx, buffer, buffer_length,
					     &buffer_offset);
			if (!strcasecmp(word, "initrd"))
				current_state = STATE_I2;
			else if (!strcasecmp(word, "title"))
				current_state = STATE_A2;
			else if (!strlen(word))
				current_state = STATE_I1;
			else
				current_state = -1;
			free_mem(ctx, word);
			break;
		case STATE_I2:
			if (buffer[buffer_offset] == '#') {
				buffer_offset++;
				current_state = STATE_C4;
				break;
			}
			word = get_next_line(ctx, buffer, buffer_length,
					     &buffer_offset);
			if (strlen(word) > 0) {
				entry->initrd =
				    alloc_mem(ctx, strlen(word) + 1);
				strcpy(entry->initrd, word);
			}
			free_mem(ctx, word);
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
			ctx->c_printf("Accepted\n");
			while (buffer[buffer_offset++] != '\n') ;
			entry->next = self;
			self = entry;
			current_state = STATE_S;
			break;
		case STATE_A2:
			ctx->c_printf("Accepted\n");
			entry->next = self;
			self = entry;
			entry = entry_alloc(ctx);
			current_state = STATE_T1;
			break;
		default:
			while (buffer_offset < buffer_length
			       && buffer[buffer_offset++] != '\n') ;
			entry_free(ctx, entry);
			current_state = STATE_S;
			break;
		}
	}

	free_mem(ctx, buffer);
	return self;
}

void display(context_t * ctx, menu_t * self, int selected)
{
	int i = 0;
	menu_t *entry;
	char buff[100];

	for (entry = self; entry != NULL; entry = entry->next, i++) {
		ctx->c_sprintf(buff, "%d. %s", i, entry->title);
		ctx->c_video_draw_text(0, 5 + i, (i == selected ? 2 : 0), buff,
				       80);
	}
}

int menu_display(context_t * ctx, menu_t * self)
{
	int key, max, selected = 0;
	menu_t *entry;
	for(max = -1, entry = self; entry != NULL; entry = entry->next, max++);
	
	while (1) {
		display(ctx, self, selected);
		key = ctx->c_video_get_key();
		if (key == 4 && selected > 0)
			selected--;
		if (key == 3 && selected < max)
			selected++;
		if (key == 1 || key == 6)
			return selected;
	}

	return 0;
}

void menu_free(context_t * ctx, menu_t * self)
{
	menu_t *entry;
	while (self != NULL) {
		entry = self;
		self = self->next;
		entry_free(ctx, self);
	}
}
