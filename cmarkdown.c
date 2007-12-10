/* cmarkdown
 * Copyright (C) <2007> Enno boland <g@s01.de>
 *
 * cmarkdown free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * To compile type
 * gcc -DVERSION=\"`date +%F`\" -o cmarkdown cmarkdown.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define BUFFERSIZE 1024
#define ERRMALLOC eprint("malloc failed\n");
#define LENGTH(x) sizeof(x)/sizeof(x[0])

typedef unsigned int (*Parser)(const char *, const char *);
struct Tag {
	char *search;
	int process;
	char *tag;
};

void eprint(const char *format, ...);			/* Prints error and exits */
void hprint(const char *begin, const char *end);	/* escapes HTML and prints it to stdout*/
void process(const char *begin, const char *end);
							/* Processes range between begin and end with parser (NULL = all parsers) */
unsigned int doreplace(const char *begin, const char *end);
							/* Parser for simple replaces */
unsigned int dolineprefix(const char *begin, const char *end);
							/* Parser for line prefix tags */
unsigned int dosurround(const char *begin, const char *end);
							/* Parser for surrounding tags */
unsigned int dounderline(const char *begin, const char *end);
							/* Parser for underline tags */
unsigned int dolink(const char *begin, const char *end);
							/* Parser for links and images */
unsigned int doshortlink(const char *begin, const char *end);
							/* Parser for links and images */
Parser parsers[] = { dounderline, dolineprefix, dosurround, dolink, doshortlink, doreplace };
							/* list of parsers */

FILE *source;
unsigned int bsize = 0, nohtml = 0;
struct Tag lineprefix[] = {
	{ "   ",	0,	"pre" },
	{ "\t",		0,	"pre" },
	{ "> ",		1,	"blockquote" },
};
struct Tag underline[] = {
	{ "=",		1,	"h1" },
	{ "-",		1,	"h2" },
};
struct Tag surround[] = {
	{ "``",		0,	"code" },
	{ "`",		0,	"code" },
	{ "__",		1,	"strong" },
	{ "**",		1,	"strong" },
	{ "*",		1,	"em" },
	{ "_",		1,	"em" },
};
char * replace[][2] = {
	{ "\n---\n", "\n<hr />\n" },
	{ "\n\n", "<br />\n<br />\n" },
};

