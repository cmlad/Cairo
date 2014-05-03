#ifndef __CAIRO_SVGCAPTURE_BACKEND_H__
#define __CAIRO_SVGCAPTURE_BACKEND_H__

#include "backend.h"

#if CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif

extern Rcairo_backend_def *RcairoBackendDef_svgcapture;
extern char *svg_output;
cairo_status_t svgcapture_capture(void *closure, const unsigned char *data, unsigned int length);

Rcairo_backend *Rcairo_new_svgcapture_backend(Rcairo_backend *be, int conn, const char *filename, double width, double height);

#endif
