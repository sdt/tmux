/* $Id$ */

/*
 * Copyright (c) 2009 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

struct screen *window_choose_init(struct window_pane *);
void	window_choose_free(struct window_pane *);
void	window_choose_resize(struct window_pane *, u_int, u_int);
void	window_choose_key(struct window_pane *, struct session *, int);
void	window_choose_mouse(
	    struct window_pane *, struct session *, struct mouse_event *);

void	window_choose_fire_callback(
	    struct window_pane *, struct window_choose_data *);
void	window_choose_redraw_screen(struct window_pane *);
void	window_choose_write_line(
	    struct window_pane *, struct screen_write_ctx *, u_int);

void	window_choose_scroll_up(struct window_pane *);
void	window_choose_scroll_down(struct window_pane *);

enum window_choose_input_type {
	WINDOW_CHOOSE_NORMAL = -1,
	WINDOW_CHOOSE_GOTO_ITEM,
};

const struct window_mode window_choose_mode = {
	window_choose_init,
	window_choose_free,
	window_choose_resize,
	window_choose_key,
	window_choose_mouse,
	NULL,
};

struct window_choose_mode_data {
	struct screen	        screen;

	struct mode_key_data	mdata;

	ARRAY_DECL(, struct window_choose_mode_item) list;
	int			width;
	u_int			top;
	u_int			selected;
	enum window_choose_input_type input_type;
	const char		*input_prompt;
	char			*input_str;

	void 			(*callbackfn)(struct window_choose_data *);
	void			(*freefn)(struct window_choose_data *);
};

int	window_choose_index_key(int);
void	window_choose_prompt_input(enum window_choose_input_type,
	    const char *, struct window_pane *, int);

void
window_choose_add(struct window_pane *wp, struct window_choose_data *wcd)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct window_choose_mode_item	*item;
	char				 tmp[10];

	ARRAY_EXPAND(&data->list, 1);
	item = &ARRAY_LAST(&data->list);

	item->name = format_expand(wcd->ft, wcd->ft_template);
	item->wcd = wcd;
	item->pos = ARRAY_LENGTH(&data->list) - 1;

	data->width = snprintf (tmp, sizeof tmp , "%u", item->pos);
}

void
window_choose_ready(struct window_pane *wp, u_int cur,
    void (*callbackfn)(struct window_choose_data *),
    void (*freefn)(struct window_choose_data *))
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen			*s = &data->screen;

	data->selected = cur;
	if (data->selected > screen_size_y(s) - 1)
		data->top = ARRAY_LENGTH(&data->list) - screen_size_y(s);

	data->callbackfn = callbackfn;
	data->freefn = freefn;

	window_choose_redraw_screen(wp);
}

struct screen *
window_choose_init(struct window_pane *wp)
{
	struct window_choose_mode_data	*data;
	struct screen			*s;
	int				 keys;

	wp->modedata = data = xmalloc(sizeof *data);

	data->callbackfn = NULL;
	data->freefn = NULL;
	data->input_type = WINDOW_CHOOSE_NORMAL;
	data->input_str = xstrdup("");
	data->input_prompt = NULL;

	ARRAY_INIT(&data->list);
	data->top = 0;

	s = &data->screen;
	screen_init(s, screen_size_x(&wp->base), screen_size_y(&wp->base), 0);
	s->mode &= ~MODE_CURSOR;
	if (options_get_number(&wp->window->options, "mode-mouse"))
		s->mode |= MODE_MOUSE_STANDARD;

	keys = options_get_number(&wp->window->options, "mode-keys");
	if (keys == MODEKEY_EMACS)
		mode_key_init(&data->mdata, &mode_key_tree_emacs_choice);
	else
		mode_key_init(&data->mdata, &mode_key_tree_vi_choice);

	return (s);
}

struct window_choose_data *
window_choose_data_create(struct cmd_ctx *ctx)
{
	struct window_choose_data	*wcd;

	wcd = xmalloc(sizeof *wcd);
	wcd->ft = format_create();
	wcd->ft_template = NULL;
	wcd->command = NULL;
	wcd->client = ctx->curclient;
	wcd->session = ctx->curclient->session;
	wcd->idx = -1;

	return (wcd);
}

void
window_choose_free(struct window_pane *wp)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct window_choose_mode_item	*item;
	u_int				 i;

	for (i = 0; i < ARRAY_LENGTH(&data->list); i++) {
		item = &ARRAY_ITEM(&data->list, i);
		if (data->freefn != NULL && item->wcd != NULL)
			data->freefn(item->wcd);
		free(item->name);
	}
	ARRAY_FREE(&data->list);
	free(data->input_str);

	screen_free(&data->screen);
	free(data);
}

void
window_choose_resize(struct window_pane *wp, u_int sx, u_int sy)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen			*s = &data->screen;

	data->top = 0;
	if (data->selected > sy - 1)
		data->top = data->selected - (sy - 1);

	screen_resize(s, sx, sy);
	window_choose_redraw_screen(wp);
}

void
window_choose_fire_callback(
	struct window_pane *wp, struct window_choose_data *wcd)
{
	struct window_choose_mode_data	*data = wp->modedata;
	const struct window_mode	*oldmode;

	oldmode = wp->mode;
	wp->mode = NULL;

	data->callbackfn(wcd);

	wp->mode = oldmode;
}

void
window_choose_prompt_input(enum window_choose_input_type input_type,
    const char *prompt, struct window_pane *wp, int key)
{
	struct window_choose_mode_data	*data = wp->modedata;
	size_t				 input_len;

	data->input_type = input_type;
	data->input_prompt = prompt;
	input_len = strlen(data->input_str) + 2;

	data->input_str = xrealloc(data->input_str, 1, input_len);
	data->input_str[input_len - 2] = key;
	data->input_str[input_len - 1] = '\0';

	window_choose_redraw_screen(wp);
}

/* ARGSUSED */
void
window_choose_key(struct window_pane *wp, unused struct session *sess, int key)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen			*s = &data->screen;
	struct screen_write_ctx		 ctx;
	struct window_choose_mode_item	*item;
	size_t				 input_len;
	u_int		       		 items, n;
	int				 idx;

	items = ARRAY_LENGTH(&data->list);

	switch (mode_key_lookup(&data->mdata, key)) {
	case MODEKEYCHOICE_CANCEL:
		window_choose_fire_callback(wp, NULL);
		window_pane_reset_mode(wp);
		break;
	case MODEKEYCHOICE_CHOOSE:
		switch (data->input_type) {
		case WINDOW_CHOOSE_NORMAL:
			item = &ARRAY_ITEM(&data->list, data->selected);
			window_choose_fire_callback(wp, item->wcd);
			window_pane_reset_mode(wp);
			break;
		case WINDOW_CHOOSE_GOTO_ITEM:
			n = strtonum(data->input_str, 0, INT_MAX, NULL);
			if (n > items - 1)
				break;
			item = &ARRAY_ITEM(&data->list, n);
			window_choose_fire_callback(wp, item->wcd);
			window_pane_reset_mode(wp);
			break;
		}
		break;
	case MODEKEYCHOICE_UP:
		if (items == 0)
			break;
		if (data->selected == 0) {
			data->selected = items - 1;
			if (data->selected > screen_size_y(s) - 1)
				data->top = items - screen_size_y(s);
			window_choose_redraw_screen(wp);
			break;
		}
		data->selected--;
		if (data->selected < data->top)
			window_choose_scroll_up(wp);
		else {
			screen_write_start(&ctx, wp, NULL);
			window_choose_write_line(
			    wp, &ctx, data->selected - data->top);
			window_choose_write_line(
			    wp, &ctx, data->selected + 1 - data->top);
			screen_write_stop(&ctx);
		}
		break;
	case MODEKEYCHOICE_DOWN:
		if (items == 0)
			break;
		if (data->selected == items - 1) {
			data->selected = 0;
			data->top = 0;
			window_choose_redraw_screen(wp);
			break;
		}
		data->selected++;

		if (data->selected < data->top + screen_size_y(s)) {
			screen_write_start(&ctx, wp, NULL);
			window_choose_write_line(
			    wp, &ctx, data->selected - data->top);
			window_choose_write_line(
			    wp, &ctx, data->selected - 1 - data->top);
			screen_write_stop(&ctx);
		} else
			window_choose_scroll_down(wp);
		break;
	case MODEKEYCHOICE_SCROLLUP:
		if (items == 0 || data->top == 0)
			break;
		if (data->selected == data->top + screen_size_y(s) - 1) {
			data->selected--;
			window_choose_scroll_up(wp);
			screen_write_start(&ctx, wp, NULL);
			window_choose_write_line(
			    wp, &ctx, screen_size_y(s) - 1);
			screen_write_stop(&ctx);
		} else
			window_choose_scroll_up(wp);
		break;
	case MODEKEYCHOICE_SCROLLDOWN:
		if (items == 0 ||
		    data->top + screen_size_y(&data->screen) >= items)
			break;
		if (data->selected == data->top) {
			data->selected++;
			window_choose_scroll_down(wp);
			screen_write_start(&ctx, wp, NULL);
			window_choose_write_line(wp, &ctx, 0);
			screen_write_stop(&ctx);
		} else
			window_choose_scroll_down(wp);
		break;
	case MODEKEYCHOICE_PAGEUP:
		if (data->selected < screen_size_y(s)) {
			data->selected = 0;
			data->top = 0;
		} else {
			data->selected -= screen_size_y(s);
			if (data->top < screen_size_y(s))
				data->top = 0;
			else
				data->top -= screen_size_y(s);
		}
		window_choose_redraw_screen(wp);
		break;
	case MODEKEYCHOICE_PAGEDOWN:
		data->selected += screen_size_y(s);
		if (data->selected > items - 1)
			data->selected = items - 1;
		data->top += screen_size_y(s);
		if (screen_size_y(s) < items) {
			if (data->top + screen_size_y(s) > items)
				data->top = items - screen_size_y(s);
		} else
			data->top = 0;
		if (data->selected < data->top)
			data->top = data->selected;
		window_choose_redraw_screen(wp);
		break;
	case MODEKEYCHOICE_BACKSPACE:
		input_len = strlen(data->input_str);
		if (input_len > 0)
			data->input_str[input_len - 1] = '\0';
		window_choose_redraw_screen(wp);
		break;
	case MODEKEYCHOICE_STARTNUMBERPREFIX:
		if (key < '0' && key > '9')
			break;

		/*
		 * If there's less than ten items (0-9) then pressing a number
		 * will automatically select that item; otherwise, prompt for
		 * the item to go to.
		 */
		if (ARRAY_LENGTH(&data->list) - 1 <= 9) {
			idx = window_choose_index_key(key);
			if (idx < 0 || (u_int) idx >= ARRAY_LENGTH(&data->list))
				break;
			data->selected = idx;

			item = &ARRAY_ITEM(&data->list, data->selected);
			window_choose_fire_callback(wp, item->wcd);
			window_pane_reset_mode(wp);
		} else {
			window_choose_prompt_input(
			    WINDOW_CHOOSE_GOTO_ITEM, "Goto item", wp, key);
		}
		break;
	default:
		break;
	}
}

