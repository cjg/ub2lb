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
	entry->modules_cnt = 0;
	entry->argc = 0;
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
	/* FIXME: free modules and args */
	free(entry);
}

enum states {S, DEF, DLY, TIT, IN, RT, K, RD, M, A, R, AQ, RQ};

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

char *argsplit(char *line, char **argv, int *argc)
{
	
}


static menu_t *fsm(char *buffer, int buffer_len)
{
	int offset;
	char *line;
	char *word;
	char *lineptr;
	int status;
	menu_t *menu, *entry, *ptr;
	
	menu = entry_alloc();
	menu->default_os = 0;
	menu->delay = 5;
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
			} else if (strcmp(word, "default") == 0) {
				status = DEF;
				lineptr = line + strlen(word);
			} else if (strcmp(word, "delay") == 0) {
				status = DLY;
				lineptr = line + strlen(word);
			} else
				status = S;
			free(word);
			break;
		case DEF:
			status = S;
			if(strlen(lineptr) == 0) 
				break;
			strtrim(lineptr);
			word = getnextword(lineptr);
			if(strlen(word) == 0) 
				break;
			menu->default_os = strtol(word);
			free(word);
			break;
		case DLY:
			status = S;
			if(strlen(lineptr) == 0) 
				break;
			strtrim(lineptr);
			word = getnextword(lineptr);
			if(strlen(word) == 0) 
				break;
			menu->delay = strtol(word);
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
			} else if(strcmp(word, "module") == 0) {
				status = M;
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
			if(strcmp(word, "tftp") == 0) {
				entry->device_type = TFTP_TYPE;
				free(word);
				free(line);
				status = IN;
			} else if(strcmp(word, "cdrom") == 0) {
				entry->device_type = CD_TYPE;
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
				dev[sep-lineptr - 1] = 0;
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
		case M:
			strtrim(lineptr);
			word = getnextword(lineptr);
			if(!strlen(word))
				status = R;
			else {
				status = IN;
				entry->modules[entry->modules_cnt++] = 
					strdup(word);
			}
			free(line);
			free(word);
			break;
		case A:
			for(ptr = menu; ptr->next != NULL; ptr=ptr->next);
			ptr->next = entry;
			status = S;
			break;
		case R:
			entry_free(entry);
			status = S;
			break;
		case AQ:
			for(ptr = menu; ptr->next != NULL; ptr=ptr->next);
			ptr->next = entry;
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

menu_t *display(menu_t * self, int selected)
{
	int i;
	menu_t *entry, *sentry;
	char buff[100];
	static int first = 0, last = 10;

	video_draw_box(1, 0, "Select boot option.", 1, 5, 5, 70, 15); 

	if(selected < first)
		first--;

	if(selected > last)
		first++;

	for(i = 0; i < first; i++)
		self = self->next;

	for (i = 0, entry = self->next; entry != NULL && i < 11; 
	     entry = entry->next, i++) {
		sprintf(buff, " %s", entry->title);
		video_draw_text(6, 8 + i, (i + first == selected ? 2 : 0), buff,
				68);
		if(i + first == selected)
			sentry = entry;
	}

	last = first + i - 1;

	return sentry;
}

menu_t *menu_display(menu_t * self)
{
	int key, max, selected, delay, old_delay;
	menu_t *entry;
	for(max = -2, entry = self; entry != NULL; entry = entry->next, max++);

	selected = self->default_os <= max ? self->default_os : max;
	delay = self->delay * 100;
	
	entry = display(self, selected);
	while (delay) {
		if(old_delay != delay / 100) {
			char tmp[30];
			sprintf(tmp, "(%d seconds left)", delay / 100);
			video_draw_text(54, 6, 0, tmp, 20);
			old_delay = delay / 100;
		}
		if(tstc())
			break;
		udelay(10000);
		delay--;
	}

	if(delay == 0)
		return entry;
 
	while(1) {
		entry = display(self, selected);
		key = video_get_key();
		if (key == 4 && selected > 0)
			selected--;
		if (key == 3 && selected < max)
			selected++;
		if (key == 117) 
			break;
		if (key == 1 || key == 6) 
			return entry;
	}

	return NULL;
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
