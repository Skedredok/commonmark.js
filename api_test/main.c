#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmark.h"
#include "node.h"

#include "harness.h"

static const cmark_node_type node_types[] = {
	CMARK_NODE_DOCUMENT,
	CMARK_NODE_BQUOTE,
	CMARK_NODE_LIST,
	CMARK_NODE_LIST_ITEM,
	CMARK_NODE_FENCED_CODE,
	CMARK_NODE_INDENTED_CODE,
	CMARK_NODE_HTML,
	CMARK_NODE_PARAGRAPH,
	CMARK_NODE_ATX_HEADER,
	CMARK_NODE_SETEXT_HEADER,
	CMARK_NODE_HRULE,
	CMARK_NODE_REFERENCE_DEF,
	CMARK_NODE_STRING,
	CMARK_NODE_SOFTBREAK,
	CMARK_NODE_LINEBREAK,
	CMARK_NODE_INLINE_CODE,
	CMARK_NODE_INLINE_HTML,
	CMARK_NODE_EMPH,
	CMARK_NODE_STRONG,
	CMARK_NODE_LINK,
	CMARK_NODE_IMAGE
};
static const int num_node_types = sizeof(node_types) / sizeof(*node_types);

static void
test_content(test_batch_runner *runner, cmark_node_type type,
	     int allowed_content);

static void
constructor(test_batch_runner *runner)
{
	for (int i = 0; i < num_node_types; ++i) {
		cmark_node_type type = node_types[i];
		cmark_node *node = cmark_node_new(type);
		OK(runner, node != NULL, "new type %d", type);

		switch (node->type) {
		case CMARK_NODE_ATX_HEADER:
		case CMARK_NODE_SETEXT_HEADER:
			INT_EQ(runner, cmark_node_get_header_level(node), 1,
			       "default header level is 1");
			node->as.header.level = 1;
			break;

		case CMARK_NODE_LIST:
			INT_EQ(runner, cmark_node_get_list_type(node),
			       CMARK_BULLET_LIST,
			       "default is list type is bullet");
			INT_EQ(runner, cmark_node_get_list_start(node), 1,
			       "default is list start is 1");
			INT_EQ(runner, cmark_node_get_list_tight(node), 0,
			       "default is list is loose");
			break;

		default:
			break;
		}

		cmark_node_destroy(node);
	}
}

