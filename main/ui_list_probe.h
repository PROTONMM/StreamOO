#pragma once
#include "streamoo_ui.h"

typedef struct {
    streamoo_ui_t ui;
    unsigned next_event;
    uint64_t requested_id;
} streamoo_ui_list_probe_t;

bool streamoo_ui_list_probe_init(streamoo_ui_list_probe_t *probe);
/* Synthetic navigation and catalog for display diagnostics; no player attached. */
bool streamoo_ui_list_probe_draw(streamoo_ui_list_probe_t *probe,
                                  uint8_t *frame, size_t bytes, uint64_t elapsed_ms);
