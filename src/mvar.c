#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mvar.h"

// TODO: Use semaphore instead of eventfd

typedef struct list_node_
{
    sem_t*              sem;
    struct list_node_*  next;
    struct list_node_*  prev;
} list_node;

typedef struct list_
{
    list_node*          head;
    list_node*          tail;
} list;

static list* list_new()
{
    list* ret = malloc(sizeof(list));
    ret->head = NULL;
    ret->tail = NULL;
    return ret;
}

static void list_free(list* l)
{
    list_node* n = l->head;
    while (n)
    {
        list_node* next = n->next;
        free(n);
        n = next;
    }
    free(l);
}

/*
static void list_push_front(list* l, int i)
{
    list_node* node = malloc(sizeof(list_node));
    node->i    = i;
    node->prev = NULL;

    if (!l->head)
    {
        assert(!l->tail);
        l->head = node;
        l->tail = node;
        l->head->next = NULL;
    }
    else
    {
        assert(l->tail);
        node->next = l->head;
        l->head = node;
    }
}
*/

static void list_push_back(list* l, sem_t* sem)
{
    list_node* node = malloc(sizeof(list_node));
    node->sem  = sem;
    node->next = NULL;

    if (!l->tail)
    {
        assert(!l->head);
        l->head = node;
        l->tail = node;
        l->head->prev = NULL;
    }
    else
    {
        assert(l->head);
        node->prev = l->tail;
        l->tail->next = node;
        l->tail = node;
    }
}

static sem_t* list_pop_front(list* l)
{
    assert(l->head);
    sem_t* ret = l->head->sem;

    if (l->head == l->tail)
    {
        free(l->head);
        l->head = NULL;
        l->tail = NULL;
    }
    else
    {
        list_node* head = l->head;
        l->head = l->head->next;
        l->head->prev = NULL;
        free(head);
    }

    return ret;
}

static bool list_is_empty(list* l)
{
    if (!l->head)
        assert(!l->tail);
    return !l->head;
}

typedef struct mvar_
{
    pthread_mutex_t lock;
    bool            is_empty;
    void*           value;
    list*           write_list;
    list*           read_list;
} mvar;

mvar* mvar_new()
{
    mvar* ret = malloc(sizeof(mvar));
    pthread_mutex_init(&ret->lock, 0);
    ret->is_empty   = true;
    ret->write_list = list_new();
    ret->read_list  = list_new();
    return ret;
}

void mvar_free(mvar* m)
{
    assert(list_is_empty(m->write_list));
    assert(list_is_empty(m->read_list));
    pthread_mutex_destroy(&m->lock);
    list_free(m->write_list);
    list_free(m->read_list);
    free(m);
}

void mvar_put(mvar* mvar, void* value)
{
    int ret = pthread_mutex_lock(&mvar->lock);
    assert(ret == 0);

    // add self to the queue if
    // - write list is not empty (it's some other thread's turn)
    // - mvar is not empty
    if (!list_is_empty(mvar->write_list) || !mvar->is_empty)
    {
        // add self to the queue ...
        sem_t self;
        sem_init(&self, 0 /* not shared between procs */, 0 /* value */);
        list_push_back(mvar->write_list, &self);

        // ... and sleep
        pthread_mutex_unlock(&mvar->lock);
        sem_wait(&self);

        // it's our turn now, remove self from the list
        pthread_mutex_lock(&mvar->lock);
        assert(mvar->write_list->head->sem == &self);
        assert(mvar->is_empty);
        (void)list_pop_front(mvar->write_list);
        sem_destroy(&self);
    }

    mvar->value      = value;
    mvar->is_empty   = false;

    // signal a reader
    if (!list_is_empty(mvar->read_list))
    {
        // printf("signaling a reader\n");
        sem_post(mvar->read_list->head->sem);
    }

    // we're done here
    pthread_mutex_unlock(&mvar->lock);
}

void* mvar_take(mvar* mvar)
{
    int ret = pthread_mutex_lock(&mvar->lock);
    assert(ret == 0);

    // same as mvar_put, check the queue first
    if (!list_is_empty(mvar->read_list) || mvar->is_empty)
    {
        sem_t self;
        sem_init(&self, 0 /* not shared between procs */, 0 /* value */);
        list_push_back(mvar->read_list, &self);
        pthread_mutex_unlock(&mvar->lock);
        sem_wait(&self);
        pthread_mutex_lock(&mvar->lock);
        assert(mvar->read_list->head->sem == &self);
        assert(!mvar->is_empty);
        (void)list_pop_front(mvar->read_list);
        sem_destroy(&self);
    }

    mvar->is_empty = true;

    // signal a writer
    if (!list_is_empty(mvar->write_list))
    {
        // printf("signaling a writer\n");
        sem_post(mvar->write_list->head->sem);
    }

    void* value = mvar->value;
    pthread_mutex_unlock(&mvar->lock);
    return value;
}
