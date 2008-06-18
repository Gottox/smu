/* libsmu - simple markup library
 * Copyright (C) <2007, 2008> Enno Boland <g s01 de>
 *
 * See LICENSE for further informations
 */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "smu.h"

#define BUFFERSIZE 512
#define LENGTH(x) sizeof(x)/sizeof(x[0])
#define ADDC(b,i) if(i % BUFFERSIZE == 0) \
	{ b = realloc(b, (i + BUFFERSIZE) * sizeof(b)); if(!b) eprint("Malloc failed."); } b[i]

typedef int (*Parser)(const char *, const char *, int);
typedef struct {
	char *search;
	int process;
	char *before, *after;
} Tag;

static int doamp(const char *begin, const char *end, int newblock);       /* Parser for & */
static int dogtlt(const char *begin, const char *end, int newblock);      /* Parser for < and > */
static int dohtml(const char *begin, const char *end, int newblock);      /* Parser for html */
static int dolineprefix(const char *begin, const char *end, int newblock);/* Parser for line prefix tags */
static int dolink(const char *begin, const char *end, int newblock);      /* Parser for links and images */
static int dolist(const char *begin, const char *end, int newblock);      /* Parser for lists */
static int doparagraph(const char *begin, const char *end, int newblock); /* Parser for paragraphs */
static int doreplace(const char *begin, const char *end, int newblock);   /* Parser for simple replaces */
static int doshortlink(const char *begin, const char *end, int newblock); /* Parser for links and images */
static int dosurround(const char *begin, const char *end, int newblock);  /* Parser for surrounding tags */
static int dounderline(const char *begin, const char *end, int newblock); /* Parser for underline tags */
static void hprint(const char *begin, const char *end);                   /* escapes HTML and prints it to output */
static void process(const char *begin, const char *end, int isblock);     /* Processes range between begin and end. */

/* list of parsers */
static Parser parsers[] = { dounderline, dohtml, dolineprefix, dolist,
                            doparagraph, dogtlt, dosurround, dolink,
                            doshortlink, doamp, doreplace };
static FILE *output;
static int nohtml = 1;

static Tag lineprefix[] = {
	{ "   ",	0,	"<pre><code>", "</code></pre>" },
	{ "\t",		0,	"<pre><code>", "</code></pre>" },
	{ "> ",		2,	"<blockquote>",	"</blockquote>" },
	{ "###### ",	1,	"<h6>",		"</h6>" },
	{ "##### ",	1,	"<h5>",		"</h5>" },
	{ "#### ",	1,	"<h4>",		"</h4>" },
	{ "### ",	1,	"<h3>",		"</h3>" },
	{ "## ",	1,	"<h2>",		"</h2>" },
	{ "# ",		1,	"<h1>",		"</h1>" },
	{ "- - -\n",	1,	"<hr />",	""},
};

static Tag underline[] = {
	{ "=",		1,	"<h1>",		"</h1>\n" },
	{ "-",		1,	"<h2>",		"</h2>\n" },
};

static Tag surround[] = {
	{ "``",		0,	"<code>",	"</code>" },
	{ "`",		0,	"<code>",	"</code>" },
	{ "___",	1,	"<strong><em>",	"</em></strong>" },
	{ "***",	1,	"<strong><em>",	"</em></strong>" },
	{ "__",		1,	"<strong>",	"</strong>" },
	{ "**",		1,	"<strong>",	"</strong>" },
	{ "_",		1,	"<em>",		"</em>" },
	{ "*",		1,	"<em>",		"</em>" },
};

static const char *replace[][2] = {
	{ "\\\\",	"\\" },
	{ "\\`",	"`" },
	{ "\\*",	"*" },
	{ "\\_",	"_" },
	{ "\\{",	"{" },
	{ "\\}",	"}" },
	{ "\\[",	"[" },
	{ "\\]",	"]" },
	{ "\\(",	"(" },
	{ "\\)",	")" },
	{ "\\#",	"#" },
	{ "\\+",	"+" },
	{ "\\-",	"-" },
	{ "\\.",	"." },
	{ "\\!",	"!" },
};

