#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

struct Tag {
	char *search;
	int hasChilds;
	char *prefix, *suffix;
};
typedef unsigned int (*Parser)(const char *, const char *);

#define BUFFERSIZE 1024
#define ERRMALLOC eprint("malloc failed\n");
#define LENGTH(x) sizeof(x)/sizeof(x[0])

FILE *source;
unsigned int bsize = 0;
struct Tag lineprefix[] = {
	{ "   ",	0,	"<pre>",	"</pre>" },
	{ "\t",		0,	"<pre>",	"</pre>" },
	{ "> ",		1,	"<blockquote>",	"</blockquote>" },
};

struct Tag underline[] = {
	{ "=",		1,	"<h1>",		"</h1>" },
	{ "-",		1,	"<h2>",		"</h2>" },
};

struct Tag surround[] = {
	{ "_",		1,	"<b>",		"</b>" },
	{ "*",		1,	"<i>",		"</i>" },
};

char * replace[][2] = {
	{ "\n---\n", "\n<hr />\n" },
	{ "\n\n", "\n<br />\n" },
	{ "<", "&lt;" },
	{ ">", "&gt;" },
	{ "&", "&amp;" },
	{ "\"", "&quot;" },
};


void eprint(const char *errstr, ...);
void process(const char *begin, const char *end, Parser parser);
unsigned int doreplace(const char *begin, const char *end);
unsigned int dolineprefix(const char *begin, const char *end);
unsigned int dosurround(const char *begin, const char *end);
unsigned int dounderline(const char *begin, const char *end);
unsigned int dolink(const char *begin, const char *end);
Parser parsers[] = { dounderline, dolineprefix, dosurround, dolink, doreplace };

void
eprint(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
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
		process(link,link+linklen, doreplace);
		fputs("\" alt=\"",stdout);
		process(desc,desc+desclen, NULL);
		fputs("\" />",stdout);
	}
	else {
		fputs("<a href=\"",stdout);
		process(link,link+linklen, doreplace);
		fputs("\">",stdout);
		process(desc,desc+desclen, NULL);
		fputs("</a>",stdout);
	}
	return p + 1 - begin;
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
		p = strstr(begin + strlen(surround[i].search), ps);
		if(!p || p > end)
			continue;
		fputs(surround[i].prefix,stdout);
		fwrite(begin + strlen(surround[i].search), sizeof(char), p - begin - l, stdout);
		fputs(surround[i].suffix,stdout);
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
		fputs(lineprefix[i].prefix, stdout);
		for(p = begin, j = 0; p != end; p++, j++) {
			buffer[j] = *p;
			if(*p == '\n') {
				if(strncmp(lineprefix[i].search,p+1,l) != 0)
					break;
				p += l;
			}
		}
		if(lineprefix[i].hasChilds)
			process(buffer,buffer+strlen(buffer), NULL);
		else
			fputs(buffer, stdout);
		fputs(lineprefix[i].suffix, stdout);
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
	p += l+1;
	if(l == 0)
		return 0;
	for(i = 0; i < LENGTH(underline); i++) {
		for(j = 0; p[j] != '\n' && p[j] == underline[i].search[0] && p+j != end; j++);
		if(j >= l) {
			putchar('\n');
			fputs(underline[i].prefix,stdout);
			if(underline[i].hasChilds)
				process(begin + 1, begin + l + 1, NULL);
			else
				fwrite(begin + 1, l, sizeof(char), stdout);
			fputs(underline[i].suffix,stdout);
			return j + l + 2;
		}
	}
	return 0;
}


void
process(const char *begin, const char *end, Parser parser) {
	const char *p;
	int affected;
	unsigned int i;
	
	for(p = begin; *p && p != end;) {
		affected = 0;
		if(parser)
			affected = parser(p, end);
		else
			for(i = 0; i < LENGTH(parsers) && affected == 0; i++)
				affected = parsers[i](p, end);
		if(affected == 0) {
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
		eprint("Usage markdown [file]\n");
	else if (argc > 1 && strcmp("-", argv[1]) != 0 && !(source = fopen(argv[1],"r")))
		eprint("Cannot open file `%s`\n",argv[1]);
	if(!(buffer = malloc(BUFFERSIZE)))
		ERRMALLOC;
	bsize = BUFFERSIZE;
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
	
	process(buffer,buffer+strlen(buffer), NULL);
	free(buffer);
}
