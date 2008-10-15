/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2008 Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gstgoosem.h>

/**
 * gst_goo_sem_new:
 * @counter: the semaphore's counter initial value
 *
 * Creates a new semaphore for threading syncronization
 *
 * Returns: a new #GstGooSem structure
 */
GstGooSem*
gst_goo_sem_new (gint counter)
{
        g_assert (counter <= 0);
        GstGooSem *self;

        self = g_new (GstGooSem, 1);
        g_assert (self != NULL);

        self->counter = counter;

        self->condition = g_cond_new ();
        self->mutex = g_mutex_new ();

        return self;
}

/**
 * gst_goo_sem_free:
 * @self: an #GstGooSem structure
 *
 * Free the structure
 */
void
gst_goo_sem_free (GstGooSem *self)
{
        g_assert (self != NULL);

        g_cond_free (self->condition);
        g_mutex_free (self->mutex);
        g_free (self);

        return;
}

/**
 * gst_goo_sem_down:
 * @self: an #GstGooSem structure
 *
 * While the counter is 0, the process will be stopped, waiting for
 * an up signal
 */
void
gst_goo_sem_down (GstGooSem *self)
{
        g_assert (self != NULL);

        g_mutex_lock (self->mutex);

        while (self->counter == 0)
        {
		g_cond_wait (self->condition, self->mutex);
        }

        self->counter--;

        g_mutex_unlock (self->mutex);

        return;
}

/**
 * gst_goo_sem_up:
 * @self: an #GstGooSem structure
 *
 * Increments the semaphore's counter and signals the condition for thread
 * execution continuation.
 */
void
gst_goo_sem_up (GstGooSem *self)
{
        g_assert (self != NULL);

        g_mutex_lock (self->mutex);
        self->counter++;
        g_cond_signal (self->condition);
        g_mutex_unlock (self->mutex);

        return;
}