static const char *insert[][2] = {
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

int
doamp(const char *begin, const char *end, int newblock) {
	const char *p;

	if(*begin != '&')
		return 0;
	if(!nohtml) {
		for(p = begin + 1; p != end && !strchr("; \\\n\t", *p); p++);
		if(p == end || *p == ';')
			return 0;
	}
	fputs("&amp;", output);
	return 1;
}

int
dogtlt(const char *begin, const char *end, int newblock) {
	int brpos;
	char c;

	if(nohtml || begin + 1 >= end)
		return 0;
	brpos = begin[1] == '>';
	if(!brpos && *begin != '<')
		return 0;
	c = begin[brpos ? 0 : 1];
	if(!brpos && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z')) {
		fputs("&lt;", output);
		return 1;
	}
	else if(brpos && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && !strchr("/\"'",c)) {
		fprintf(output, "%c&gt;",c);
		return 2;
	}
	return 0;
}

int
dohtml(const char *begin, const char *end, int newblock) {
	const char *p, *tag, *tagend;

	if(nohtml || !newblock || *begin == '\n' || begin + 2 >= end)
		return 0;
	p = begin;
	if(p[1] == '\n')
		p++;
	if(p[1] != '<' || strchr(" /\n\t\\", p[2]))
		return 0;
	tag = p + 2;
	p += 2;
	for(; !strchr(" >", *p); p++);
	tagend = p;
	while((p = strstr(p, "\n</")) && p < end) {
		p += 3;
		if(strncmp(p, tag, tagend - tag) == 0 && p[tagend - tag] == '>') {
			p++;
			fwrite(begin, sizeof(char), p - begin + tagend - tag, output);
			puts("\n");
			return -(p - begin + tagend - tag);
		}
	}
	return 0;
}

int
dolineprefix(const char *begin, const char *end, int newblock) {
	unsigned int i, j, l;
	char *buffer;
	const char *p;

	if(newblock)
		p = begin;
	else if(*begin == '\n')
		p = begin + 1;
	else
		return 0;
	for(i = 0; i < LENGTH(lineprefix); i++) {
		l = strlen(lineprefix[i].search);
		if(end - p < l)
			continue;
		if(strncmp(lineprefix[i].search, p, l))
			continue;
		if(*begin == '\n')
			fputc('\n', output);
		fputs(lineprefix[i].before, output);
		if(lineprefix[i].search[l-1] == '\n') {
			fputc('\n', output);
			return l;
		}
		if(!(buffer = malloc(BUFFERSIZE)))
			eprint("Malloc failed.");
		buffer[0] = '\0';
		for(j = 0, p += l; p < end; p++, j++) {
			ADDC(buffer, j) = *p;
			if(*p == '\n' && p + l < end) {
				if(strncmp(lineprefix[i].search, p + 1, l) != 0)
					break;
				p += l;
			}
		}
		ADDC(buffer, j) = '\0';
		if(lineprefix[i].process)
			process(buffer, buffer + strlen(buffer), lineprefix[i].process >= 2);
		else
			hprint(buffer, buffer + strlen(buffer));
		puts(lineprefix[i].after);
		free(buffer);
		return -(p - begin);
	}
	return 0;
}

int
dolink(const char *begin, const char *end, int newblock) {
	int img;
	const char *desc, *link, *p, *q, *descend, *linkend;

	if(*begin == '[')
		img = 0;
	else if(strncmp(begin, "![", 2) == 0)
		img = 1;
	else
		return 0;
	p = desc = begin + 1 + img;
	if(!(p = strstr(desc, "](")) || p > end)
		return 0;
	for(q = strstr(desc, "!["); q && q < end && q < p; q = strstr(q + 1, "!["))
		if(!(p = strstr(p + 1, "](")) || p > end)
			return 0;
	descend = p;
	link = p + 2;
	if(!(p = strstr(link, ")")) || p > end)
		return 0;
	linkend = p;
	if(img) {
		fputs("<img src=\"", output);
		hprint(link, linkend);
		fputs("\" alt=\"", output);
		hprint(desc, descend);
		fputs("\" />", output);
	}
	else {
		fputs("<a href=\"", output);
		hprint(link, linkend);
		fputs("\">", output);
		process(desc, descend, 0);
		fputs("</a>", output);
	}
	return p + 1 - begin;
}

int
dolist(const char *begin, const char *end, int newblock) {
	unsigned int i, j, indent, run, ul, isblock;
	const char *p, *q;
	char *buffer;

	isblock = 0;
	if(newblock)
		p = begin;
	else if(*begin == '\n')
		p = begin + 1;
	else
		return 0;
	q = p;
	if(*p == '-' || *p == '*' || *p == '+')
		ul = 1;
	else {
		ul = 0;
		for(; p < end && *p >= '0' && *p <= '9'; p++);
		if(p >= end || *p != '.')
			return 0;
	}
	p++;
	if(p >= end || !(*p == ' ' || *p == '\t'))
		return 0;
	for(p++; p != end && (*p == ' ' || *p == '\t'); p++);
	indent = p - q;
	if(!(buffer = malloc(BUFFERSIZE)))
		eprint("Malloc failed.");
	if(!newblock)
		fputc('\n', output);
	fputs(ul ? "<ul>\n" : "<ol>\n", output);
	run = 1;
	for(; p < end && run; p++) {
		for(i = 0; p < end && run; p++, i++) {
			if(*p == '\n') {
				if(p + 1 == end)
					break;
				else if(p[1] == '\n') {
					p++;
					ADDC(buffer, i) = '\n';
					i++;
					run = 0;
					isblock++;
				}
				q = p + 1;
				j = 0;
				if(ul && (*q == '-' || *q == '*' || *q == '+'))
					j = 1;
				else if(!ul) {
					for(; q + j != end && q[j] >= '0' && q[j] <= '9' && j < indent; j++);
					if(q + j == end)
						break;
					if(j > 0 && q[j] == '.')
						j++;
					else
						j = 0;
				}
				if(q + indent < end)
					for(; (q[j] == ' ' || q[j] == '\t') && j < indent; j++);
				if(j == indent) {
					ADDC(buffer, i) = '\n';
					i++;
					p += indent;
					run = 1;
					if(*q == ' ' || *q == '\t')
						p++;
					else
						break;
				}
			}
			ADDC(buffer, i) = *p;
		}
		ADDC(buffer, i) = '\0';
		fputs("<li>", output);
		process(buffer, buffer + i, isblock > 1 || (isblock == 1 && run));
		fputs("</li>\n", output);
	}
	fputs(ul ? "</ul>\n" : "</ol>\n", output);
	free(buffer);
	p--;
	while(*(--p) == '\n');
	return -(p - begin + 1);
}

int
doparagraph(const char *begin, const char *end, int newblock) {
	const char *p;

	if(!newblock)
		return 0;
	p = strstr(begin, "\n\n");
	if(!p || p > end)
		p = end;
	if(p - begin <= 1)
		return 0;
	fputs("<p>\n", output);
	process(begin, p, 0);
	fputs("</p>\n", output);
	return -(p - begin);
}

int
doreplace(const char *begin, const char *end, int newblock) {
	unsigned int i, l;

	for(i = 0; i < LENGTH(insert); i++)
		if(strncmp(insert[i][0], begin, strlen(insert[i][0])) == 0)
			fputs(insert[i][1], output);
	for(i = 0; i < LENGTH(replace); i++) {
		l = strlen(replace[i][0]);
		if(end - begin < l)
			continue;
		if(strncmp(replace[i][0], begin, l) == 0) {
			fputs(replace[i][1], output);
			return l;
		}
	}
	return 0;
}

int
doshortlink(const char *begin, const char *end, int newblock) {
	const char *p, *c;
	int ismail = 0;

	if(*begin != '<')
		return 0;
	for(p = begin + 1; p != end; p++) {
		switch(*p) {
		case ' ':
		case '\t':
		case '\n':
			return 0;
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
			fputs("<a href=\"", output);
			if(ismail == 1) {
				/* mailto: */
				fputs("&#x6D;&#x61;i&#x6C;&#x74;&#x6F;:", output);
				for(c = begin + 1; *c != '>'; c++)
					fprintf(output, "&#%u;", *c);
				fputs("\">", output);
				for(c = begin + 1; *c != '>'; c++)
					fprintf(output, "&#%u;", *c);
			}
			else {
				hprint(begin + 1, p);
				fputs("\">", output);
				hprint(begin + 1, p);
			}
			fputs("</a>", output);
			return p - begin + 1;
		}
	}
	return 0;
}

int
dosurround(const char *begin, const char *end, int newblock) {
	unsigned int i, l;
	const char *p, *start, *stop;

	for(i = 0; i < LENGTH(surround); i++) {
		l = strlen(surround[i].search);
		if(end - begin < 2*l || strncmp(begin, surround[i].search, l) != 0)
			continue;
		start = begin + l;
		p = start - 1;
		do {
			p = strstr(p + 1, surround[i].search);
		} while(p && p[-1] == '\\');
		if(!p || p >= end ||
				!(stop = strstr(start, surround[i].search)) || stop >= end)
			continue;
		fputs(surround[i].before, output);
		if(surround[i].process)
			process(start, stop, 0);
		else
			hprint(start, stop);
		fputs(surround[i].after, output);
		return stop - begin + l;
	}
	return 0;
}

int
dounderline(const char *begin, const char *end, int newblock) {
	unsigned int i, j, l;
	const char *p;
 
	if(!newblock)
		return 0;
	p = begin;
	for(l = 0; p + l != end && p[l] != '\n'; l++);
	p += l + 1;
	if(l == 0)
		return 0;
	for(i = 0; i < LENGTH(underline); i++) {
		for(j = 0; p + j != end && p[j] != '\n' && p[j] == underline[i].search[0]; j++);
		if(j >= l) {
			fputs(underline[i].before, output);
			if(underline[i].process)
				process(begin, begin + l, 0);
			else
				hprint(begin, begin + l);
			fputs(underline[i].after, output);
			return -(j + p - begin);
		}
	}
	return 0;
}

void
hprint(const char *begin, const char *end) {
	const char *p;

	for(p = begin; p != end; p++) {
		if(*p == '&')
			fputs("&amp;", output);
		else if(*p == '"')
			fputs("&quot;", output);
		else if(*p == '>')
			fputs("&gt;", output);
		else if(*p == '<')
			fputs("&lt;", output);
		else
			fputc(*p, output);
	}
}

void
process(const char *begin, const char *end, int newblock) {
	const char *p, *q;
	int affected;
	unsigned int i;
	
	for(p = begin; p != end;) {
		if(newblock)
			while(*p == '\n')
				if(++p == end)
					return;
		affected = 0;
		for(i = 0; i < LENGTH(parsers) && affected == 0; i++)
			affected = parsers[i](p, end, newblock);
		p += abs(affected);
		if(!affected) {
			if(nohtml)
				hprint(p, p + 1);
			else
				fputc(*p, output);
			p++;
		}
		for(q = p; q != end && *q == '\n'; q++);
		if(q == end)
			return;
		else if(p[0] == '\n' && p + 1 != end && p[1] == '\n')
			newblock = 1;
		else
			newblock = affected < 0;
	}
}

/** library call **/

int
smu_convert(FILE *out, FILE *in, int suppresshtml) {
	char *buffer;
	int s;
	unsigned long len, bsize;

	nohtml = suppresshtml;
	output = out;

	bsize = 2 * BUFFERSIZE;
	if(!(buffer = malloc(bsize)))
		eprint("Malloc failed.");
	len = 0;
	while((s = fread(buffer + len, 1, BUFFERSIZE, in))) {
		len += s;
		if(BUFFERSIZE + len + 1 > bsize) {    
			bsize += BUFFERSIZE;
			if(!(buffer = realloc(buffer, bsize)))
				eprint("Malloc failed.");
		}
	}
	buffer[len] = '\0';
	process(buffer, buffer + len, 1);
	free(buffer);

	return EXIT_SUCCESS;
}
