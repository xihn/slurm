/*****************************************************************************\
 *  events.h - Internal declarations for event handlers
 *****************************************************************************
 *  Copyright (C) SchedMD LLC.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

/*
 * Named event based signaling and waiting
 */

#ifndef _CONMGR_EVENTS_H
#define _CONMGR_EVENTS_H

#include <pthread.h>
#include <stdbool.h>

#include "src/common/macros.h"

/*
 * WARNING: Only use functions below to access/edit struct members
 */
typedef struct {
	const char *name; /* stringified event name */
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int pending; /* reliable signals pending */
	int waiting; /* # threads waiting for signal */
} event_signal_t;

#define EVENT_INITIALIZER(event_name) \
	{ \
		.mutex = PTHREAD_MUTEX_INITIALIZER, \
		.cond = PTHREAD_COND_INITIALIZER, \
		.pending = 0, \
		.waiting = 0, \
		.name = event_name, \
	}

#define EVENT_FREE_MEMBERS(event) \
do { \
	slurm_cond_destroy(&((event)->cond)); \
	slurm_mutex_destroy(&((event)->mutex)); \
} while (false)

/*
 * Wait (aka block) for a signal for a given event
 * NOTE: call EVENT_WAIT() instead
 * IN event - event ptr to wait for signal
 * IN caller - __func__ from caller
 */
extern void event_wait_now(event_signal_t *event, const char *caller);

/*
 * Wait (aka block) for a signal for a given event
 * Note: A thread currently blocked under EVENT_WAIT() is referred to as a
 *	waiter.
 * IN event- ptr to event to wait on
 */
#define EVENT_WAIT(event) event_wait_now(event, __func__)

/*
 * Send signal to a given event
 * NOTE: call EVENT_SIGNAL() or EVENT_BROADCAST() instead
 * IN reliable - ensure signal is received if nothing is waiting yet
 * IN singular - only send signal once. if another reliable signal has been
 *	sent, then ignore signal request.
 * IN broadcast - send signal to all waiters at once
 * IN event - ptr to event to signal
 * IN caller - __func__ from caller
 */
extern void event_signal_now(bool reliable, bool singular, bool broadcast,
			     event_signal_t *event, const char *caller);

/*
 * Send signal to one currently waiting thread or drop signal if there are no
 * currently waiting threads.
 */
#define EVENT_SIGNAL(event) \
	event_signal_now(false, false, false, event, __func__)
/* Send signal to one waiter even if EVENT_WAIT() called later */
#define EVENT_SIGNAL_RELIABLE(event) \
	event_signal_now(true, false, false, event, __func__)
/*
 * Send signal to one waiter even if EVENT_WAIT() called later but drop signal
 * if there is already another reliable signal pending a waiter.
 */
#define EVENT_SIGNAL_RELIABLE_SINGULAR(event) \
	event_signal_now(true, true, false, event, __func__)
/*
 * Send signal to all currently waiting threads or drop signal if there are no
 * currently waiting threads.
 */
#define EVENT_BROADCAST(event) \
	event_signal_now(false, false, true, event, __func__)

#endif /* _CONMGR_EVENTS_H */
