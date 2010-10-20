#include "e-test-backends.h"

void
eina_art_null_backend_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data)
{
	g_warning("Null backend here, failing!");
	eina_art_backend_finish(backend, search);
}

void
eina_art_null_random_search(EinaArtBackend *backend, EinaArtSearch *search, gpointer data)
{
	gboolean s = g_random_boolean();
	g_warning("Random backend here: %s!", s ? "success" : "faling");

	if (s)
		eina_art_search_set_result(search, (gpointer) g_strdup("0xdeadbeef"));

	eina_art_backend_finish(backend, search);
}

