/*
 * lomo/lomo-em-art-backends.h
 *
 * Copyright (C) 2004-2011 Eina
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

#ifndef __LOMO_EM_ART_BACKENDS_H__
#define __LOMO_EM_ART_BACKENDS_H__

#include <lomo/lomo-em-art-backend.h>

void lomo_em_art_infolder_sync_backend_search(LomoEMArtBackend *backend,
	LomoEMArtSearch *search, gpointer data);
void lomo_em_art_embeded_metadata_backend_search(LomoEMArtBackend *backend,
	LomoEMArtSearch *search, gpointer data);

#endif /* __LOMO_EM_ART_BACKENDS_H__ */