static void
accessors(test_batch_runner *runner)
{
	static const unsigned char markdown[] =
		"## Header\n"
		"\n"
		"* Item 1\n"
		"* Item 2\n"
		"\n"
		"2. Item 1\n"
		"\n"
		"3. Item 2\n"
		"\n"
		"\n"
		"    code\n"
		"\n"
		"``` lang\n"
		"fenced\n"
		"```\n"
		"\n"
		"<div>html</div>\n"
		"\n"
		"[link](url 'title')\n";

	cmark_node *doc = cmark_parse_document(markdown, sizeof(markdown) - 1);

	// Getters

	cmark_node *header = cmark_node_first_child(doc);
	INT_EQ(runner, cmark_node_get_type(header), CMARK_NODE_ATX_HEADER,
	       "get_type header");
	INT_EQ(runner, cmark_node_get_header_level(header), 2,
	       "get_header_level");

	cmark_node *bullet_list = cmark_node_next(header);
	INT_EQ(runner, cmark_node_get_type(bullet_list), CMARK_NODE_LIST,
	       "get_type bullet list");
	INT_EQ(runner, cmark_node_get_list_type(bullet_list),
	       CMARK_BULLET_LIST, "get_list_type bullet");
	INT_EQ(runner, cmark_node_get_list_tight(bullet_list), 1,
	       "get_list_tight tight");

	cmark_node *ordered_list = cmark_node_next(bullet_list);
	INT_EQ(runner, cmark_node_get_type(ordered_list), CMARK_NODE_LIST,
	       "get_type ordered list");
	INT_EQ(runner, cmark_node_get_list_type(ordered_list),
	       CMARK_ORDERED_LIST, "get_list_type ordered");
	INT_EQ(runner, cmark_node_get_list_start(ordered_list), 2,
	       "get_list_start");
	INT_EQ(runner, cmark_node_get_list_tight(ordered_list), 0,
	       "get_list_tight loose");

	cmark_node *code = cmark_node_next(ordered_list);
	INT_EQ(runner, cmark_node_get_type(code), CMARK_NODE_INDENTED_CODE,
	       "get_type indented code");
	STR_EQ(runner, cmark_node_get_string_content(code), "code\n",
	       "get_string_content indented code");

	cmark_node *fenced = cmark_node_next(code);
	INT_EQ(runner, cmark_node_get_type(fenced), CMARK_NODE_FENCED_CODE,
	       "get_type fenced code");
	STR_EQ(runner, cmark_node_get_string_content(fenced), "fenced\n",
	       "get_string_content fenced code");
	STR_EQ(runner, cmark_node_get_fence_info(fenced), "lang",
	       "get_fence_info");

	cmark_node *html = cmark_node_next(fenced);
	INT_EQ(runner, cmark_node_get_type(html), CMARK_NODE_HTML,
	       "get_type html");
	STR_EQ(runner, cmark_node_get_string_content(html),
	       "<div>html</div>\n", "get_string_content html");

	cmark_node *paragraph = cmark_node_next(html);
	INT_EQ(runner, cmark_node_get_type(paragraph), CMARK_NODE_PARAGRAPH,
	       "get_type paragraph");
	INT_EQ(runner, cmark_node_get_start_line(paragraph), 19,
	       "get_start_line");
	INT_EQ(runner, cmark_node_get_start_column(paragraph), 1,
	       "get_start_column");
	INT_EQ(runner, cmark_node_get_end_line(paragraph), 19,
	       "get_end_line");

	cmark_node *link = cmark_node_first_child(paragraph);
	INT_EQ(runner, cmark_node_get_type(link), CMARK_NODE_LINK,
	       "get_type link");
	STR_EQ(runner, cmark_node_get_url(link), "url",
	       "get_url");
	STR_EQ(runner, cmark_node_get_title(link), "title",
	       "get_title");

	cmark_node *string = cmark_node_first_child(link);
	INT_EQ(runner, cmark_node_get_type(string), CMARK_NODE_STRING,
	       "get_type string");
	STR_EQ(runner, cmark_node_get_string_content(string), "link",
	       "get_string_content string");

	// Setters

	OK(runner, cmark_node_set_header_level(header, 3),
	   "set_header_level");

	OK(runner, cmark_node_set_list_type(bullet_list, CMARK_ORDERED_LIST),
	   "set_list_type ordered");
	OK(runner, cmark_node_set_list_start(bullet_list, 3),
	   "set_list_start");
	OK(runner, cmark_node_set_list_tight(bullet_list, 0),
	   "set_list_tight loose");

	OK(runner, cmark_node_set_list_type(ordered_list, CMARK_BULLET_LIST),
	   "set_list_type bullet");
	OK(runner, cmark_node_set_list_tight(ordered_list, 1),
	   "set_list_tight tight");

	OK(runner, cmark_node_set_string_content(code, "CODE\n"),
	   "set_string_content indented code");

	OK(runner, cmark_node_set_string_content(fenced, "FENCED\n"),
	   "set_string_content fenced code");
	OK(runner, cmark_node_set_fence_info(fenced, "LANG"),
	   "set_fence_info");

	OK(runner, cmark_node_set_string_content(html, "<div>HTML</div>\n"),
	   "set_string_content html");

	OK(runner, cmark_node_set_url(link, "URL"),
	   "set_url");
	OK(runner, cmark_node_set_title(link, "TITLE"),
	   "set_title");

	OK(runner, cmark_node_set_string_content(string, "LINK"),
	   "set_string_content string");

	char *rendered_html = (char *)cmark_render_html(doc);
	static const char expected_html[] =
		"<h3>Header</h3>\n"
		"<ol start=\"3\">\n"
		"<li>\n"
		"<p>Item 1</p>\n"
		"</li>\n"
		"<li>\n"
		"<p>Item 2</p>\n"
		"</li>\n"
		"</ol>\n"
		"<ul start=\"2\">\n"
		"<li>Item 1</li>\n"
		"<li>Item 2</li>\n"
		"</ul>\n"
		"<pre><code>CODE\n"
		"</code></pre>\n"
		"<pre><code class=\"language-LANG\">FENCED\n"
		"</code></pre>\n"
		"<div>HTML</div>\n"
		"<p><a href=\"URL\" title=\"TITLE\">LINK</a></p>\n";
	STR_EQ(runner, rendered_html, expected_html, "setters work");
	free(rendered_html);

	// Getter errors

	INT_EQ(runner, cmark_node_get_header_level(bullet_list), 0,
	       "get_header_level error");
	INT_EQ(runner, cmark_node_get_list_type(header), CMARK_NO_LIST,
	       "get_list_type error");
	INT_EQ(runner, cmark_node_get_list_start(code), 0,
	       "get_list_start error");
	INT_EQ(runner, cmark_node_get_list_tight(fenced), 0,
	       "get_list_tight error");
	OK(runner, cmark_node_get_string_content(ordered_list) == NULL,
	   "get_string_content error");
	OK(runner, cmark_node_get_fence_info(paragraph) == NULL,
	   "get_fence_info error");
	OK(runner, cmark_node_get_url(html) == NULL,
	   "get_url error");
	OK(runner, cmark_node_get_title(header) == NULL,
	   "get_title error");

	// Setter errors

	OK(runner, !cmark_node_set_header_level(bullet_list, 3),
	   "set_header_level error");
	OK(runner, !cmark_node_set_list_type(header, CMARK_ORDERED_LIST),
	   "set_list_type error");
	OK(runner, !cmark_node_set_list_start(code, 3),
	   "set_list_start error");
	OK(runner, !cmark_node_set_list_tight(fenced, 0),
	   "set_list_tight error");
	OK(runner, !cmark_node_set_string_content(ordered_list, "content\n"),
	   "set_string_content error");
	OK(runner, !cmark_node_set_fence_info(paragraph, "lang"),
	   "set_fence_info error");
	OK(runner, !cmark_node_set_url(html, "url"),
	   "set_url error");
	OK(runner, !cmark_node_set_title(header, "title"),
	   "set_title error");

	OK(runner, !cmark_node_set_header_level(header, 0),
	   "set_header_level too small");
	OK(runner, !cmark_node_set_header_level(header, 7),
	   "set_header_level too large");
	OK(runner, !cmark_node_set_list_type(bullet_list, CMARK_NO_LIST),
	   "set_list_type invalid");
	OK(runner, !cmark_node_set_list_start(bullet_list, -1),
	   "set_list_start negative");

	cmark_node_destroy(doc);
}