/* ARGSUSED */
void
window_choose_mouse(
    struct window_pane *wp, unused struct session *sess, struct mouse_event *m)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen			*s = &data->screen;
	struct window_choose_mode_item	*item;
	u_int				 idx;

	if ((m->b & 3) == 3)
		return;
	if (m->x >= screen_size_x(s))
		return;
	if (m->y >= screen_size_y(s))
		return;

	idx = data->top + m->y;
	if (idx >= ARRAY_LENGTH(&data->list))
		return;
	data->selected = idx;

	item = &ARRAY_ITEM(&data->list, data->selected);
	window_choose_fire_callback(wp, item->wcd);
	window_pane_reset_mode(wp);
}

void
window_choose_write_line(
    struct window_pane *wp, struct screen_write_ctx *ctx, u_int py)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct window_choose_mode_item	*item;
	struct options			*oo = &wp->window->options;
	struct screen			*s = &data->screen;
	struct grid_cell		 gc;
	size_t				 last, xoff = 0;
	char				 hdr[32];
	int				 utf8flag;

	if (data->callbackfn == NULL)
		fatalx("called before callback assigned");

	last = screen_size_y(s) - 1;
	utf8flag = options_get_number(&wp->window->options, "utf8");
	memcpy(&gc, &grid_default_cell, sizeof gc);
	if (data->selected == data->top + py)
		window_mode_attrs(&gc, oo);

	screen_write_cursormove(ctx, 0, py);
	if (data->top + py  < ARRAY_LENGTH(&data->list)) {
		item = &ARRAY_ITEM(&data->list, data->top + py);
		screen_write_nputs(ctx, screen_size_x(s) - 1,
		    &gc, utf8flag, "(%*d) %s", data->width,
		    item->pos, item->name);

	}
	while (s->cx < screen_size_x(s))
		screen_write_putc(ctx, &gc, ' ');

	if (data->input_type != WINDOW_CHOOSE_NORMAL) {
		window_mode_attrs(&gc, oo);

		xoff = xsnprintf(hdr, sizeof hdr,
			"%s: %s", data->input_prompt, data->input_str);
		screen_write_cursormove(ctx, 0, last);
		screen_write_puts(ctx, &gc, "%s", hdr);
		screen_write_cursormove(ctx, xoff, py);
		memcpy(&gc, &grid_default_cell, sizeof gc);
	}

}

