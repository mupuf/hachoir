#ifndef RINGBUFFERHACHOIR_H
#define RINGBUFFERHACHOIR_H

#include "samplesringbuffer.h"

struct RBMarker { uint64_t freq; uint64_t time; };
typedef SamplesRingBuffer<gr_complex, RBMarker> SamplesRingBufferHachoir;

#endif // RINGBUFFERHACHOIR_H
