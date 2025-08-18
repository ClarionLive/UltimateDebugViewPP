// Stub unistd.h for Windows compatibility
#ifndef _UNISTD_H
#define _UNISTD_H

#ifdef _WIN32
#include <io.h>
#include <process.h>

#define isatty _isatty
#define fileno _fileno

#endif

#endif