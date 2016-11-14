#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stddef.h>

#include "src/mvar.h"

typedef struct
{
    ptrdiff_t   i;
    mvar*       m;
} writer_arg;

void* writer_thr(void* arg0)
{
    printf("writer\n");
    writer_arg* arg = (writer_arg*)arg0;
    mvar_put(arg->m, (void*)arg->i);
    return NULL;
}

void* reader_thr(void* arg0)
{
    printf("reader\n");
    mvar* arg = (mvar*)arg0;
    return mvar_take(arg);
}

#define N 100

int main()
{
    pthread_t writers[N];
    pthread_t readers[N];

    mvar* m = mvar_new();

    writer_arg args[N];
    for (int i = 0; i < N; ++i)
    {
        args[i].i = (ptrdiff_t)i;
        args[i].m = m;
    }

    for (int i = 0; i < N; ++i)
        pthread_create(&writers[i], 0, writer_thr, &args[i]);

    for (int i = 0; i < N; ++i)
        pthread_create(&readers[i], 0, reader_thr, m);

    for (int i = 0; i < N; ++i)
        pthread_join(writers[i], NULL);

    for (int i = 0; i < N; ++i)
    {
        void* ret;
        pthread_join(readers[i], &ret);
        printf("read: %d\n", (int)(ptrdiff_t)ret);
    }

    mvar_free(m);
    return 0;
}
