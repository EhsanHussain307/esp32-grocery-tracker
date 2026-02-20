#include "ui.h"
#include "display.h"
#include <stdio.h>
#include <string.h>

#include "storage_nvs.h"
#include "mqtt_mgr.h"

static void set_message(ui_ctx_t *ui, const char *l1, const char *l2, int ms)
{
    ui->state = UI_MESSAGE;
    snprintf(ui->msg1, sizeof(ui->msg1), "%s", l1 ? l1 : "");
    snprintf(ui->msg2, sizeof(ui->msg2), "%s", l2 ? l2 : "");
    ui->msg_ms_left = ms;
}

static const char* cat_name(category_t c)
{
    switch (c) {
        case CAT_PANTRY: return "Pantry";
        case CAT_FRIDGE: return "Fridge";
        case CAT_CLEANING: return "Cleaning";
        default: return "?";
    }
}

void ui_init(ui_ctx_t *ui, list_store_t *store)
{
    memset(ui, 0, sizeof(*ui));
    ui->store = store;
    ui->state = UI_HOME;
    ui->current_cat = CAT_PANTRY;
    ui->current_index = 0;
}

static void quick_add(ui_ctx_t *ui, char digit_key)
{
    int item_id = catalog_find_by_quick_key(digit_key);
    if (item_id < 0) {
        set_message(ui, "No item mapped", "Edit catalog.c", 1200);
        return;
    }

    bool changed = list_store_set_status(ui->store, (uint8_t)item_id, ITEM_MISSING);
    // Find name
    const char *name = "?";
    for (int i = 0; i < catalog_item_count(); i++) {
        const item_t *it = catalog_get_item(i);
        if (it && it->id == item_id) { name = it->name; break; }
    }

    if (changed) {
        char l1[32]; snprintf(l1, sizeof(l1), "Added:");
        char l2[32]; snprintf(l2, sizeof(l2), "%s", name);
        set_message(ui, l1, l2, 1200);
            list_store_print_missing(ui->store);   // <-- ADD HERE
          
            storage_nvs_save_missing_bits(list_store_get_missing_bits(ui->store));
           mqtt_mgr_publish_item((uint8_t)item_id, name, "MISSING");
    } else {
        set_message(ui, "Already on list", name, 1200);
    }
}

void ui_handle_key(ui_ctx_t *ui, char key)
{
    // Digits are always quick-add when on HOME
    if (ui->state == UI_HOME) {
        if (key >= '0' && key <= '9') {
            quick_add(ui, key);
            return;
        }
        if (key == 'D') { // Browse
            ui->state = UI_BROWSE_CAT;
            ui->current_cat = CAT_PANTRY;
            return;
        }
        if (key == 'C') { // Bought mode
            ui->state = UI_BOUGHT_MODE;
            ui->current_index = list_store_first_missing_index(ui->store);
            if (ui->current_index < 0) {
                set_message(ui, "Shopping list", "is empty ✅", 1500);
                ui->state = UI_HOME;
            }
            return;
        }
    }

    if (ui->state == UI_BROWSE_CAT) {
       if (key == '#') { ui->state = UI_HOME; return; }   // # = Back

if (key == '1') ui->current_cat = CAT_PANTRY;
else if (key == '2') ui->current_cat = CAT_FRIDGE;
else if (key == '3') ui->current_cat = CAT_CLEANING;
else if (key == '*') {                              // * = Confirm/Select
    ui->state = UI_BROWSE_ITEMS;
    ui->current_index = catalog_first_index_in_category(ui->current_cat);
}
return;

        }
    
    

    if (ui->state == UI_BROWSE_ITEMS) {
    if (key == '#') { ui->state = UI_BROWSE_CAT; return; }  // # = Back

if (key == 'A') ui->current_index = catalog_next_index_in_category(ui->current_cat, ui->current_index, -1);
else if (key == 'B') ui->current_index = catalog_next_index_in_category(ui->current_cat, ui->current_index, +1);
else if (key == '*') {                                   // * = Add
    const item_t *it = catalog_get_item(ui->current_index);
    if (!it) return;
    bool changed = list_store_set_status(ui->store, it->id, ITEM_MISSING);
    if (changed) set_message(ui, "Added:", it->name, 1200);
    else set_message(ui, "Already on list", it->name, 1200);

    // Optional: print list after change
 if (changed) list_store_print_missing(ui->store);
storage_nvs_save_missing_bits(list_store_get_missing_bits(ui->store));
mqtt_mgr_publish_item(it->id, it->name, "MISSING");
    ui->state = UI_BROWSE_ITEMS;
}
return;

    }

    if (ui->state == UI_BOUGHT_MODE) {
       if (key == '#') { ui->state = UI_HOME; return; }   // # = Back
if (ui->current_index < 0) { ui->state = UI_HOME; return; }

if (key == 'A') ui->current_index = list_store_next_missing_index(ui->store, ui->current_index, -1);
else if (key == 'B') ui->current_index = list_store_next_missing_index(ui->store, ui->current_index, +1);
else if (key == '*') {                              // * = Bought/Confirm
    const item_t *it = catalog_get_item(ui->current_index);
    if (!it) return;
    bool changed = list_store_set_status(ui->store, it->id, ITEM_HAVE);
    if (changed) set_message(ui, "Bought ✅", it->name, 1200);

    // Optional: print list after change
     if (changed) list_store_print_missing(ui->store);

     storage_nvs_save_missing_bits(list_store_get_missing_bits(ui->store));
mqtt_mgr_publish_item(it->id, it->name, "HAVE");
    ui->current_index = list_store_first_missing_index(ui->store);
    if (ui->current_index < 0) ui->state = UI_HOME;
}
return;

    }

    if (ui->state == UI_MESSAGE) {
        if (key == '#') ui->state = UI_HOME;
        return;
    }
}

void ui_render(ui_ctx_t *ui)
{
    // simple timer-less message: we just show it; main loop redraws anyway
    if (ui->state == UI_HOME) {
        display_draw_lines("Grocery List",
                           "0-9: Quick Add",
                           "D:find C:bought",
                          "*:Select  #:Back"
);
        return;
    }

    if (ui->state == UI_BROWSE_CAT) {
        char l2[32];
        snprintf(l2, sizeof(l2), "Cat: %s", cat_name(ui->current_cat));
        display_draw_lines("BROWSE CATEGORIES",
                           "1:Pantry 2:Fridge",
                           "3:Cleaning",
                           "*:Select  #:Back");
        return;
    }

    if (ui->state == UI_BROWSE_ITEMS) {
        const item_t *it = catalog_get_item(ui->current_index);
        display_draw_lines("BROWSE ITEMS",
                           it ? it->name : "(none)",
                           "A:Prev  B:Next",
                           "*:Add   #:Back");
        return;
    }

    if (ui->state == UI_BOUGHT_MODE) {
        const item_t *it = catalog_get_item(ui->current_index);
        display_draw_lines("BOUGHT MODE",
                           it ? it->name : "(empty)",
                           "A:Prev  B:Next",
                           "*:Bought  #:Back");
        return;
    }

    if (ui->state == UI_MESSAGE) {
        display_draw_lines(ui->msg1, ui->msg2, "", "(# to Home)");
        return;
    }
}