int
window_choose_index_key(int key)
{
	static const char	keys[] = "0123456789";
	const char	       *ptr;
	u_int			idx = 0;

	for (ptr = keys; *ptr != '\0'; ptr++) {
		if (key == *ptr)
			return (idx);
		idx++;
	}
	return (-1);
}

void
window_choose_redraw_screen(struct window_pane *wp)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen			*s = &data->screen;
	struct screen_write_ctx	 	 ctx;
	u_int				 i;

	screen_write_start(&ctx, wp, NULL);
	for (i = 0; i < screen_size_y(s); i++)
		window_choose_write_line(wp, &ctx, i);
	screen_write_stop(&ctx);
}

void
window_choose_scroll_up(struct window_pane *wp)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen_write_ctx		 ctx;

	if (data->top == 0)
		return;
	data->top--;

	screen_write_start(&ctx, wp, NULL);
	screen_write_cursormove(&ctx, 0, 0);
	screen_write_insertline(&ctx, 1);
	window_choose_write_line(wp, &ctx, 0);
	if (screen_size_y(&data->screen) > 1)
		window_choose_write_line(wp, &ctx, 1);
	screen_write_stop(&ctx);
}

void
window_choose_scroll_down(struct window_pane *wp)
{
	struct window_choose_mode_data	*data = wp->modedata;
	struct screen			*s = &data->screen;
	struct screen_write_ctx		 ctx;

	if (data->top >= ARRAY_LENGTH(&data->list))
		return;
	data->top++;

	screen_write_start(&ctx, wp, NULL);
	screen_write_cursormove(&ctx, 0, 0);
	screen_write_deleteline(&ctx, 1);
	window_choose_write_line(wp, &ctx, screen_size_y(s) - 1);
	if (screen_size_y(&data->screen) > 1)
		window_choose_write_line(wp, &ctx, screen_size_y(s) - 2);
	screen_write_stop(&ctx);
}

