#ifndef __LIBSYNC_MVAR_H
#define __LIBSYNC_MVAR_H

typedef struct mvar_ mvar;

mvar* mvar_new();
void mvar_free(mvar*);
void mvar_put(mvar*, void*);
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
