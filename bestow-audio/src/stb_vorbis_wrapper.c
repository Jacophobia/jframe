// stb_vorbis_wrapper.c
// Compiles stb_vorbis as C to avoid C++ module header conflicts

// Tell stb_vorbis to only expose the header (declarations)
// The implementation is included here in this C file
#include "stb_vorbis.c"
