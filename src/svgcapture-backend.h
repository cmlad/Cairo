#ifndef __CAIRO_SVGCAPTURE_BACKEND_H__
#define __CAIRO_SVGCAPTURE_BACKEND_H__

#include "backend.h"

#if CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif

typedef struct {
	char *svg_buffer;
	unsigned int svg_buffer_size;
	unsigned int svg_content_length;
	int svg_full;
	char *png_buffer;
	unsigned int png_buffer_size;
} SVGOutputBuffer;

extern Rcairo_backend_def *RcairoBackendDef_svgcapture;
cairo_status_t svgcapture_capture_svg_output(void *closure, const unsigned char *data, unsigned int length);
SEXP svg_capture_capture_to_svg(Rcairo_backend *be);
int svg_capture_realloc_output_buffer_if_needed(SVGOutputBuffer *svg_output_buffer, unsigned int new_output_length);
void svg_capture_remove_surface(Rcairo_backend *be);
int svg_capture_reset_surface(Rcairo_backend *be);

Rcairo_backend *Rcairo_new_svgcapture_backend(Rcairo_backend *be, int conn, const char *filename, double width, double height);

#endif
