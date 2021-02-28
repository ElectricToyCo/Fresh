#include "alutInternal.h"

ALvoid *
_alutMalloc (size_t size)
{
  ALvoid *ptr = malloc (size == 0 ? 1 : size);
  if (ptr == NULL)
    {
      _alutSetError (ALUT_ERROR_OUT_OF_MEMORY);
    }
  return ptr;
}

ALboolean
_alutFormatConstruct (ALint numChannels, ALint bitsPerSample, ALenum *format)
{
  switch (numChannels)
    {
    case 1:
      switch (bitsPerSample)
        {
        case 8:
          *format = AL_FORMAT_MONO8;
          return AL_TRUE;
        case 16:
          *format = AL_FORMAT_MONO16;
          return AL_TRUE;
        }
      break;
    case 2:
      switch (bitsPerSample)
        {
        case 8:
          *format = AL_FORMAT_STEREO8;
          return AL_TRUE;
        case 16:
          *format = AL_FORMAT_STEREO16;
          return AL_TRUE;
        }
      break;
    }
  return AL_FALSE;
}

ALboolean
_alutFormatGetNumChannels (ALenum format, ALint *numChannels)
{
  switch (format)
    {
    case AL_FORMAT_MONO8:
    case AL_FORMAT_MONO16:
      *numChannels = 1;
      return AL_TRUE;
    case AL_FORMAT_STEREO8:
    case AL_FORMAT_STEREO16:
      *numChannels = 2;
      return AL_TRUE;
    }
  return AL_FALSE;
}

ALboolean
_alutFormatGetBitsPerSample (ALenum format, ALint *bitsPerSample)
{
  switch (format)
    {
    case AL_FORMAT_MONO8:
    case AL_FORMAT_STEREO8:
      *bitsPerSample = 8;
      return AL_TRUE;
    case AL_FORMAT_MONO16:
    case AL_FORMAT_STEREO16:
      *bitsPerSample = 16;
      return AL_TRUE;
    }
  return AL_FALSE;
}
