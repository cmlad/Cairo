#include <stdlib.h>
#include <string.h>
#include "svgcapture-backend.h"
#ifdef HAVE_RCONN_H
#include <R.h>
#include <Rinternals.h>
#include <Rdefines.h>
#define R_INTERFACE_PTRS
#include <Rinterface.h>
#include <Rembedded.h>
#include <R_ext/Print.h>
#include <R_ext/RConn.h>
#endif

char *svg_output;
cairo_status_t svgcapture_capture(void *closure, const unsigned char *data, unsigned int length)
{
	int start_from = 0;
	if (svg_output) {
		start_from = strlen(svg_output) - 1;
		char *new_svg_output = (char *) malloc(length + start_from + 1);
		strcpy(new_svg_output, svg_output);
		free(svg_output);
		svg_output = new_svg_output;
	}
	else {
		svg_output = (char *) malloc(length + 1);
	}
	
	strncpy(svg_output + start_from, data, length);
	svg_output[length + start_from] = '\0';
	return CAIRO_STATUS_SUCCESS;
}

#if CAIRO_HAS_SVG_SURFACE


static char *types_list[] = { "svgcapture", 0 };
static Rcairo_backend_def RcairoBackendDef_ = {
  BET_SVGCAPTURE,
  types_list,
  "SVG Capture",
  CBDF_FILE|CBDF_CONN|CBDF_MULTIPAGE, /* we can't really do multi-page, but we can't do multifiles, either */
  0
};
Rcairo_backend_def *RcairoBackendDef_svgcapture = &RcairoBackendDef_;

static void svgcapture_save_page(Rcairo_backend* be, int pageno){
	//cairo_show_page(be->cc);
}

static void svgcapture_backend_destroy(Rcairo_backend* be)
{
	cairo_surface_destroy(be->cs);
	cairo_destroy(be->cc);
	free(be);
}

Rcairo_backend *Rcairo_new_svgcapture_backend(Rcairo_backend *be, int conn, const char *filename, double width, double height)
{
	be->backend_type = BET_SVGCAPTURE;
	be->destroy_backend = svgcapture_backend_destroy;
	be->save_page = svgcapture_save_page;

	be->width = width;
	be->height = height;

	be->cs = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);

	if (cairo_surface_status(be->cs) != CAIRO_STATUS_SUCCESS){
		free(be);
		return NULL;
	}

	be->cc = cairo_create(be->cs);

	if (cairo_status(be->cc) != CAIRO_STATUS_SUCCESS){
		free(be);
		return NULL;
	}

	cairo_set_operator(be->cc,CAIRO_OPERATOR_OVER);

	return be;
}
#else
Rcairo_backend_def *RcairoBackendDef_svgcapture = 0;

Rcairo_backend *Rcairo_new_svgcapture_backend(Rcairo_backend *be, int conn, const char *filename, double width, double height)
{
	error("cairo library was compiled without SVG support.");
	return NULL;
}
#endif
