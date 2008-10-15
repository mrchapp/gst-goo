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

#ifndef __GST_GOO_SEM_H__
#define __GST_GOO_SEM_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GstGooSem GstGooSem;

/**
 * GstGooSem:
 *
 * A simple semaphore for threads syncronization
 */
struct _GstGooSem
{
        GCond *condition;
        GMutex *mutex;
        gint counter;
};

GstGooSem* gst_goo_sem_new (gint counter);
void gst_goo_sem_free (GstGooSem* self);
void gst_goo_sem_down (GstGooSem* self);
void gst_goo_sem_up   (GstGooSem* self);

G_END_DECLS

#endif /* __GST_GOO_SEM_H__ */
