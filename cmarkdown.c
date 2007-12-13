/* cmarkdown
 * Copyright (C) <2007> Enno boland <g s01 de>
 *
 * See LICENSE for further informations
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define BUFFERSIZE 512
#define LENGTH(x) sizeof(x)/sizeof(x[0])
#define ADDC(b,i) if((i + 1) % BUFFERSIZE == 0) \
	{ b = realloc(b,((i + 1)+ BUFFERSIZE) * sizeof(b)); if(!b) eprint("Malloc failed."); }; b[i]


typedef unsigned int (*Parser)(const char *, const char *);
struct Tag {
	char *search;
	int process;
	char *tag;
};


void eprint(const char *format, ...);			/* Prints error and exits */
void hprint(const char *begin, const char *end);	/* escapes HTML and prints it to stdout*/
unsigned int doamp(const char *begin, const char *end);	/* Parser for & */
unsigned int dohtml(const char *begin, const char *end);
							/* Parser for html */
unsigned int dolineprefix(const char *begin, const char *end);
							/* Parser for line prefix tags */
unsigned int dolink(const char *begin, const char *end);
							/* Parser for links and images */
unsigned int dolist(const char *begin, const char *end);
							/* Parser for lists */
unsigned int doparagraph(const char *begin, const char *end);
							/* Parser for paragraphs */
unsigned int doreplace(const char *begin, const char *end);
							/* Parser for simple replaces */
unsigned int doshortlink(const char *begin, const char *end);
							/* Parser for links and images */
unsigned int dosurround(const char *begin, const char *end);
							/* Parser for surrounding tags */
unsigned int dounderline(const char *begin, const char *end);
							/* Parser for underline tags */
void process(const char *begin, const char *end);	/* Processes range between begin and end. */

Parser parsers[] = { dounderline, dohtml, dolineprefix, dolist, doparagraph,
	dosurround, dolink, doshortlink, doamp, doreplace };	/* list of parsers */
