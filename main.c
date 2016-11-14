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

void* mvar_reader_thr_p(void* arg0)
{
    writer_arg* arg = (writer_arg*)arg0;
    (void)mvar_take(arg->m);
    printf("reader %lu returning\n", arg->i);
    return NULL;
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

typedef void*   (*mk_chan)     ();
typedef void*   (*reader_thr)  (void*);
typedef void*   (*writer_thr)  (void*);
typedef void    (*free_chan)   (void*);

void create_thread(pthread_t* thr, void*(thr_fn)(void*), void* arg)
{
    int pt_ret = pthread_create(thr, 0, thr_fn, arg);
    if (pt_ret != 0)
    {
        printf("Error while creating writer thread: %s\n", strerror(pt_ret));
        exit(1);
    }
}

void** wait_threads(pthread_t* thrs, int n, bool collect_rets)
{
    void** rets = collect_rets ? malloc(sizeof(void*) * n) : NULL;

    for (int i = 0; i < n; ++i)
    {
        int pt_ret = pthread_join(*(thrs + i), collect_rets ? rets + i : NULL);
        if (pt_ret != 0)
        {
            printf("pthread_join() failed: %s\n", strerror(pt_ret)); // probably EDEADLK
            exit(1);
        }
    }

    return rets;
}

void mvar_test_writers_first(mk_chan mk_chan, free_chan free_chan, reader_thr reader_thr, writer_thr writer_thr,
                             bool wait_writers /* wait for writers to return first */)
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
        create_thread(writers + i, writer_thr, args + i);

    if (wait_writers)
        (void)wait_threads(writers, N, false);

    for (int i = 0; i < N; ++i)
        create_thread(readers + i, reader_thr, m);

    {
        void** rets = wait_threads(readers, N, true);
        for (int i = 0; i < N; ++i)
            return_vals[(ptrdiff_t)rets[i]] = true;
        free(rets);
    }

    if (!wait_writers)
        (void)wait_threads(writers, N, false);

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
        create_thread(readers + i, reader_thr, m);

    for (int i = 0; i < N; ++i)
        create_thread(writers + i, writer_thr, args + i);

    {
        void** rets = wait_threads(readers, N, true);
        for (int i = 0; i < N; ++i)
            return_vals[(ptrdiff_t)rets[i]] = true;
        free(rets);
    }

    (void)wait_threads(writers, N, false);

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
        if (i % 2)
            create_thread(readers + reader_idx++, reader_thr, m);
        else
        {
            create_thread(writers + writer_idx, writer_thr, args + writer_idx);
            ++writer_idx;
        }

    {
        void** rets = wait_threads(readers, N, true);
        for (int i = 0; i < N; ++i)
            return_vals[(ptrdiff_t)rets[i]] = true;
        free(rets);
    }

    (void)wait_threads(writers, N, false);

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
    mvar_test_writers_first((mk_chan)mvar_new, (free_chan)mvar_free, mvar_reader_thr, mvar_writer_thr, false);
    mvar_test_readers_first((mk_chan)mvar_new, (free_chan)mvar_free, mvar_reader_thr, mvar_writer_thr);
    mvar_test_mixed_order((mk_chan)mvar_new, (free_chan)mvar_free, mvar_reader_thr, mvar_writer_thr);

    printf("Testing channels...\n");
    mvar_test_writers_first(
            (mk_chan)chan_mvar_new, (free_chan)chan_mvar_free, chan_reader_thr, chan_writer_thr, true);
    mvar_test_readers_first((mk_chan)chan_mvar_new, (free_chan)chan_mvar_free, chan_reader_thr, chan_writer_thr);
    mvar_test_mixed_order((mk_chan)chan_mvar_new, (free_chan)chan_mvar_free, chan_reader_thr, chan_writer_thr);

    printf("Testing mvar FIFO property...\n");
    {
        mvar* m = mvar_new();
        pthread_t threads[10];
        writer_arg args[10];
        for (int i = 0; i < 10; ++i)
        {
            args[i].i = i;
            args[i].m = m;
            create_thread(threads + i, mvar_reader_thr_p, args + i);
            usleep(100000); // FIXME: any better ways to make sure all reader
                            // threads are queued up?
        }

        for (int i = 0; i < 10; ++i)
            mvar_put(m, NULL);

        wait_threads(threads, 10, false);
        mvar_free(m);
    }

    return 0;
}
