/* smu - simple markup
 * Copyright (C) <2007, 2008> Enno Boland <g s01 de>
 *
 * See LICENSE for further informations
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define LENGTH(x)  sizeof(x)/sizeof(x[0])
#define ADDC(b,i)  if(i % BUFSIZ == 0) { b = realloc(b, (i + BUFSIZ) * sizeof(char)); if(!b) eprint("Malloc failed."); } b[i]

typedef int (*Parser)(const char *, const char *, int);
typedef struct {
	char *search;
	int process;
	char *before, *after;
} Tag;

static int doamp(const char *begin, const char *end, int newblock);       /* Parser for & */
static int docomment(const char *begin, const char *end, int newblock);   /* Parser for html-comments */
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
static void *ereallocz(void *p, size_t size);
static void hprint(const char *begin, const char *end);                   /* escapes HTML and prints it to output */
static void process(const char *begin, const char *end, int isblock);     /* Processes range between begin and end. */

/* list of parsers */
static Parser parsers[] = { dounderline, docomment, dolineprefix,
                            dolist, doparagraph, dogtlt, dosurround, dolink,
                            doshortlink, dohtml, doamp, doreplace };
static int nohtml = 0;

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
	fputs("&amp;", stdout);
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
		fputs("&lt;", stdout);
		return 1;
	}
	else if(brpos && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') && !strchr("/\"'",c)) {
		fprintf(stdout, "%c&gt;",c);
		return 2;
	}
	return 0;
}

int
docomment(const char *begin, const char *end, int newblock) {
	char *p;

	if(nohtml || strncmp("<!--", begin, 4))
		return 0;
	p = strstr(begin, "-->");
	if(!p || p + 3 >= end)
		return 0;
	return (p + 3 - begin) * (newblock ? -1 : 1);
}

int
dohtml(const char *begin, const char *end, int newblock) {
	const char *p, *tag, *tagend;

	if(nohtml || begin + 2 >= end)
		return 0;
	p = begin;
	if(p[0] != '<' || !isalpha(p[1]))
		return 0;
	p++;
	tag = p;
	for(; isalnum(*p) && p < end; p++);
	tagend = p;
	if(p > end || tag == tagend)
		return 0;
	while((p = strstr(p, "</")) && p < end) {
		p += 2;
		if(strncmp(p, tag, tagend - tag) == 0 && p[tagend - tag] == '>') {
			p++;
			fwrite(begin, sizeof(char), p - begin + tagend - tag + 1, stdout);
			return p - begin + tagend - tag + 1;
		}
	}
	p = strchr(tagend, '>');
	if(p) {
		fwrite(begin, sizeof(char), p - begin + 2, stdout);
		return p - begin + 2;
	}
	else
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
			fputc('\n', stdout);
		fputs(lineprefix[i].before, stdout);
		if(lineprefix[i].search[l-1] == '\n') {
			fputc('\n', stdout);
			return l;
		}
		if(!(buffer = malloc(BUFSIZ)))
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
	int img, len, sep;
	const char *desc, *link, *p, *q, *descend, *linkend;
	const char *title = NULL, *titleend = NULL;

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
	if(!(q = strchr(link, ')')) || q > end)
		return 0;
	if((p = strpbrk(link, "\"'")) && p < end && q > p) {
		sep = p[0]; /* separator: can be " or ' */
		title = p + 1;
		/* strip trailing whitespace */
		for(linkend = p; linkend > link && isspace(*(linkend - 1)); linkend--);
		if(!(p = strchr(title, sep)) || q > end || p > q)
			return 0;
		titleend = p;
		len = p + 2 - begin;
	}
	else {
		linkend = q;
		len = q + 1 - begin;
	}
	if(img) {
		fputs("<img src=\"", stdout);
		hprint(link, linkend);
		fputs("\" alt=\"", stdout);
		hprint(desc, descend);
		fputs("\" ", stdout);
		if(title && titleend) {
			fputs("title=\"", stdout);
			hprint(title, titleend);
			fputs("\" ", stdout);
		}
		fputs("/>", stdout);
	}
	else {
		fputs("<a href=\"", stdout);
		hprint(link, linkend);
		fputs("\"", stdout);
		if(title && titleend) {
			fputs(" title=\"", stdout);
			hprint(title, titleend);
			fputs("\"", stdout);
		}
		fputs(">", stdout);
		process(desc, descend, 0);
		fputs("</a>", stdout);
	}
	return len;
}

