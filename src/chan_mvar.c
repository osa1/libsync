#include "chan_mvar.h"
#include "mvar.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct chan_mvar_
{
    mvar* read_end;

    // Write end always points to an empty mvar.
    mvar* write_end;
} chan_mvar;

typedef struct cell_
{
    void* elem;
    mvar* next;
} cell;

chan_mvar* chan_mvar_new()
{
    chan_mvar* ret = malloc(sizeof(chan_mvar));
    mvar* hole = mvar_new();
    ret->read_end = mvar_new();
    ret->write_end = mvar_new();

    mvar_put(ret->read_end, hole);
    mvar_put(ret->write_end, hole);

    return ret;
}

void chan_mvar_free(chan_mvar* chan)
{
    mvar* write_end_hole = mvar_take(chan->write_end);
    mvar* head = mvar_take(chan->read_end);

    if (head != write_end_hole)
    {
        printf("freeing non-empty channels is not supported.\n");
        exit(1);
    }

    mvar_free(write_end_hole);
    mvar_free(chan->write_end);
    mvar_free(chan->read_end);
    free(chan);
}

void chan_write(chan_mvar* chan, void* val)
{
    // The invariant here is that the mvar pointed my write_end is always be empty.
    mvar* old_hole = mvar_take(chan->write_end);
    // Write end will point to a new hole, old hole will be filled with the arg
    mvar* new_hole = mvar_new();
    // Allocate new cons cell
    cell* new_cell = malloc(sizeof(cell));
    new_cell->elem = val;
    new_cell->next = new_hole;
    // Update the old hole
    mvar_put(old_hole, new_cell);
    // Update the channel
    mvar_put(chan->write_end, new_hole);
}

void* chan_read(chan_mvar* chan)
{
    mvar* head = mvar_take(chan->read_end);
    cell* cell = mvar_take(head);
    mvar_put(chan->read_end, cell->next);
    mvar_free(head);
    void* ret  = cell ->elem;
    free(cell);
    return ret;
}
