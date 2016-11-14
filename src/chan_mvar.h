#ifndef __LIBSYNC_CHAN_MVAR_H
#define __LIBSYNC_CHAN_MVAR_H

// A multi-producer, multi-reader thread-safe FIFO channel implementation based
// on mvars.

typedef struct chan_mvar_ chan_mvar;

chan_mvar* chan_mvar_new();
void chan_mvar_free(chan_mvar*);
void chan_write(chan_mvar*, void*);
void* chan_read(chan_mvar*);

#endif
