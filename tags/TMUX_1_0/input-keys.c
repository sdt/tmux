/* $Id: input-keys.c,v 1.29 2009-07-28 22:37:02 tcunha Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

struct input_key_ent {
	int		 key;
	const char	*data;

	int		 flags;
#define INPUTKEY_KEYPAD 0x1	/* keypad key */
#define INPUTKEY_CURSOR 0x2	/* cursor key */
#define INPUTKEY_CTRL 0x4	/* may be modified with ctrl */
#define INPUTKEY_XTERM 0x4	/* may have xterm argument appended */
};

struct input_key_ent input_keys[] = {
	/* Backspace key. */
	{ KEYC_BSPACE, "\177",	   0 },

	/* Function keys. */
	{ KEYC_F1,     "\033OP",   INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F2,     "\033OQ",   INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F3,     "\033OR",   INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F4,     "\033OS",   INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F5,     "\033[15~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F6,     "\033[17~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F7,     "\033[18~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F8,     "\033[19~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F9,     "\033[20~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F10,    "\033[21~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F11,    "\033[23~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F12,    "\033[24~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F13,    "\033[25~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F14,    "\033[26~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F15,    "\033[28~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F16,    "\033[29~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F17,    "\033[31~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F18,    "\033[32~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F19,    "\033[33~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_F20,    "\033[34~", INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_IC,     "\033[2~",  INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_DC,     "\033[3~",  INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_HOME,   "\033[1~",  INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_END,    "\033[4~",  INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_NPAGE,  "\033[6~",  INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_PPAGE,  "\033[5~",  INPUTKEY_CTRL|INPUTKEY_XTERM },
	{ KEYC_BTAB,   "\033[Z",   INPUTKEY_CTRL },

	/* Arrow keys. Cursor versions must come first. */
	{ KEYC_UP | KEYC_CTRL,     "\033Oa", 0 },
	{ KEYC_DOWN | KEYC_CTRL,   "\033Ob", 0 },
	{ KEYC_RIGHT | KEYC_CTRL,  "\033Oc", 0 },
	{ KEYC_LEFT | KEYC_CTRL,   "\033Od", 0 },
	
	{ KEYC_UP | KEYC_SHIFT,    "\033[a", 0 },
	{ KEYC_DOWN | KEYC_SHIFT,  "\033[b", 0 },
	{ KEYC_RIGHT | KEYC_SHIFT, "\033[c", 0 },
	{ KEYC_LEFT | KEYC_SHIFT,  "\033[d", 0 },
	
	{ KEYC_UP,     "\033OA",   INPUTKEY_CURSOR },
	{ KEYC_DOWN,   "\033OB",   INPUTKEY_CURSOR },
	{ KEYC_RIGHT,  "\033OC",   INPUTKEY_CURSOR },
	{ KEYC_LEFT,   "\033OD",   INPUTKEY_CURSOR },

	{ KEYC_UP,     "\033[A",   0 },
	{ KEYC_DOWN,   "\033[B",   0 },
	{ KEYC_RIGHT,  "\033[C",   0 },
	{ KEYC_LEFT,   "\033[D",   0 },

	/* Keypad keys. Keypad versions must come first. */
	{ KEYC_KP0_1,  "/", INPUTKEY_KEYPAD },
	{ KEYC_KP0_2,  "*", INPUTKEY_KEYPAD },
	{ KEYC_KP0_3,  "-", INPUTKEY_KEYPAD },
	{ KEYC_KP1_0,  "7", INPUTKEY_KEYPAD },
	{ KEYC_KP1_1,  "8", INPUTKEY_KEYPAD },
	{ KEYC_KP1_2,  "9", INPUTKEY_KEYPAD },
	{ KEYC_KP1_3,  "+", INPUTKEY_KEYPAD },
	{ KEYC_KP2_0,  "4", INPUTKEY_KEYPAD },
	{ KEYC_KP2_1,  "5", INPUTKEY_KEYPAD },
	{ KEYC_KP2_2,  "6", INPUTKEY_KEYPAD },
	{ KEYC_KP3_0,  "1", INPUTKEY_KEYPAD },
	{ KEYC_KP3_1,  "2", INPUTKEY_KEYPAD },
	{ KEYC_KP3_2,  "3", INPUTKEY_KEYPAD },
	{ KEYC_KP3_3,  "\n", INPUTKEY_KEYPAD }, /* this can be CRLF too? */
	{ KEYC_KP4_0,  "0", INPUTKEY_KEYPAD },
	{ KEYC_KP4_2,  ".", INPUTKEY_KEYPAD },
	{ KEYC_KP0_1,  "\033Oo", 0 },
	{ KEYC_KP0_2,  "\033Oj", 0 },
	{ KEYC_KP0_3,  "\033Om", 0 },
	{ KEYC_KP1_0,  "\033Ow", 0 },
	{ KEYC_KP1_1,  "\033Ox", 0 },
	{ KEYC_KP1_2,  "\033Oy", 0 },
	{ KEYC_KP1_3,  "\033Ok", 0 },
	{ KEYC_KP2_0,  "\033Ot", 0 },
	{ KEYC_KP2_1,  "\033Ou", 0 },
	{ KEYC_KP2_2,  "\033Ov", 0 },
	{ KEYC_KP3_0,  "\033Oq", 0 },
	{ KEYC_KP3_1,  "\033Or", 0 },
	{ KEYC_KP3_2,  "\033Os", 0 },
	{ KEYC_KP3_3,  "\033OM", 0 },
	{ KEYC_KP4_0,  "\033Op", 0 },
	{ KEYC_KP4_2,  "\033On", 0 },
};

/* Translate a key code from client into an output key sequence. */
void
input_key(struct window_pane *wp, int key)
{
	struct input_key_ent   *ike;
	u_int			i;
	char			ch;
	size_t			dlen;
	int			xterm_keys;

	log_debug2("writing key 0x%x", key);

	if (key != KEYC_NONE && (key & ~KEYC_ESCAPE) < 0x100) {
		if (key & KEYC_ESCAPE)
			buffer_write8(wp->out, '\033');
		buffer_write8(wp->out, (uint8_t) (key & ~KEYC_ESCAPE));
		return;
	}

	for (i = 0; i < nitems(input_keys); i++) {
		ike = &input_keys[i];

		if ((ike->flags & INPUTKEY_KEYPAD) &&
		    !(wp->screen->mode & MODE_KKEYPAD))
			continue;
		if ((ike->flags & INPUTKEY_CURSOR) &&
		    !(wp->screen->mode & MODE_KCURSOR))
			continue;

		if ((key & KEYC_ESCAPE) && (ike->key | KEYC_ESCAPE) == key)
			break;
		if ((key & KEYC_SHIFT) && (ike->key | KEYC_SHIFT) == key)
			break;
		if ((key & KEYC_CTRL) && (ike->key | KEYC_CTRL) == key) {
			if (ike->flags & INPUTKEY_CTRL)
				break;
		}
		if (ike->key == key)
			break;
	}
	if (i == nitems(input_keys)) {
		log_debug2("key 0x%x missing", key);
		return;
	}
	dlen = strlen(ike->data);

	log_debug2("found key 0x%x: \"%s\"", key, ike->data);

	/*
	 * If in xterm keys mode, work out and append the modifier as an
	 * argument.
	 */
	xterm_keys = options_get_number(&wp->window->options, "xterm-keys");
	if (xterm_keys && ike->flags & INPUTKEY_XTERM) {
		ch = '\0';
		if (key & (KEYC_SHIFT|KEYC_ESCAPE|KEYC_CTRL))
			ch = '8';
		else if (key & (KEYC_ESCAPE|KEYC_CTRL))
			ch = '7';
		else if (key & (KEYC_SHIFT|KEYC_CTRL))
			ch = '6';
		else if (key & KEYC_CTRL)
			ch = '5';
		else if (key & (KEYC_SHIFT|KEYC_ESCAPE))
			ch = '4';
		else if (key & KEYC_ESCAPE)
			ch = '3';
		else if (key & KEYC_SHIFT)
			ch = '2';
		if (ch != '\0') {
			buffer_write(wp->out, ike->data, dlen - 1);
			buffer_write8(wp->out, ';');
			buffer_write8(wp->out, ch);
			buffer_write8(wp->out, ike->data[dlen - 1]);
		} else
			buffer_write(wp->out, ike->data, dlen);
		return;
	}

	/*
	 * Not in xterm mode. Prefix a \033 for escape, and set bit 5 of the
	 * last byte for ctrl.
	 */
	if (key & KEYC_ESCAPE)
		buffer_write8(wp->out, '\033');
	if (key & KEYC_CTRL && ike->flags & INPUTKEY_CTRL) {
		buffer_write(wp->out, ike->data, dlen - 1);
		buffer_write8(wp->out, ike->data[dlen - 1] ^ 0x20);
		return;
	}
	buffer_write(wp->out, ike->data, dlen);
}

/* Handle input mouse. */
void
input_mouse(struct window_pane *wp, u_char b, u_char x, u_char y)
{
	if (wp->screen->mode & MODE_MOUSE) {
		buffer_write(wp->out, "\033[M", 3);
		buffer_write8(wp->out, b + 32);
		buffer_write8(wp->out, x + 33);
		buffer_write8(wp->out, y + 33);
	}
}
