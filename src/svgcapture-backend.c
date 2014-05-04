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

#if CAIRO_HAS_SVG_SURFACE

#define MAX_SVG_BUF_SIZE      8*1024*1024 // 8MB max buffer size (use only powers of 2)
#define INITIAL_SVG_BUF_SIZE  32*1024 // start with 32kb buffer

cairo_status_t svgcapture_capture_svg_output(void *closure, const unsigned char *data, unsigned int length) {
	Rcairo_backend *be = (Rcairo_backend *)closure;
	SVGOutputBuffer *svg_output_buffer;
	svg_output_buffer = (SVGOutputBuffer *) be->backendSpecific;

	// Add content only if the buffer is large enough
	if(svg_capture_realloc_output_buffer_if_needed(svg_output_buffer, length)) {

		// Add new output
		strncpy(svg_output_buffer->svg_buffer + svg_output_buffer->svg_content_length, data, length);

		// Update content length
		svg_output_buffer->svg_content_length += length;

		// Add NULL byte
		svg_output_buffer->svg_buffer[svg_output_buffer->svg_content_length] = '\0';
		
	}

	// Always return a success
	return CAIRO_STATUS_SUCCESS;
}

cairo_status_t svgcapture_capture_png_output(void *closure, const unsigned char *data, unsigned int length) {
	SVGOutputBuffer *svg_output_buffer = (SVGOutputBuffer *)closure;

	if (svg_output_buffer->png_buffer_size) {

		int new_size = svg_output_buffer->png_buffer_size + length;

		// Expand buffer
		char *new_buffer;
		if(!(new_buffer = (char *) realloc(svg_output_buffer->png_buffer, new_size))) {
			return CAIRO_STATUS_NO_MEMORY;
		}

		// Add new output
		memcpy(new_buffer + svg_output_buffer->png_buffer_size, data, length);

		// Switch buffers
		svg_output_buffer->png_buffer = new_buffer;

		// Update buffer size
		svg_output_buffer->png_buffer_size = new_size;
	}
	else {
		// Allocate new buffer
		svg_output_buffer->png_buffer = (char *) malloc(length);
		memcpy(svg_output_buffer->png_buffer, data, length);

		svg_output_buffer->png_buffer_size = length;
	}

	return CAIRO_STATUS_SUCCESS;
}

void svg_capture_clear_output(SVGOutputBuffer *svg_output_buffer) {
	svg_output_buffer->svg_content_length = 0;
	svg_output_buffer->svg_buffer[0] = '\0';
}

SEXP svg_capture_capture_to_svg(Rcairo_backend *be) {
	SVGOutputBuffer *svg_output_buffer;
	svg_output_buffer = (SVGOutputBuffer *) be->backendSpecific;

	svg_capture_clear_output(svg_output_buffer);

	double w = be->width;
	double h = be->height;

	cairo_surface_t *news = cairo_svg_surface_create_for_stream(svgcapture_capture_svg_output, be, w, h);
	cairo_t *newcr = cairo_create(news);
	cairo_set_source_surface (newcr, be->cs, 0.0, 0.0);
	cairo_paint (newcr);
	cairo_surface_destroy(news);
	cairo_destroy (newcr);

	if (!svg_output_buffer->svg_full) {
		SEXP output = mkString(svg_output_buffer->svg_buffer);
		
		return output;
	}
	else {
		// Reset full status
		svg_output_buffer->svg_full = 0;

		// Print to a PNG surface
		cairo_surface_t *news = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
		cairo_t *newcr = cairo_create(news);
		cairo_set_source_surface (newcr, be->cs, 0.0, 0.0);
		cairo_paint (newcr);

		cairo_surface_write_to_png_stream(news, svgcapture_capture_png_output, svg_output_buffer);
		int len = svg_output_buffer->png_buffer_size;

		SEXP output = PROTECT(allocVector(RAWSXP, len));
		unsigned char *output_data = RAW(output);
		
		// Copy data to the R object
		memcpy(output_data, svg_output_buffer->png_buffer, len);

		// Free the PNG buffer
		svg_output_buffer->png_buffer_size = 0;
		free(svg_output_buffer->png_buffer);
		svg_output_buffer->png_buffer = NULL;

		cairo_surface_destroy(news);
		cairo_destroy (newcr);

		printf("DONEEEE");
		UNPROTECT(1);
		return output;



		// Create a new surface with a graphical error
		/*svg_capture_reset_surface(be);
		cairo_t *cc = be->cc;
		cairo_set_source_rgba(cc, 0.8, 0.8, 0.8, 1);
		cairo_rectangle(cc, 0, 0, w, h);
		cairo_fill(cc);

		cairo_set_source_rgba(cc, 0, 0, 0, 1);
		cairo_set_font_size(cc, 16);
		cairo_move_to(cc, 20, 35);
		cairo_show_text(cc, "This graphic contains too many elements");
		cairo_move_to(cc, 20, 56);
		cairo_show_text(cc, "to be handled by StatAce.");

		cairo_set_font_size(cc, 13);
		cairo_move_to(cc, 20, 85);
		cairo_show_text(cc, "Please regenerate and save to an image with png(),");
		cairo_move_to(cc, 20, 103);
		cairo_show_text(cc, "jpeg(), bmp(), tiff() or the Cairo package.");*/

		

		//return mkString("Test"); //svg_capture_capture_to_svg(be);
	}
}

