#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/mvar.h"
#include "src/chan_mvar.h"

typedef struct
{
    ptrdiff_t   i;
    void*       m;
} writer_arg;

void* mvar_writer_thr(void* arg0)
{
    writer_arg* arg = (writer_arg*)arg0;
    mvar_put((mvar*)arg->m, (void*)arg->i);
    return NULL;
}

void* mvar_reader_thr(void* arg0)
{
    mvar* arg = (mvar*)arg0;
    return mvar_take(arg);
}

void* chan_writer_thr(void* arg0)
{
    writer_arg* arg = (writer_arg*)arg0;
    chan_write((chan_mvar*)arg->m, (void*)arg->i);
    return NULL;
}

void* chan_reader_thr(void* arg0)
{
    chan_mvar* arg = (chan_mvar*)arg0;
    return chan_read(arg);
}

static int N = 10000;

////////////////////////////////////////////////////////////////////////////////
// I'm not sure what's a good way to test this program. Here I experiment with
// different order of thread creation: spawn readers first, spawn writers
// first, spawn in mixed order.

typedef void*(*mk_chan)();
typedef void*(*reader_thr)(void*);
typedef void*(*writer_thr)(void*);
typedef void(*free_chan)(void*);

void mvar_test_writers_first(mk_chan mk_chan, free_chan free_chan, reader_thr reader_thr, writer_thr writer_thr)
{
    pthread_t* writers = malloc(sizeof(pthread_t) * N);
    pthread_t* readers = malloc(sizeof(pthread_t) * N);

    void* m = mk_chan();

    writer_arg* args = malloc(sizeof(writer_arg) * N);
    for (int i = 0; i < N; ++i)
    {
        args[i].i = (ptrdiff_t)i;
        args[i].m = m;
    }

    bool* return_vals = malloc(sizeof(bool) * N);
    memset(return_vals, 0, sizeof(bool) * N);

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_create(writers + i, 0, writer_thr, args + i);
        if (pt_ret != 0)
        {
            printf("Error while creating writer thread: %s\n", strerror(pt_ret));
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_create(readers + i, 0, reader_thr, m);
        if (pt_ret != 0)
        {
            printf("Error while creating reader thread: %s\n", strerror(pt_ret));
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        void* ret;
        int pt_ret = pthread_join(readers[i], &ret);
        if (pt_ret != 0)
        {
            assert(pt_ret = EDEADLK);
            printf("Deadlock!\n");
            exit(1);
        }
        // printf("read: %p\n", ret);
        return_vals[(ptrdiff_t)ret] = true;
    }

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_join(writers[i], NULL);
        if (pt_ret != 0)
        {
            assert(pt_ret = EDEADLK);
            printf("Deadlock!\n");
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
        assert(return_vals[i]);

    free(writers);
    free(readers);
    free_chan(m);
    free(args);
    free(return_vals);
}

void mvar_test_readers_first(mk_chan mk_chan, free_chan free_chan, reader_thr reader_thr, writer_thr writer_thr)
{
    pthread_t* writers = malloc(sizeof(pthread_t) * N);
    pthread_t* readers = malloc(sizeof(pthread_t) * N);

    void* m = mk_chan();

    writer_arg* args = malloc(sizeof(writer_arg) * N);
    for (int i = 0; i < N; ++i)
    {
        args[i].i = (ptrdiff_t)i;
        args[i].m = m;
    }

    bool* return_vals = malloc(sizeof(bool) * N);
    memset(return_vals, 0, sizeof(bool) * N);

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_create(readers + i, 0, reader_thr, m);
        if (pt_ret != 0)
        {
            printf("Error while creating reader thread: %s\n", strerror(pt_ret));
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_create(writers + i, 0, writer_thr, args + i);
        if (pt_ret != 0)
        {
            printf("Error while creating writer thread: %s\n", strerror(pt_ret));
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
    {
        void* ret;
        int pt_ret = pthread_join(readers[i], &ret);
        if (pt_ret != 0)
        {
            assert(pt_ret = EDEADLK);
            printf("Deadlock!\n");
            exit(1);
        }
        // printf("read: %p\n", ret);
        return_vals[(ptrdiff_t)ret] = true;
    }

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_join(writers[i], NULL);
        if (pt_ret != 0)
        {
            assert(pt_ret = EDEADLK);
            printf("Deadlock!\n");
            exit(1);
        }
    }

    for (int i = 0; i < N; ++i)
        assert(return_vals[i]);

    free(writers);
    free(readers);
    free_chan(m);
    free(args);
    free(return_vals);
}

void mvar_test_mixed_order(mk_chan mk_chan, free_chan free_chan, reader_thr reader_thr, writer_thr writer_thr)
{
    pthread_t* writers = malloc(sizeof(pthread_t) * N);
    pthread_t* readers = malloc(sizeof(pthread_t) * N);

    void* m = mk_chan();

    writer_arg* args = malloc(sizeof(writer_arg) * N);
    for (int i = 0; i < N; ++i)
    {
        args[i].i = (ptrdiff_t)i;
        args[i].m = m;
    }

    bool* return_vals = malloc(sizeof(bool) * N);
    memset(return_vals, 0, sizeof(bool) * N);

    int writer_idx = 0;
    int reader_idx = 0;
    for (int i = 0; i < N * 2; ++i)
    {
        if (i % 2)
        {
            int pt_ret = pthread_create(readers + reader_idx++, 0, reader_thr, m);
            if (pt_ret != 0)
            {
                printf("Error while creating reader thread: %s\n", strerror(pt_ret));
                exit(1);
            }
        }
        else
        {
            int pt_ret = pthread_create(writers + writer_idx, 0, writer_thr, args + writer_idx);
            ++writer_idx;
            if (pt_ret != 0)
            {
                printf("Error while creating writer thread: %s\n", strerror(pt_ret));
                exit(1);
            }
        }
    }

    for (int i = 0; i < N; ++i)
    {
        void* ret;
        int pt_ret = pthread_join(readers[i], &ret);
        if (pt_ret != 0)
        {
            assert(pt_ret = EDEADLK);
            printf("Deadlock!\n");
            exit(1);
        }
        // printf("read: %p\n", ret);
        return_vals[(ptrdiff_t)ret] = true;
    }

    for (int i = 0; i < N; ++i)
    {
        int pt_ret = pthread_join(writers[i], NULL);
        if (pt_ret != 0)
        {
            assert(pt_ret = EDEADLK);
            printf("Deadlock!\n");
            exit(1);
        }
    }


    for (int i = 0; i < N; ++i)
        assert(return_vals[i]);

    free(writers);
    free(readers);
    free_chan(m);
    free(args);
    free(return_vals);
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    // 16345 * 2 = 32690 threads max on my laptop

    if (argc == 2)
        N = atoi(argv[1]);
    else if (argc != 1)
        printf("Can't parse cmd args, running with N = %d\n", N);

    // TODO: Test FIFO property

    printf("Testing mvars...\n");
    mvar_test_writers_first((mk_chan)mvar_new, (free_chan)mvar_free, mvar_reader_thr, mvar_writer_thr);
    mvar_test_readers_first((mk_chan)mvar_new, (free_chan)mvar_free, mvar_reader_thr, mvar_writer_thr);
    mvar_test_mixed_order((mk_chan)mvar_new, (free_chan)mvar_free, mvar_reader_thr, mvar_writer_thr);

    printf("Testing channels...\n");
    mvar_test_writers_first((mk_chan)chan_mvar_new, (free_chan)chan_mvar_free, chan_reader_thr, chan_writer_thr);
    mvar_test_readers_first((mk_chan)chan_mvar_new, (free_chan)chan_mvar_free, chan_reader_thr, chan_writer_thr);
    mvar_test_mixed_order((mk_chan)chan_mvar_new, (free_chan)chan_mvar_free, chan_reader_thr, chan_writer_thr);

    return 0;
}