FILE *source;
unsigned int bsize = 0, nohtml = 0;
struct Tag lineprefix[] = {
	{ "   ",	0,	"pre" },
	{ "\t",		0,	"pre" },
	{ "> ",		1,	"blockquote" },
	{ "###### ",	1,	"h6" },
	{ "##### ",	1,	"h5" },
	{ "#### ",	1,	"h4" },
	{ "### ",	1,	"h3" },
	{ "## ",	1,	"h2" },
	{ "# ",		1,	"h1" },
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
	{ "\n- - -\n",	"\n<hr />\n" },
	{ "\n- - - \n",	"\n<hr />\n" },
	{ " #######\n",	"\n" },
	{ " ######\n",	"\n" },
	{ " #####\n",	"\n" },
	{ " ####\n",	"\n" },
	{ " ###\n",	"\n" },
	{ " ##\n",	"\n" },
	{ " #\n",	"\n" },
	{ " >",		"&gt;" },
	{ "< ",		"&lt;" },
};
char * insert[][2] = {
	{ "  \n",	"<br />" },
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
doamp(const char *begin, const char *end) {
	const char *p;

	if(*begin != '&')
		return 0;
	if(!nohtml) {
		for(p = begin + 1; !strchr("; \\\n\t",*p); p++);
		if(*p == ';')
			return 0;
	}
	fputs("&amp;",stdout);
	return 1;
}
unsigned int
dohtml(const char *begin, const char *end) {
	const char *p, *tag, *tagend;
	if(nohtml || *begin != '\n' || !*begin)
		return 0;
	p = begin;
	if(p[1] == '\n')
		p++;
	if(p[1] != '<' || strchr(" /\n\t\\",p[2]))
		return 0;
	tag = p + 2;
	p += 2;
	for(; !strchr(" >",*p);p++);
	tagend = p;
	while((p = strstr(p,"\n</")) && p < end) {
		p+=3;
		if(strncmp(p, tag, tagend-tag) == 0 && p[tagend-tag] == '>') {
			p++;
			fwrite(begin, sizeof(char), p - begin + tagend - tag,stdout);
			puts("\n");
			return p - begin + tagend - tag;
		}
	}
	return 0;
}
unsigned int
dolineprefix(const char *begin, const char *end) {
	unsigned int i, j, l;
	char *buffer;
	const char *p;

	if(*begin != '\n' || !*begin)
		return 0;
	p = begin;
	if(p[1] == '\n')
		p++;
	for(i = 0; i < LENGTH(lineprefix); i++) {
		l = strlen(lineprefix[i].search);
		if(end - p+1 < l)
			continue;
		if(strncmp(lineprefix[i].search,p+1,l))
			continue;
		if(!(buffer = malloc(BUFFERSIZE)))
			eprint("Malloc failed.");
		buffer[0] = '\0';
		printf("\n<%s>",lineprefix[i].tag);
		for(j = 0; p != end; p++, j++) {
			ADDC(buffer,j) = *p;
			if(*p == '\n') {
				if(strncmp(lineprefix[i].search,p+1,l) != 0)
					break;
				p += l;
			}
		}
		ADDC(buffer,j) = '\0';
		if(lineprefix[i].process)
			process(buffer,buffer+strlen(buffer));
		else
			hprint(buffer,buffer+strlen(buffer));
		printf("</%s>\n",lineprefix[i].tag);
		free(buffer);
		return p - begin;
	}
	return 0;
}

unsigned int
dolink(const char *begin, const char *end) {
	int img;
	const char *desc, *link, *p, *q, *r, *descend, *linkend;

	if(*begin == '[')
		img = 0;
	else if(strncmp(begin,"![",2) == 0)
		img = 1;
	else
		return 0;
	p = desc = begin + 1 + img;
	if(!(p = strstr(desc, "](")) || p > end)
		return 0;
	for(q = strstr(desc, "!["); q && q < end && q < p; q = strstr(q+1,"!["))
		if(!(p = strstr(p+1, "](")) || p > end)
			return 0;
	descend = p;
	link = p + 2;
	if(!(p = strstr(link, ")")) || p > end)
		return 0;
	linkend = p;
	if(img) {
		fputs("<img src=\"",stdout);
		process(link,linkend);
		fputs("\" alt=\"",stdout);
		process(desc,descend);
		fputs("\" />",stdout);
	}
	else {
		fputs("<a href=\"",stdout);
		process(link,linkend);
		fputs("\">",stdout);
		process(desc,descend);
		fputs("</a>",stdout);
	}
	return p + 1 - begin;
}

unsigned int
dolist(const char *begin, const char *end) {
	unsigned int i,j,k,indent,run,ul;
	const char *p, *q;
	char *buffer;

	if(*begin != '\n')
		return 0;
	p = begin;
	if(p[1] == '\n')
		p++;
	q = p;
	if((p[1] == '-' || p[1] == '*' || p[1] == '+') && p[2] == ' ') {
		ul = 1;
		p++;
	}
	else {
		ul = 0;
		for(p++; *p && p != end && *p >= '0' && *p <= '9';p++);
		if(!*p || p[0] != '.' || p[1] != ' ')
			return 0;
	}
	for(p++; *p && p != end && *p == ' '; p++);
	indent = p - q - 1;

	if(!(buffer = malloc(BUFFERSIZE)))
		eprint("Malloc failed.");

	puts(ul ? "<ul>" : "<ol>");
	run = 1;
	for(i = 0; *p && p < end && run; p++) {
		buffer[0] = '\0';
		for(i = 0; *p && p < end && run; p++,i++) {
			if(*p == '\n') {
				if(p[1] == '\n') {
					p++;
					run = 0;
				}
				q = p + 1;
				if(ul && (*q == '-' || *q == '*' || *q == '+'))
					j = 1;
				else {
					for(j = 0; q[j] >= '0' && q[j] <= '9' && j < indent; j++);
					if(j > 0 && q[j] == '.')
						j++;
					else
						j = 0;
				}
				for(;q[j] == ' ' && j < indent; j++);
				if(j == indent) {
					p += indent;
					run = 1;
					if(q[1] == ' ')
						p++;
					else
						break;
					ADDC(buffer,i) = '\n';
					i++;
				}
				ADDC(buffer,i) = '\n';
				i++;
			}
			ADDC(buffer,i) = *p;
		}
		ADDC(buffer,i) = '\0';
		if(run == 0)
			while(buffer[--i] == '\n')
				buffer[i] = '\0';
		fputs("<li>",stdout);
		process(buffer,buffer+i+1);
		fputs("</li>\n",stdout);
	}
	puts(ul ? "</ul>" : "</ol>");
	free(buffer);
	p--;
	while(*(--p) == '\n');
	return p + 1 - begin;
}

unsigned int
doparagraph(const char *begin, const char *end) {
	const char *p;

	if(strncmp(begin,"\n\n",2))
		return 0;
	if(!(p = strstr(begin + 2,"\n\n")))
		p = end - 1;
	if(p > end)
		return 0;
	if(p - begin - 2 <= 0) 
		return 0;
	fputs("\n<p>",stdout);
	process(begin+2,p);
	fputs("</p>\n",stdout);
	return p - begin;
}

unsigned int
doreplace(const char *begin, const char *end) {
	unsigned int i, l;

	for(i = 0; i < LENGTH(insert); i++)
		if(strncmp(insert[i][0],begin,strlen(insert[i][0])) == 0)
			fputs(insert[i][1], stdout);
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
doshortlink(const char *begin, const char *end) {
	const char *p, *c;
	int ismail = 0;

	if(*begin != '<')
		return 0;
	for(p = begin+1; p && p != end && !strchr(" \t\n",*p); p++) {
		switch(*p) {
		case '#':
		case ':':
			ismail = -1;
			break;
		case '@':
			if(ismail == 0)
				ismail = 1;
			break;
		case '>':
			if(ismail == 0)
				return 0;
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
				hprint(begin+1,p);
				fputs("\">",stdout);
				hprint(begin+1,p);
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
dounderline(const char *begin, const char *end) {
	unsigned int i, j, l, nl;
	const char *p;

	if(*begin != '\n' && *begin != '\0')
		return 0;
	nl = 0;
	p = begin;
	if(p[1] == '\n') {
		nl = 1;
		p++;
	}
	for(p++,l = 0; p[l] != '\n' && p[l] && p+l != end; l++);
	p += l + 1;
	if(l == 0)
		return 0;
	for(i = 0; i < LENGTH(underline); i++) {
		for(j = 0; p[j] != '\n' && p[j] == underline[i].search[0] && p+j != end; j++);
		if(j >= l) {
			putchar('\n');
			printf("<%s>",underline[i].tag);
			if(underline[i].process)
				process(begin + 1 + nl, begin + l + 1 + nl);
			else
				hprint(begin + 1 + nl, begin + l + 1 + nl);
			printf("</%s>\n",underline[i].tag);
			return j + p - begin;
		}
	}
	return 0;
}

void
process(const char *begin, const char *end) {
	const char *p, *q;
	int affected;
	unsigned int i;
	
	for(p = begin; *p && p < end;) {
		affected = 0;
		for(i = 0; i < LENGTH(parsers) && affected == 0; i++)
			affected = parsers[i](p, end);
		if(affected)
			p += affected;
		else {
			for(q = p; *q == '\n' && q != end; q++);
			if(q == end)
				return;
			else if(nohtml)
				hprint(p,p+1);
			else
				putchar(*p);
			p++;
		}
	}
}

int
main(int argc, char *argv[]) {
	char *p, *buffer;
	int s;

	source = stdin;
	if(argc > 1 && strcmp("-v", argv[1]) == 0)
		eprint("markdown in C %s (C) Enno Boland\n",VERSION);
	else if(argc > 1 && strcmp("-h", argv[1]) == 0)
		eprint("Usage %s [-n] [file]\n -n escape html strictly\n",argv[0]);
	if(argc > 1 && strcmp("-n", argv[1]) == 0)
		nohtml = 1;
	if(argc > 1 + nohtml && strcmp("-", argv[1 + nohtml]) != 0 && !(source = fopen(argv[1 + nohtml],"r")))
		eprint("Cannot open file `%s`\n",argv[1 + nohtml]);
	if(!(buffer = malloc(BUFFERSIZE)))
		eprint("Malloc failed.");
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
				eprint("Malloc failed.");
		}
	}
	process(buffer,buffer+strlen(buffer));
	putchar('\n');
	free(buffer);
}