static void
create_tree(test_batch_runner *runner)
{
	char *html;
	cmark_node *doc = cmark_node_new(CMARK_NODE_DOCUMENT);

	cmark_node *p = cmark_node_new(CMARK_NODE_PARAGRAPH);
	OK(runner, cmark_node_append_child(doc, p), "append1");
	INT_EQ(runner, cmark_node_check(doc), 0, "append1 consistent");

	cmark_node *emph = cmark_node_new(CMARK_NODE_EMPH);
	OK(runner, cmark_node_prepend_child(p, emph), "prepend1");
	INT_EQ(runner, cmark_node_check(doc), 0, "prepend1 consistent");

	cmark_node *str1 = cmark_node_new(CMARK_NODE_STRING);
	cmark_node_set_string_content(str1, "Hello, ");
	OK(runner, cmark_node_prepend_child(p, str1), "prepend2");
	INT_EQ(runner, cmark_node_check(doc), 0, "prepend2 consistent");

	cmark_node *str3 = cmark_node_new(CMARK_NODE_STRING);
	cmark_node_set_string_content(str3, "!");
	OK(runner, cmark_node_append_child(p, str3), "append2");
	INT_EQ(runner, cmark_node_check(doc), 0, "append2 consistent");

	cmark_node *str2 = cmark_node_new(CMARK_NODE_STRING);
	cmark_node_set_string_content(str2, "world");
	OK(runner, cmark_node_append_child(emph, str2), "append3");
	INT_EQ(runner, cmark_node_check(doc), 0, "append3 consistent");

	html = (char *)cmark_render_html(doc);
	STR_EQ(runner, html, "<p>Hello, <em>world</em>!</p>\n",
	       "render_html");
	free(html);

	OK(runner, cmark_node_insert_before(str1, str3), "ins before1");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins before1 consistent");
	// 31e
	OK(runner, cmark_node_first_child(p) == str3, "ins before1 works");

	OK(runner, cmark_node_insert_before(str1, emph), "ins before2");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins before2 consistent");
	// 3e1
	OK(runner, cmark_node_last_child(p) == str1, "ins before2 works");

	OK(runner, cmark_node_insert_after(str1, str3), "ins after1");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins after1 consistent");
	// e13
	OK(runner, cmark_node_last_child(p) == str3, "ins after1 works");

	OK(runner, cmark_node_insert_after(str1, emph), "ins after2");
	INT_EQ(runner, cmark_node_check(doc), 0, "ins after2 consistent");
	// 1e3
	OK(runner, cmark_node_first_child(p) == str1, "ins after2 works");

	html = (char *)cmark_render_html(doc);
	STR_EQ(runner, html, "<p>Hello, <em>world</em>!</p>\n",
	       "render_html after shuffling");
	free(html);

	cmark_node_destroy(doc);
}

