#ifndef __LIBSYNC_MVAR_H
#define __LIBSYNC_MVAR_H

/**
 * An mvar is a mutable cell with a FIFO queue for multi-threaded use.
 * Initially it's empty. Writing (`mvar_put()`) to an empty mvar and reading
 * (`mvar_take()`) a filled mvar do not block. Otherwise threads threads blocks
 * and continue in FIFO order.
 */

typedef struct mvar_ mvar;

/**
 * Create a new empty mvar.
 */
mvar* mvar_new();

/**
 * Free an mvar. Threads blocked by this mvar will stay blocked forever so make
 * sure no threads are blocked before freeing an mvar.
 */
void mvar_free(mvar*);

/**
 * Write a value to an mvar. If the mvar not empty then this function blocks
 * until the thread is able to write.
 */
void mvar_put(mvar*, void*);

/**
 * Take value of an mvar. If the mvar is empty then this function blocks until
 * another thread puts a value in this mvar.
 */
void* mvar_take(mvar*);

#ifdef USE_EVENTFD

/**
 * Only available with -DUSE_EVENTFD. Returns an eventfd that will be signalled
 * when mvar is ready to be read via the returned eventfd. See
 * mvar_take_eventfd() for reading.
 *
 * Use this function with caution: mvars will still have FIFO property, so if
 * you forgot to call mvar_take_eventfd() when the eventfd is ready for
 * reading, you block all other threads.
 */
int mvar_get_take_eventfd(mvar*);

/**
 * Only available with -DUSE_EVENTFD. Takes the mvar using given eventfd.
 * Returns -1 if it's not eventfd's turn according to FIFO.
 */
int mvar_take_eventfd(mvar*, int fd, void** ret);

#endif

#endif
