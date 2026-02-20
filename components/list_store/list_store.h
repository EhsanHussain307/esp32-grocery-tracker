#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ITEM_HAVE = 0,
    ITEM_MISSING = 1
} item_status_t;

// simple bitset store up to 64 items
typedef struct {
    uint64_t missing_bits;
} list_store_t;

void list_store_init(list_store_t *s);

bool list_store_is_missing(const list_store_t *s, uint8_t item_id);

// returns true if state actually changed
bool list_store_set_status(list_store_t *s, uint8_t item_id, item_status_t st);

// find next missing item index in catalog order
int list_store_first_missing_index(const list_store_t *s);
int list_store_next_missing_index(const list_store_t *s, int current_index, int direction);

void list_store_print_missing(const list_store_t *s);

//for nvs storage
uint64_t list_store_get_missing_bits(const list_store_t *s);
void list_store_set_missing_bits(list_store_t *s, uint64_t bits);
