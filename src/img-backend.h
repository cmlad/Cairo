#ifndef __IMAGE_BACKEND_H__
#define __IMAGE_BACKEND_H__

#include "backend.h"

Rcairo_backend *Rcairo_new_image_backend(char *filename, char *type, int width, int height);

#endif
