/*
 * gel/gel-job-queue.c
 *
 * Copyright (C) 2004-2010 Eina
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gel-job-queue.h"
#include <glib/gi18n.h>
#include "gel-misc.h"

G_DEFINE_TYPE (GelJobQueue, gel_job_queue, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GEL_TYPE_JOB_QUEUE, GelJobQueuePrivate))

typedef struct _GelJobQueuePrivate GelJobQueuePrivate;

typedef struct {
	GelJobCallback callback, cancel;
	gpointer data;
} GelJob;

struct _GelJobQueuePrivate {
	GQueue *jobs;
	GelJob *current;
};

// GEL_AUTO_QUARK_FUNC(gel_job_queue)

static void
gel_job_queue_get_property (GObject *object, guint property_id,
	GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
gel_job_queue_set_property (GObject *object, guint property_id,
	const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
gel_job_queue_dispose (GObject *object)
{
	GelJobQueue *self = GEL_JOB_QUEUE(object);
	GelJobQueuePrivate *priv = GET_PRIVATE(self);

	if (priv->jobs && !g_queue_is_empty(priv->jobs))
	{
		g_queue_foreach(priv->jobs, (GFunc) g_free, NULL);
		g_queue_clear(priv->jobs);
	}

	if (priv->jobs)
	{
		g_queue_free(priv->jobs);
		priv->jobs = NULL;
	}

	G_OBJECT_CLASS (gel_job_queue_parent_class)->dispose (object);
}

static void
gel_job_queue_finalize (GObject *object)
{
	G_OBJECT_CLASS (gel_job_queue_parent_class)->finalize (object);
}

static void
gel_job_queue_class_init (GelJobQueueClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GelJobQueuePrivate));

	object_class->get_property = gel_job_queue_get_property;
	object_class->set_property = gel_job_queue_set_property;
	object_class->dispose = gel_job_queue_dispose;
	object_class->finalize = gel_job_queue_finalize;
}

static void
gel_job_queue_init (GelJobQueue *self)
{
	GelJobQueuePrivate *priv = GET_PRIVATE(self);
	priv->jobs = g_queue_new();
}

GelJobQueue*
gel_job_queue_new (void)
{
	return g_object_new (GEL_TYPE_JOB_QUEUE, NULL);
}

void
gel_job_queue_push_job(GelJobQueue *self, GelJobCallback callback, GelJobCallback cancel, gpointer data) 
{
	g_return_if_fail(GEL_IS_JOB_QUEUE(self));
	g_return_if_fail(callback != NULL);

	GelJobQueuePrivate *priv = GET_PRIVATE(self);

	GelJob *job = g_new0(GelJob, 1);
	job->callback = callback;
	job->cancel   = cancel;
	job->data     = data;

	g_queue_push_tail(priv->jobs, job);

	return;
}

gboolean
gel_job_queue_run(GelJobQueue *self)
{
	g_return_val_if_fail(GEL_IS_JOB_QUEUE(self), FALSE);

	GelJobQueuePrivate *priv = GET_PRIVATE(self);
	if (priv->current != NULL)
	{
		g_warning(N_("Application called gel_job_queue_run while a job is active. Application has to call gel_job_queue_cancel before, this IS a bug."));
		g_free(priv->current);
		priv->current = NULL;
	}

	if (g_queue_is_empty(priv->jobs))
		return FALSE;

	priv->current = g_queue_pop_head(priv->jobs);
	priv->current->callback(self, priv->current->data);

	return TRUE;
}

gboolean
gel_job_queue_next(GelJobQueue *self)
{
	g_return_val_if_fail(GEL_IS_JOB_QUEUE(self), FALSE);
	GelJobQueuePrivate *priv = GET_PRIVATE(self);

	// Free current job and NULLify it.
	g_return_val_if_fail(priv->current != NULL, FALSE);
	g_free(priv->current);
	priv->current = NULL;

	// Run next job
	return gel_job_queue_run(self);
}

void
gel_job_queue_stop(GelJobQueue *self)
{
}
