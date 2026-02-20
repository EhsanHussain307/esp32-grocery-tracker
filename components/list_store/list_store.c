#include <stdio.h>

#include "list_store.h"
#include "catalog.h"



void list_store_print_missing(const list_store_t *s)
{
    printf("\n===== GROCERY LIST  =====\n");
    int count = 0;
    for (int i = 0; i < catalog_item_count(); i++) {
        const item_t *it = catalog_get_item(i);
        if (it && list_store_is_missing(s, it->id)) {
            printf("- %s\n", it->name);
            count++;
        }
    }
    if (count == 0) printf("(empty)\n");
    printf("=================================\n\n");
}


void list_store_init(list_store_t *s)
{
    s->missing_bits = 0;
}

bool list_store_is_missing(const list_store_t *s, uint8_t item_id)
{
    if (item_id >= 64) return false;
    return (s->missing_bits >> item_id) & 1ULL;
}

bool list_store_set_status(list_store_t *s, uint8_t item_id, item_status_t st)
{
    if (item_id >= 64) return false;
    uint64_t mask = (1ULL << item_id);
    bool was_missing = (s->missing_bits & mask) != 0;

    if (st == ITEM_MISSING) {
        s->missing_bits |= mask;
    } else {
        s->missing_bits &= ~mask;
    }

    bool now_missing = (s->missing_bits & mask) != 0;
    return was_missing != now_missing; // true only if change happened (prevents duplicates)
}

int list_store_first_missing_index(const list_store_t *s)
{
    for (int i = 0; i < catalog_item_count(); i++) {
        const item_t *it = catalog_get_item(i);
        if (it && list_store_is_missing(s, it->id)) return i;
    }
    return -1;
}

int list_store_next_missing_index(const list_store_t *s, int current_index, int direction)
{
    if (catalog_item_count() == 0) return -1;
    int i = current_index;
    for (int tries = 0; tries < catalog_item_count(); tries++) {
        i += direction;
        if (i < 0) i = catalog_item_count() - 1;
        if (i >= catalog_item_count()) i = 0;

        const item_t *it = catalog_get_item(i);
        if (it && list_store_is_missing(s, it->id)) return i;
    }
    return current_index;
}
uint64_t list_store_get_missing_bits(const list_store_t *s) { return s->missing_bits; }
void list_store_set_missing_bits(list_store_t *s, uint64_t bits) { s->missing_bits = bits; }
