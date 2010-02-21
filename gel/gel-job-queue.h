#ifndef _GEL_JOB_QUEUE
#define _GEL_JOB_QUEUE

#include <glib-object.h>

G_BEGIN_DECLS

#define GEL_TYPE_JOB_QUEUE gel_job_queue_get_type()

#define GEL_JOB_QUEUE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEL_TYPE_JOB_QUEUE, GelJobQueue))

#define GEL_JOB_QUEUE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), GEL_TYPE_JOB_QUEUE, GelJobQueueClass))

#define GEL_IS_JOB_QUEUE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEL_TYPE_JOB_QUEUE))

#define GEL_IS_JOB_QUEUE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEL_TYPE_JOB_QUEUE))

#define GEL_JOB_QUEUE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEL_TYPE_JOB_QUEUE, GelJobQueueClass))

typedef struct {
	GObject parent;
} GelJobQueue;

typedef struct {
	GObjectClass parent_class;
} GelJobQueueClass;

GType gel_job_queue_get_type (void);

GelJobQueue*
gel_job_queue_new (void);

typedef void (*GelJobCallback) (GelJobQueue *queue, gpointer data);

void
gel_job_queue_push_job(GelJobQueue *self, GelJobCallback callback, GelJobCallback cancel, gpointer data); 

gboolean
gel_job_queue_run(GelJobQueue *self);

gboolean
gel_job_queue_next(GelJobQueue *self);

G_END_DECLS

#endif