void
eprint(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
hprint(const char *begin, const char *end) {
	const char *p;

	for(p = begin; p && p != end; p++) {
		if(*p == '&')		fputs("&amp;",stdout);
		else if(*p == '"')	fputs("&quot;",stdout);
		else if(*p == '>')	fputs("&gt;",stdout);
		else if(*p == '<')	fputs("&lt;",stdout);
		else			putchar(*p);
	}
}

unsigned int
dolink(const char *begin, const char *end) {
	int img;
	const char *desc, *link, *p;
	unsigned int desclen, linklen;

	if(*begin != '[' && strncmp(begin,"![",2) != 0)
		return 0;
	img = 1;
	if(*begin == '[')
		img = 0;
	desc = begin + 1 + img;
	if(!(p = strstr(desc, "](")) || p > end)
		return 0;
	desclen = p - desc;
	link = p + 2;
	if(!(p = strstr(link, ")")) || p > end)
		return 0;
	linklen = p - link;

	if(img) {
		fputs("<img src=\"",stdout);
		process(link,link+linklen);
		fputs("\" alt=\"",stdout);
		process(desc,desc+desclen);
		fputs("\" />",stdout);
	}
	else {
		fputs("<a href=\"",stdout);
		process(link,link+linklen);
		fputs("\">",stdout);
		process(desc,desc+desclen);
		fputs("</a>",stdout);
	}
	return p + 1 - begin;
}
unsigned int
doshortlink(const char *begin, const char *end) {
	const char *p, *c;
	int ismail = 0;

	if(*begin != '<')
		return 0;
	for(p = begin+1; p && p != end && !strstr(" \t\n",p); p++) {
		switch(*p) {
		case ':':
			ismail = -1;
			break;
		case '@':
			if(ismail == 0)
				ismail = 1;
			break;
		case '>':
			fputs("<a href=\"",stdout);
			if(ismail == 1) {
				/* mailto: */
				fputs("&#x6D;&#x61;i&#x6C;&#x74;&#x6F;:",stdout);
				for(c = begin+1; *c != '>'; c++) {
					printf("&#%u;",*c);
				}
				fputs("\">",stdout);
				for(c = begin+1; *c != '>'; c++) {
					printf("&#%u;",*c);
				}
			}
			else {
				hprint(begin+1,p-1);
				fputs("\">",stdout);
				hprint(begin+1,p-1);
			}
			fputs("</a>",stdout);
			return p - begin + 1;
		}
	}
	return 0;
}

unsigned int
dosurround(const char *begin, const char *end) {
	unsigned int i,l;
	const char *p,*ps;

	for(i = 0; i < LENGTH(surround); i++) {
		l = strlen(surround[i].search);
		if(end - begin < l || strncmp(begin,surround[i].search,l) != 0)
			continue;
		for(ps = surround[i].search; *ps == '\n'; ps++);
		l = strlen(ps);
		p = begin + strlen(surround[i].search) - 1;
		do {
			p = strstr(p+1, ps);
		} while(p && p[-1] == '\\');
		if(!p || p > end)
			continue;
		printf("<%s>",surround[i].tag);
		if(surround[i].process)
			process(begin + strlen(surround[i].search), p);
		else
			hprint(begin + strlen(surround[i].search), p);
		printf("</%s>",surround[i].tag);
		return p - begin + l;
	}
	return 0;
}
unsigned int
doreplace(const char *begin, const char *end) {
	unsigned int i, l;

	for(i = 0; i < LENGTH(replace); i++) {
		l = strlen(replace[i][0]);
		if(end - begin < l)
			continue;
		if(strncmp(replace[i][0],begin,l) == 0) {
			fputs(replace[i][1], stdout);
			return l;
		}
	}
	return 0;
}

unsigned int
dolineprefix(const char *begin, const char *end) {
	unsigned int i, j, l;
	char *buffer;
	const char *p;

	if(*begin != '\n' && *begin != '\0')
		return 0;
	for(i = 0; i < LENGTH(lineprefix); i++) {
		l = strlen(lineprefix[i].search);
		if(end - begin+1 < l)
			continue;
		if(strncmp(lineprefix[i].search,begin+1,l))
			continue;

		if(!(buffer = malloc(end - begin+1)))
			ERRMALLOC;
		printf("<%s>",lineprefix[i].tag);
		for(p = begin, j = 0; p != end; p++, j++) {
			buffer[j] = *p;
			if(*p == '\n') {
				if(strncmp(lineprefix[i].search,p+1,l) != 0)
					break;
				p += l;
			}
		}
		if(lineprefix[i].process)
			process(buffer,buffer+strlen(buffer));
		else
			hprint(buffer,buffer+strlen(buffer));
		printf("</%s>",lineprefix[i].tag);
		free(buffer);
		return p - begin;
	}
	return 0;
}

unsigned int
dounderline(const char *begin, const char *end) {
	unsigned int i, j, l;
	const char *p;

	if(*begin != '\n' && *begin != '\0')
		return 0;
	for(p = begin+1,l = 0; p[l] != '\n' && p[l] && p+l != end; l++);
	p += l + 1;
	if(l == 0)
		return 0;
	for(i = 0; i < LENGTH(underline); i++) {
		for(j = 0; p[j] != '\n' && p[j] == underline[i].search[0] && p+j != end; j++);
		if(j >= l) {
			putchar('\n');
			printf("<%s>",underline[i].tag);
			if(underline[i].process)
				process(begin+1, begin + l + 1);
			else
				hprint(begin+1, begin + l + 1);
			printf("</%s>",underline[i].tag);
			return j + l + 2;
		}
	}
	return 0;
}

void
process(const char *begin, const char *end) {
	const char *p;
	int affected;
	unsigned int i;
	
	for(p = begin; *p && p != end;) {
		affected = 0;
		for(i = 0; i < LENGTH(parsers) && affected == 0; i++)
			affected = parsers[i](p, end);
		if(affected == 0) {
			if(nohtml)
				hprint(p,p+1);
			else
				putchar(*p);
			p++;
		}
		else
			p += affected;
	}
}

int
main(int argc, char *argv[]) {
	char *p, *buffer;
	int s;

	source = stdin;
	if(argc > 1 && strcmp("-v", argv[1]) == 0)
		eprint("markdown in C "VERSION" (C) Enno Boland\n");
	else if(argc > 1 && strcmp("-h", argv[1]) == 0)
		eprint("Usage %s [-n] [file]\n -n escape html strictly\n",argv[0]);

	if(argc > 1 && strcmp("-n", argv[1]) == 0)
		nohtml = 1;
	if(argc > 1 + nohtml && strcmp("-", argv[1 + nohtml]) != 0 && !(source = fopen(argv[1 + nohtml],"r")))
		eprint("Cannot open file `%s`\n",argv[1 + nohtml]);
	if(!(buffer = malloc(BUFFERSIZE)))
		ERRMALLOC;
	bsize = BUFFERSIZE;
	/* needed to properly process first line */
	strcpy(buffer,"\n");

	p = buffer+strlen(buffer);
	while(s = fread(p, sizeof(char),BUFFERSIZE, source)) {
		p += s;
		*p = '\0';
		if(BUFFERSIZE + strlen(buffer) > bsize) {
			bsize += BUFFERSIZE;
			if(!(buffer = realloc(buffer, bsize)))
				ERRMALLOC;
		}
	}
	
	process(buffer,buffer+strlen(buffer));
	free(buffer);
}
