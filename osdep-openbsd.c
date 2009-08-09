/* $Id: osdep-openbsd.c,v 1.18 2009/08/09 16:08:12 tcunha Exp $ */

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

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef nitems
#define nitems(_a) (sizeof((_a)) / sizeof((_a)[0]))
#endif

#define is_runnable(p) \
	((p)->p_stat == SRUN || (p)->p_stat == SIDL || (p)->p_stat == SONPROC)
#define is_stopped(p) \
	((p)->p_stat == SSTOP || (p)->p_stat == SZOMB || (p)->p_stat == SDEAD)

struct proc	*cmp_procs(struct proc *, struct proc *);
char		*osdep_get_name(int, char *);

struct proc *
cmp_procs(struct proc *p1, struct proc *p2)
{
	void	*ptr1, *ptr2;

	if (is_runnable(p1) && !is_runnable(p2))
		return (p1);
	if (!is_runnable(p1) && is_runnable(p2))
		return (p2);

	if (is_stopped(p1) && !is_stopped(p2))
		return (p1);
	if (!is_stopped(p1) && is_stopped(p2))
		return (p2);

	if (p1->p_estcpu > p2->p_estcpu)
		return (p1);
	if (p1->p_estcpu < p2->p_estcpu)
		return (p2);

	if (p1->p_slptime < p2->p_slptime)
		return (p1);
	if (p1->p_slptime > p2->p_slptime)
		return (p2);

	if ((p1->p_flag & P_SINTR) && !(p2->p_flag & P_SINTR))
		return (p1);
	if (!(p1->p_flag & P_SINTR) && (p2->p_flag & P_SINTR))
		return (p2);

	ptr1 = LIST_FIRST(&p1->p_children);
	ptr2 = LIST_FIRST(&p2->p_children);
	if (ptr1 == NULL && ptr2 != NULL)
		return (p1);
	if (ptr1 != NULL && ptr2 == NULL)
		return (p2);

	if (strcmp(p1->p_comm, p2->p_comm) < 0)
		return (p1);
	if (strcmp(p1->p_comm, p2->p_comm) > 0)
		return (p2);

	if (p1->p_pid > p2->p_pid)
		return (p1);
	return (p2);
}

char *
osdep_get_name(int fd, char *tty)
{
	int		 mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PGRP, 0 };
	struct stat	 sb;
	size_t		 len;
	struct kinfo_proc *buf, *newbuf;
	struct proc	*p, *bestp;
	u_int		 i;
	char		*name;

	buf = NULL;

	if (stat(tty, &sb) == -1)
		return (NULL);
	if ((mib[3] = tcgetpgrp(fd)) == -1)
		return (NULL);

retry:
	if (sysctl(mib, nitems(mib), NULL, &len, NULL, 0) == -1)
		return (NULL);
	len = (len * 5) / 4;

	if ((newbuf = realloc(buf, len)) == NULL)
		goto error;
	buf = newbuf;

	if (sysctl(mib, nitems(mib), buf, &len, NULL, 0) == -1) {
		if (errno == ENOMEM)
			goto retry;
		goto error;
	}

	bestp = NULL;
	for (i = 0; i < len / sizeof (struct kinfo_proc); i++) {
		if (buf[i].kp_eproc.e_tdev != sb.st_rdev)
			continue;
		p = &buf[i].kp_proc;
		if (bestp == NULL)
			bestp = &buf[i].kp_proc;
		else
			bestp = cmp_procs(&buf[i].kp_proc, bestp);
	}

	name = NULL;
	if (bestp != NULL)
		name = strdup(bestp->p_comm);

	free(buf);
	return (name);

error:
	free(buf);
	return (NULL);
}