void
window_choose_ctx(struct window_choose_data *cdata)
{
	struct cmd_ctx		 ctx;
	struct cmd_list		*cmdlist;
	char			*cause;

	/* The command template will have already been replaced.  But if it's
	 * NULL, bail here.
	 */
	if (cdata->command == NULL)
		return;

	if (cmd_string_parse(cdata->command, &cmdlist, &cause) != 0) {
		if (cause != NULL) {
			*cause = toupper((u_char) *cause);
			status_message_set(cdata->client, "%s", cause);
			free(cause);
		}
		return;
	}

	ctx.msgdata = NULL;
	ctx.curclient = cdata->client;

	ctx.error = key_bindings_error;
	ctx.print = key_bindings_print;
	ctx.info = key_bindings_info;

	ctx.cmdclient = NULL;

	cmd_list_exec(cmdlist, &ctx);
	cmd_list_free(cmdlist);
}

struct window_choose_data *
window_choose_add_session(struct window_pane *wp, struct cmd_ctx *ctx,
    struct session *s, const char *template, char *action, u_int idx)
{
	struct window_choose_data	*wcd;

	wcd = window_choose_data_create(ctx);
	wcd->idx = s->idx;
	wcd->command = cmd_template_replace(action, s->name, 1);
	wcd->ft_template = xstrdup(template);
	format_add(wcd->ft, "line", "%u", idx);
	format_session(wcd->ft, s);

	wcd->client->references++;
	wcd->session->references++;

	window_choose_add(wp, wcd);

	return (wcd);
}

struct window_choose_data *
window_choose_add_window(struct window_pane *wp, struct cmd_ctx *ctx,
    struct session *s, struct winlink *wl, const char *template,
    char *action, u_int idx)
{
	struct window_choose_data	*wcd;
	char				*action_data;

	wcd = window_choose_data_create(ctx);

	xasprintf(&action_data, "%s:%d", s->name, wl->idx);
	wcd->command = cmd_template_replace(action, action_data, 1);
	free(action_data);

	wcd->idx = wl->idx;
	wcd->ft_template = xstrdup(template);
	format_add(wcd->ft, "line", "%u", idx);
	format_session(wcd->ft, s);
	format_winlink(wcd->ft, s, wl);
	format_window_pane(wcd->ft, wl->window->active);

	wcd->client->references++;
	wcd->session->references++;

	window_choose_add(wp, wcd);

	return (wcd);
}