int svg_capture_realloc_output_buffer_if_needed(SVGOutputBuffer *svg_output_buffer, unsigned int new_output_length) {

	// Determine the new total length (including NULL byte)
	int new_len = svg_output_buffer->svg_content_length + new_output_length + 1;

	if (new_len <= svg_output_buffer->svg_buffer_size) {
		return 1;
	}

	// Check if the new length is above the max
	if (new_len > MAX_SVG_BUF_SIZE) {
		// With the new output the buffer will be above its max -> set as full without expanding
		svg_output_buffer->svg_full = 1;
		return 0;
	}

	// Double the size until we can fit the new length of the output
	int new_size = svg_output_buffer->svg_buffer_size;
	while (new_size < new_len) {
		new_size *= 2;
	}

	// Reallocate the buffer with the new size
	char *new_buffer;
	if(!(new_buffer = (char *) realloc(svg_output_buffer->svg_buffer, new_size * (sizeof(char))))) {
		// Error reallocating -> treat as full
		svg_output_buffer->svg_full = 1;
		return 0;
	}

	// Set the new buffer and update the size in the struct
	svg_output_buffer->svg_buffer = new_buffer;
	svg_output_buffer->svg_buffer_size = new_size;

	return 1;

}

static char *types_list[] = { "svgcapture", 0 };
static Rcairo_backend_def RcairoBackendDef_ = {
  BET_SVGCAPTURE,
  types_list,
  "SVG Capture",
  CBDF_FILE|CBDF_CONN|CBDF_MULTIPAGE, /* we can't really do multi-page, but we can't do multifiles, either */
  0
};
Rcairo_backend_def *RcairoBackendDef_svgcapture = &RcairoBackendDef_;

static void svgcapture_save_page(Rcairo_backend* be, int pageno) {
}

static void svgcapture_backend_destroy(Rcairo_backend* be)
{
	if (be->backendSpecific) {
		free(be->backendSpecific);

	}
	svg_capture_remove_surface(be);
	free(be);
}

void svg_capture_remove_surface(Rcairo_backend *be) {
	cairo_surface_destroy(be->cs);
	cairo_destroy(be->cc);
}

int svg_capture_reset_surface(Rcairo_backend *be) {
	if (be->cs) {
		svg_capture_remove_surface(be);
	}

	be->cs = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);	

	if (cairo_surface_status(be->cs) != CAIRO_STATUS_SUCCESS){
		free(be);
		return 0;
	}

	be->cc = cairo_create(be->cs);

	if (cairo_status(be->cc) != CAIRO_STATUS_SUCCESS){
		free(be);
		return 0;
	}

	cairo_set_operator(be->cc,CAIRO_OPERATOR_OVER);

	return 1;
}

Rcairo_backend *Rcairo_new_svgcapture_backend(Rcairo_backend *be, int conn, const char *filename, double width, double height)
{
	be->backend_type = BET_SVGCAPTURE;
	be->destroy_backend = svgcapture_backend_destroy;
	be->save_page = svgcapture_save_page;

	be->width = width;
	be->height = height;

	// Create and allocate buffer struct
	SVGOutputBuffer *svg_output_buffer;
	if (!(svg_output_buffer = (SVGOutputBuffer *) malloc(sizeof(SVGOutputBuffer)))) {
		return NULL;
	}

	// Set initial values
	svg_output_buffer->svg_buffer_size = INITIAL_SVG_BUF_SIZE;
	svg_output_buffer->svg_full = 0;
	svg_output_buffer->png_buffer_size = 0;

	// Create and allocate buffer
	if(!(svg_output_buffer->svg_buffer = (char *) malloc(svg_output_buffer->svg_buffer_size))) {
		return NULL;
	}
	svg_capture_clear_output(svg_output_buffer);

	be->backendSpecific = svg_output_buffer;

	if (!svg_capture_reset_surface(be)) {
		return NULL;
	}

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