int
dolist(const char *begin, const char *end, int newblock) {
	unsigned int i, j, indent, run, ul, isblock;
	const char *p, *q;
	char *buffer = NULL;

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
	buffer = ereallocz(buffer, BUFSIZ);
	if(!newblock)
		fputc('\n', stdout);
	fputs(ul ? "<ul>\n" : "<ol>\n", stdout);
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
		fputs("<li>", stdout);
		process(buffer, buffer + i, isblock > 1 || (isblock == 1 && run));
		fputs("</li>\n", stdout);
	}
	fputs(ul ? "</ul>\n" : "</ol>\n", stdout);
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
	fputs("<p>", stdout);
	process(begin, p, 0);
	fputs("</p>\n", stdout);
	return -(p - begin);
}

int
doreplace(const char *begin, const char *end, int newblock) {
	unsigned int i, l;

	for(i = 0; i < LENGTH(insert); i++)
		if(strncmp(insert[i][0], begin, strlen(insert[i][0])) == 0)
			fputs(insert[i][1], stdout);
	for(i = 0; i < LENGTH(replace); i++) {
		l = strlen(replace[i][0]);
		if(end - begin < l)
			continue;
		if(strncmp(replace[i][0], begin, l) == 0) {
			fputs(replace[i][1], stdout);
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
			fputs("<a href=\"", stdout);
			if(ismail == 1) {
				/* mailto: */
				fputs("&#x6D;&#x61;i&#x6C;&#x74;&#x6F;:", stdout);
				for(c = begin + 1; *c != '>'; c++)
					fprintf(stdout, "&#%u;", *c);
				fputs("\">", stdout);
				for(c = begin + 1; *c != '>'; c++)
					fprintf(stdout, "&#%u;", *c);
			}
			else {
				hprint(begin + 1, p);
				fputs("\">", stdout);
				hprint(begin + 1, p);
			}
			fputs("</a>", stdout);
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
			stop = p;
			p = strstr(p + 1, surround[i].search);
		} while(p && p[-1] == '\\');
		if (p && p[-1] != '\\')
			stop = p;
		if(!stop || stop < start || stop >= end)
			continue;
		fputs(surround[i].before, stdout);
		if(surround[i].process)
			process(start, stop, 0);
		else
			hprint(start, stop);
		fputs(surround[i].after, stdout);
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
			fputs(underline[i].before, stdout);
			if(underline[i].process)
				process(begin, begin + l, 0);
			else
				hprint(begin, begin + l);
			fputs(underline[i].after, stdout);
			return -(j + p - begin);
		}
	}
	return 0;
}

void *
ereallocz(void *p, size_t size) {
	void *res;
	if(p)
		res = realloc(p , size);
	else
		res = calloc(1, size);

	if(!res)
		eprint("fatal: could not malloc() %u bytes\n", size);
	return res;
}

void
hprint(const char *begin, const char *end) {
	const char *p;

	for(p = begin; p != end; p++) {
		if(*p == '&')
			fputs("&amp;", stdout);
		else if(*p == '"')
			fputs("&quot;", stdout);
		else if(*p == '>')
			fputs("&gt;", stdout);
		else if(*p == '<')
			fputs("&lt;", stdout);
		else
			fputc(*p, stdout);
	}
}

void
process(const char *begin, const char *end, int newblock) {
	const char *p, *q;
	int affected;
	unsigned int i;

	for(p = begin; p < end;) {
		if(newblock)
			while(*p == '\n')
				if(++p == end)
					return;
		affected = 0;
		for(i = 0; i < LENGTH(parsers) && !affected; i++)
			affected = parsers[i](p, end, newblock);
		p += abs(affected);
		if(!affected) {
			if(nohtml)
				hprint(p, p + 1);
			else
				fputc(*p, stdout);
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

int
main(int argc, char *argv[]) {
	char *buffer = NULL;
	int s, i;
	unsigned long len, bsize;
	FILE *source = stdin;

	for(i = 1; i < argc; i++) {
		if(!strcmp("-v", argv[i]))
			eprint("simple markup %s (C) Enno Boland\n",VERSION);
		else if(!strcmp("-n", argv[i]))
			nohtml = 1;
		else if(argv[i][0] != '-')
			break;
		else if(!strcmp("--", argv[i])) {
			i++;
			break;
		}
		else
			eprint("Usage %s [-n] [file]\n -n escape html strictly\n", argv[0]);
	}
	if(i < argc && !(source = fopen(argv[i], "r")))
		eprint("Cannot open file `%s`\n",argv[i]);
	bsize = 2 * BUFSIZ;
	buffer = ereallocz(buffer, bsize);
	len = 0;
	while((s = fread(buffer + len, 1, BUFSIZ, source))) {
		len += s;
		if(BUFSIZ + len + 1 > bsize) {
			bsize += BUFSIZ;
			if(!(buffer = realloc(buffer, bsize)))
				eprint("realloc failed.");
		}
	}
	buffer[len] = '\0';
	process(buffer, buffer + len, 1);
	fclose(source);
	free(buffer);
	return EXIT_SUCCESS;
}
