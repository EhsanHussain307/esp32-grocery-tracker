#include "catalog.h"
#include <stddef.h>

static item_t ITEMS[] = {
    // Quick items (digits). Adjust to your favorites.
    { 0, CAT_FRIDGE,  "Milk",     '1' },
    { 1, CAT_FRIDGE,  "Eggs",     '2' },
    { 2, CAT_PANTRY,  "Bread",    '3' },
    { 3, CAT_PANTRY,  "Rice",     '4' },
    { 4, CAT_PANTRY,  "Sugar",    '5' },
    { 5, CAT_PANTRY,  "Coffee",   '6' },
    { 6, CAT_PANTRY,  "Tuna",     '7' },
    { 7, CAT_PANTRY,  "Oil",      '8' },
    { 8, CAT_FRIDGE,  "Cheese",   '9' },
    { 9, CAT_FRIDGE,  "Yogurt",   '0' },

    // Browse-only items (no quick key)
    {10, CAT_PANTRY,   "Pasta",     0 },
    {11, CAT_PANTRY,   "Flour",     0 },
    {12, CAT_PANTRY,   "Tea",       0 },
    {13, CAT_FRIDGE,   "Chicken",   0 },
    {14, CAT_FRIDGE,   "Tomatoes",  0 },
    {15, CAT_CLEANING, "Detergent", 0 },
};

void catalog_init_defaults(void) { /* nothing */ }

int catalog_item_count(void) {
    return (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));
}

const item_t* catalog_get_item(int index) {
    if (index < 0 || index >= catalog_item_count()) return NULL;
    return &ITEMS[index];
}

int catalog_find_by_quick_key(char key)
{
    for (int i = 0; i < catalog_item_count(); i++) {
        if (ITEMS[i].quick_key == key) return ITEMS[i].id;
    }
    return -1;
}

static int find_index_by_id(uint8_t id)
{
    for (int i = 0; i < catalog_item_count(); i++) {
        if (ITEMS[i].id == id) return i;
    }
    return -1;
}

int catalog_first_index_in_category(category_t cat)
{
    for (int i = 0; i < catalog_item_count(); i++) {
        if (ITEMS[i].cat == cat) return i;
    }
    return -1;
}

int catalog_next_index_in_category(category_t cat, int current_index, int direction)
{
    if (current_index < 0) return catalog_first_index_in_category(cat);

    int i = current_index;
    while (1) {
        i += direction;
        if (i < 0) i = catalog_item_count() - 1;
        if (i >= catalog_item_count()) i = 0;
        if (ITEMS[i].cat == cat) return i;
        if (i == current_index) break;
    }
    return current_index;
}
