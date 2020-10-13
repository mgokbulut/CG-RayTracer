// The makers of stb are scared of build systems so they share a header file with both forward declares
// and the implementation (without inline keyword because it needs to be a C library). We use this
// compilation unit to compile the implementation so that the other compilation units can refer to it
// without multiple definitions errors.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
