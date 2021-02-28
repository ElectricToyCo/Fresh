// Modified by Jeff Wofford for minimal MinGW functionality 2013-07-28

#ifndef AL_ALUT_H
#define AL_ALUT_H

#include "AL/alc.h"
#include "AL/al.h"

#define ALUT_API_MAJOR_VERSION                 1
#define ALUT_API_MINOR_VERSION                 1

#define ALUT_ERROR_NO_ERROR                    0
#define ALUT_ERROR_OUT_OF_MEMORY               0x200
#define ALUT_ERROR_INVALID_ENUM                0x201
#define ALUT_ERROR_INVALID_VALUE               0x202
#define ALUT_ERROR_INVALID_OPERATION           0x203
#define ALUT_ERROR_NO_CURRENT_CONTEXT          0x204
#define ALUT_ERROR_AL_ERROR_ON_ENTRY           0x205
#define ALUT_ERROR_ALC_ERROR_ON_ENTRY          0x206
#define ALUT_ERROR_OPEN_DEVICE                 0x207
#define ALUT_ERROR_CLOSE_DEVICE                0x208
#define ALUT_ERROR_CREATE_CONTEXT              0x209
#define ALUT_ERROR_MAKE_CONTEXT_CURRENT        0x20A
#define ALUT_ERROR_DESTROY_CONTEXT             0x20B
#define ALUT_ERROR_GEN_BUFFERS                 0x20C
#define ALUT_ERROR_BUFFER_DATA                 0x20D
#define ALUT_ERROR_IO_ERROR                    0x20E
#define ALUT_ERROR_UNSUPPORTED_FILE_TYPE       0x20F
#define ALUT_ERROR_UNSUPPORTED_FILE_SUBTYPE    0x210
#define ALUT_ERROR_CORRUPT_OR_TRUNCATED_DATA   0x211

#define ALUT_LOADER_BUFFER                     0x300
#define ALUT_LOADER_MEMORY                     0x301

ALboolean alutInit (int *argcp, char **argv);
ALboolean alutInitWithoutContext (int *argcp, char **argv);
ALboolean alutExit (void);

ALenum alutGetError (void);
const char *alutGetErrorString (ALenum error);

ALuint alutCreateBufferFromFile (const char *fileName);
ALuint alutCreateBufferFromFileImage (const ALvoid *data, ALsizei length);

ALvoid *alutLoadMemoryFromFile (const char *fileName, ALenum *format, ALsizei *size, ALfloat *frequency);
ALvoid *alutLoadMemoryFromFileImage (const ALvoid *data, ALsizei length, ALenum *format, ALsizei *size, ALfloat *frequency);

const char *alutGetMIMETypes (ALenum loader);

ALint alutGetMajorVersion (void);
ALint alutGetMinorVersion (void);

#endif
