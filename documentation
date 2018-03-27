smu - a Simple Markup Language
==============================

_smu_ is a very simple and minimal markup language. It is designed for use in
wiki-like environments. smu makes it very easy to write your documents on the
fly and convert them into HTML.

smu is capable of parsing very large documents. It scales just great as long
as you avoid a huge amount of indents (this will be fixed in future releases
of smu).

Syntax
======

smu was started as a rewrite of
[markdown](http://daringfireball.net/projects/markdown/) but became something
more lightweight and consistent. The biggest difference between markdown and smu
is that smu doesn't support _reference style links_

Inline pattern
--------------

There are several pattern you can use to highlight your text:

* Emphasis
  * Surround your text with `*` or `_` to get *emphasised* text:
    	This *is* cool.
    	This _is_ cool, too.
  * Surround your text with `**` or `__` to get **strong** text:
    	This **is** cool.
    	This __is__ cool, too.
  * Surround your text with `***` or `___` to get ***strong and emphasised*** text:
    	This ***is*** cool.
    	This ___is___ cool, too.
  * But this example won't work as expected:
    	***Hello** you*
    This is a wontfix bug because it would make the source too complex.
    Use this instead:
    	***Hello*** *you*

* inline Code

  You can produce inline code with surrounding `\`` or `\`\``

  	Use `rm -rf /` if you're a N00b.

  	Use ``rm -rf /`` if you're a N00b.

  `\`\`ABC\`\`` makes it possible to use Backticks without backslashing them.


Titles
------

Creating titles in smu is very easy. There are two different syntax styles. The
first is underlining:

	Heading
	=======
	
	Topic
	-----

This is very intuitive and self explaining. The resulting sourcecode looks like
this:

	<h1>Heading</h1>
	<h2>Topic</h2>

Use the following prefixes if you don't like underlining:

	# h1
	## h2
	### h3
	#### h4
	##### h5
	###### h6

Links
-----

The simplest way to define a link is with simple `<>`.

	<http://s01.de>

You can do the same for E-Mail addresses:

	<yourname@s01.de>

If you want to define a label for the url, you have to use a different syntax

	[smu - simple mark up](http://s01.de/~gottox/index.cgi/proj_smu)

The resulting HTML-Code

	<a href="http://s01.de/~gottox/index.cgi/proj_smu">smu - simple mark up</a></p>

Lists
-----

Defining lists is very straightforward:

	* Item 1
	* Item 2
	* Item 3

Result:

	<ul>
	<li>Item 1</li>
	<li>Item 2</li>
	<li>Item 3</li>
	</ul>

Defining ordered lists is also very easy:

	1. Item 1
	2. Item 2
	3. Item 3

It is possible to use any leading number you want. So if you don't want to keep
your list synchronised, you simple can use any number. In this case it's
recommended to use `0.`, but it isn't mandatory.

	0. Item 1
	0. Item 2
	0. Item 3

Both examples will cause the same result. Even this is possible:

	1000. Item 1
	432.  Item 2
	0.    Item 3

This will be the result in these example:

	<ol>
	<li>Item 1</li>
	<li>Item 2</li>
	<li>Item 3</li>
	</ol>

Code & Blockquote
-----------------

Use the `> ` as a line prefix for defining blockquotes. Blockquotes are
interpreted as well. This makes it possible to embed links, headings and even
other quotes into a quote:

	> Hello
	> This is a quote with a [link](http://s01.de/~gottox)

Result:
	<blockquote><p>
	Hello
	This is a quote with a <a href="http://s01.de/~gottox">link</a></p>
	</blockquote>


You can define block code with a leading Tab or with __3__ leading spaces

		this.is(code)
	
	   this.is(code, too)

Result:
	<pre><code>this.is(code)</code></pre>
	<pre><code>this.is(code, too)
	</code></pre>

Please note that you can't use HTML or smu syntax in a code block.

Other interesting stuff
-----------------------

* to insert a horizontal rule simple add `- - -` into an empty line:

  	Hello
  	- - -
  	Hello2

  Result:
  	<p>
  	Hello
  	<hr />
  	
  	Hello2</p>

* You can escape the following pattern to avoid them from being interpreted:

  	\ ` * _ { } [ ] ( ) # + - . !

* To force a linebreak simple add two spaces to the end of the line:

  	No linebreak
  	here.
  	But here is  
  	one.

embed HTML
----------
