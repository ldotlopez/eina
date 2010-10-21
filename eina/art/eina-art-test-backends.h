/*
 * eina/art/eina-art-test-backends.h
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

#ifndef _EINA_ART_TEST_BACKENDS
#define _EINA_ART_TEST_BACKENDS

#include <eina/art/eina-art-backend.h>

void
eina_art_null_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data);
void
eina_art_random_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data);
void
eina_art_infolder_sync_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data);

#endif
