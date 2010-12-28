/*
 * plugins/fieshta/fieshta-stage.h
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

#ifndef _FIESTA_STAGE
#define _FIESTA_STAGE

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define FIESTA_STAGE_TYPE_STAGE fieshta_stage_get_type()

#define FIESTA_STAGE_STAGE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), FIESTA_STAGE_TYPE_STAGE, FieshtaStage))

#define FIESTA_STAGE_STAGE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), FIESTA_STAGE_TYPE_STAGE, FieshtaStageClass))

#define FIESTA_STAGE_IS_STAGE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), FIESTA_STAGE_TYPE_STAGE))

#define FIESTA_STAGE_IS_STAGE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), FIESTA_STAGE_TYPE_STAGE))

#define FIESTA_STAGE_STAGE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), FIESTA_STAGE_TYPE_STAGE, FieshtaStageClass))

typedef struct {
	ClutterGroup parent;
} FieshtaStage;

typedef struct {
	ClutterGroupClass parent_class;
} FieshtaStageClass;

GType fieshta_stage_get_type (void);

FieshtaStage* fieshta_stage_new (void);

void fieshta_stage_set_slots(FieshtaStage* self, guint slots);
ClutterActor *
fieshta_stage_get_nth(FieshtaStage* self, guint slot);
void
fieshta_stage_push(FieshtaStage* self, ClutterActor *actor);

G_END_DECLS

#endif /* _FIESTA_STAGE */
