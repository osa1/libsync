#ifndef __LIBSYNC_CHAN_MVAR_H
#define __LIBSYNC_CHAN_MVAR_H

/**
 * A multi-producer, multi-reader thread-safe FIFO channel implementation based
 * on mvars. In other words, `chan_write()` and `chan_read()` functions are
 * thread safe. If a channel is empty read operation blocks until a value
 * becomes available. Readers blocked in `chan_read()` read values in FIFO
 * order.
 */

typedef struct chan_mvar_ chan_mvar;

/**
 * Create an empty channel.
 */
chan_mvar* chan_mvar_new();

/**
 * Free a channel. Readers blocked in `chan_read()` will stay blocked forever,
 * so make sure no thread is blocked.
 */
void chan_mvar_free(chan_mvar*);

/**
 * Write a value to a channel. Does not block.
 */
void chan_write(chan_mvar*, void*);

/**
 * Read a value from a channel. Blocks if the channel is empty. Threads blocked
 * by this function read values in FIFO order.
 */
void* chan_read(chan_mvar*);

#endif