void
hierarchy(test_batch_runner *runner)
{
	cmark_node *bquote1 = cmark_node_new(CMARK_NODE_BQUOTE);
	cmark_node *bquote2 = cmark_node_new(CMARK_NODE_BQUOTE);
	cmark_node *bquote3 = cmark_node_new(CMARK_NODE_BQUOTE);

	OK(runner, cmark_node_append_child(bquote1, bquote2),
	   "append bquote2");
	OK(runner, cmark_node_append_child(bquote2, bquote3),
	   "append bquote3");
	OK(runner, !cmark_node_append_child(bquote3, bquote3),
	   "adding a node as child of itself fails");
	OK(runner, !cmark_node_append_child(bquote3, bquote1),
	   "adding a parent as child fails");

	cmark_node_destroy(bquote1);

	int max_node_type = CMARK_NODE_LAST_BLOCK > CMARK_NODE_LAST_INLINE
			    ? CMARK_NODE_LAST_BLOCK : CMARK_NODE_LAST_INLINE;
	OK(runner, max_node_type < 32, "all node types < 32");

	int list_item_flag = 1 << CMARK_NODE_LIST_ITEM;
	int top_level_blocks =
		(1 << CMARK_NODE_BQUOTE) |
		(1 << CMARK_NODE_LIST) |
		(1 << CMARK_NODE_FENCED_CODE) |
		(1 << CMARK_NODE_INDENTED_CODE) |
		(1 << CMARK_NODE_HTML) |
		(1 << CMARK_NODE_PARAGRAPH) |
		(1 << CMARK_NODE_ATX_HEADER) |
		(1 << CMARK_NODE_SETEXT_HEADER) |
		(1 << CMARK_NODE_HRULE) |
		(1 << CMARK_NODE_REFERENCE_DEF);
	int all_inlines =
		(1 << CMARK_NODE_STRING) |
		(1 << CMARK_NODE_SOFTBREAK) |
		(1 << CMARK_NODE_LINEBREAK) |
		(1 << CMARK_NODE_INLINE_CODE) |
		(1 << CMARK_NODE_INLINE_HTML) |
		(1 << CMARK_NODE_EMPH) |
		(1 << CMARK_NODE_STRONG) |
		(1 << CMARK_NODE_LINK) |
		(1 << CMARK_NODE_IMAGE);

	test_content(runner, CMARK_NODE_DOCUMENT,      top_level_blocks);
	test_content(runner, CMARK_NODE_BQUOTE,        top_level_blocks);
	test_content(runner, CMARK_NODE_LIST,          list_item_flag);
	test_content(runner, CMARK_NODE_LIST_ITEM,     top_level_blocks);
	test_content(runner, CMARK_NODE_FENCED_CODE,   0);
	test_content(runner, CMARK_NODE_INDENTED_CODE, 0);
	test_content(runner, CMARK_NODE_HTML,          0);
	test_content(runner, CMARK_NODE_PARAGRAPH,     all_inlines);
	test_content(runner, CMARK_NODE_ATX_HEADER,    all_inlines);
	test_content(runner, CMARK_NODE_SETEXT_HEADER, all_inlines);
	test_content(runner, CMARK_NODE_HRULE,         0);
	test_content(runner, CMARK_NODE_REFERENCE_DEF, 0);
	test_content(runner, CMARK_NODE_STRING,        0);
	test_content(runner, CMARK_NODE_SOFTBREAK,     0);
	test_content(runner, CMARK_NODE_LINEBREAK,     0);
	test_content(runner, CMARK_NODE_INLINE_CODE,   0);
	test_content(runner, CMARK_NODE_INLINE_HTML,   0);
	test_content(runner, CMARK_NODE_EMPH,          all_inlines);
	test_content(runner, CMARK_NODE_STRONG,        all_inlines);
	test_content(runner, CMARK_NODE_LINK,          all_inlines);
	test_content(runner, CMARK_NODE_IMAGE,         all_inlines);
}

static void
test_content(test_batch_runner *runner, cmark_node_type type,
	     int allowed_content)
{
	cmark_node *node = cmark_node_new(type);

	for (int i = 0; i < num_node_types; ++i) {
		cmark_node_type child_type = node_types[i];
		cmark_node *child = cmark_node_new(child_type);

		int got = cmark_node_append_child(node, child);
		int expected = (allowed_content >> i) & 1;

		INT_EQ(runner, got, expected,
		       "add %d as child of %d", child_type, type);

		cmark_node_destroy(child);
	}

	cmark_node_destroy(node);
}

int main() {
	int retval;
	test_batch_runner *runner = test_batch_runner_new();

	constructor(runner);
	accessors(runner);
	create_tree(runner);
	hierarchy(runner);

	test_print_summary(runner);
	retval =  test_ok(runner) ? 0 : 1;
	free(runner);

	return retval;
}

