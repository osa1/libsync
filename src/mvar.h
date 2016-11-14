#ifndef __LIBSYNC_MVAR_H
#define __LIBSYNC_MVAR_H

typedef struct mvar_ mvar;

mvar* mvar_new();
void mvar_free();
void mvar_put(mvar*, void*);
void* mvar_take(mvar*);

#endif
