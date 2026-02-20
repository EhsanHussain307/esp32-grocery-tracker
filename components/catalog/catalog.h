#pragma once
#include <stdint.h>

typedef enum {
    CAT_PANTRY = 0,
    CAT_FRIDGE = 1,
    CAT_CLEANING = 2,
    CAT_COUNT
} category_t;

typedef struct {
    uint8_t id;
    category_t cat;
    const char *name;
    char quick_key;   // '0'..'9' or 0 if not quick
} item_t;

void catalog_init_defaults(void);

int catalog_item_count(void);
const item_t* catalog_get_item(int index);

// quick key -> item id, returns -1 if none
int catalog_find_by_quick_key(char key);

// for browse lists
int catalog_first_index_in_category(category_t cat);
int catalog_next_index_in_category(category_t cat, int current_index, int direction); // direction: +1 / -1
