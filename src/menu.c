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
#include "iniparser.h"

/*
typedef struct menu {
	char *title;
	char *kernel;
	char *append;
	char *initrd;
	struct menu *next;
} menu_t;
*/

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
enum states {S, TIT, IN, RT, K, RD, A, R, AQ, RQ};

char *getline(char *src, int src_len, int *src_offset)
{
	char *nlptr, *line;
	int len;
	
	if (*src_offset >= src_len)
		return NULL;

	nlptr = strchr(src + *src_offset, '\n');
	if(nlptr != NULL)
		len = nlptr - (src + *src_offset) + 1;
	else
		len = src_len - *src_offset;

	line = malloc(len + 1);
	strncpy(line, src + *src_offset, len);
	*src_offset += len;
	line[len] = 0;

	return line;
}

char *eatcomments(char *line)
{
	char *cptr;

	cptr = strchr(line, '#');
	
	if(cptr != NULL)
		line[cptr - line] = 0;

	return line;
}

char *getnextword(char *line)
{
	char *bptr, *word;
	int len;

	bptr = strchr(line, ' ');
	
	if(bptr == NULL)
		bptr = strchr(line, '\t');
		
	if(bptr != NULL)
		len = bptr - line;
	else
		len = strlen(line);

	word = malloc(len + 1);
	strncpy(word, line, len);
	word[len] = 0;

	return word;
}

char *strtrim(char *s)
{
	char *ptr_s, *tmp;
	int n;

	ptr_s = s;
	for (; isspace(*ptr_s); ptr_s++) ;

	for (n = strlen(ptr_s); n > 0 && isspace(*(ptr_s + n - 1)); n--) ;
	if (n < 0)
		n = 0;

	tmp = strdup(ptr_s);
	strcpy(s, tmp);
	*(s + n) = 0;
	free(tmp);

	return s;
}


static menu_t *fsm(char *buffer, int buffer_len)
{
	int offset;
	char *line;
	char *word;
	char *lineptr;
	int status;
	menu_t *menu, *entry;
	
	menu = NULL;
	line = NULL;
	offset = 0;
	status = S;
	int parse = 1, kernel;
	while(parse) {
		switch(status) {
		case S:
			if((line = getline(buffer, buffer_len, 
					   &offset)) == NULL) {
				parse = 0;
				break;
			}
			eatcomments(line);
			strtrim(line);
			if(strlen(line) == 0) {
				free(line);
				status = S;
				break;
			}
			word = getnextword(line);
			if(strcmp(word, "title") == 0) {
				status = TIT;
				lineptr = line + strlen(word);
			} else
				status = S;
			free(word);
			break;
		case TIT:
			strtrim(lineptr);
			if(strlen(lineptr) > 0) {
				status = IN;
				kernel = 0;
				entry = entry_alloc();
				entry->title = strdup(lineptr);
			} else
				status = S;
			free(line);
			break;
		case IN:
			if((line = getline(buffer, buffer_len, 
					   &offset)) == NULL) {
				if(kernel)
					status = AQ;
				else
					status = RQ;
				break;
			}
			eatcomments(line);
			strtrim(line);
			if(strlen(line) == 0) {
				free(line);
				status = IN;
				break;
			}
			word = getnextword(line);
			lineptr = line + strlen(word);
			if(strcmp(word, "kernel") == 0) {
				status = K;
			} else if(strcmp(word, "root") == 0) {
				status = RT;
			} else if(strcmp(word, "initrd") == 0) {
				status = RD;
			} else if(strcmp(word, "title") == 0) {
				offset -= strlen(line) + 1;
				free(line);
				if(kernel) {
					status = A;
				} else {
					status = R;
				}
			} else {
				free(line);
				status = R;
			}
			free(word);
			break;
		case K:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if(!strlen(word))
				status = R;
			else {
				status = IN;
				kernel = 1;
				entry->kernel = strdup(word);
				lineptr = lineptr + strlen(word);
				strtrim(lineptr);
				if(strlen(lineptr) > 0) {
					entry->append = strdup(lineptr);
				}
			}
			free(line);
			free(word);
			break;
		case RT:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if(!strlen(word)) {
				status = R;
				free(word);
				free(line);
				break;
			}
			lineptr = lineptr + strlen(word);
			strtrim(lineptr);
			/* accept tftp or ide */
			if(strcmp(word, "tftp") == 0) {
				if(strlen(lineptr) == 0) {
					status = R;
					free(word);
					free(line);
					break;
				}
				entry->device_type = TFTP_TYPE;
				free(word);
				free(line);
				status = IN;
			} else if(strcmp(word, "ide") == 0) {
				char *sep = strchr(lineptr, ':');
				if(sep == NULL || *++sep == 0) {
					status = R;
					free(line);
					free(word);
					break;
				}
				char *dev = malloc(sep - lineptr);
				strncpy(dev, lineptr, sep - lineptr - 1);
				entry->device_type = IDE_TYPE;
				entry->device_num = strtol(dev);
				entry->partition = strtol(sep);
				free(word);
				free(line);
				free(dev);
				status = IN;
			} else {
				status = R;
				free(word);
				free(line);
			}
			break;
		case RD:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if(!strlen(word))
				status = R;
			else {
				status = IN;
				entry->initrd = strdup(word);
			}
			free(line);
			free(word);
			break;
		case A:
			entry->next = menu;
			menu = entry;
			status = S;
			break;
		case R:
			entry_free(entry);
			status = S;
			break;
		case AQ:
			entry->next = menu;
			menu = entry;
			return menu;
			break;
		case RQ:
		default:
			entry_free(entry);
			return NULL;
			break;
		}
	}
	return menu;
}

menu_t *menu_load(boot_dev_t * boot)
{
	menu_t *self;
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

	self = fsm(buffer, buffer_length);

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
