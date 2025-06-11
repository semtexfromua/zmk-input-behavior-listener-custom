/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_behavior_move_to_keypress

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

enum move_to_keypress_xy_data_mode {
    IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_NONE,
    IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL,
    IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_ABS,
};

struct move_to_keypress_xy_data {
    enum move_to_keypress_xy_data_mode mode;
    int16_t x_delta;
    int16_t y_delta;
};

struct behavior_move_to_keypress_data {
    const struct device *dev;
    struct move_to_keypress_xy_data data;
    int64_t last_trigger_time;
    uint16_t min_interval_ms;
};

struct behavior_move_to_keypress_config {
    int16_t threshold;
    uint16_t rate_limit_ms;
    bool x_invert;
    bool y_invert;
    struct zmk_behavior_binding bindings[4]; // right, left, up, down
};

static void handle_rel_code(const struct behavior_move_to_keypress_config *config,
                            struct behavior_move_to_keypress_data *data, struct input_event *evt) {
    // Listener вже відфільтрував події, обробляємо X та Y
    switch (evt->code) {
    case INPUT_REL_X:
        data->data.mode = IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL;
        int16_t x_val = evt->value;
        if (config->x_invert) {
            x_val = -x_val;
        }
        data->data.x_delta += x_val;
        break;
    case INPUT_REL_Y:
        data->data.mode = IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL;
        int16_t y_val = evt->value;
        if (config->y_invert) {
            y_val = -y_val;
        }
        data->data.y_delta += y_val;
        break;
    default:
        break;
    }
}

static void trigger_key_press(const struct zmk_behavior_binding *binding) {
    if (!binding->behavior_dev) {
        return;
    }

    struct zmk_behavior_binding_event evt = {
        .position = 0,
        .timestamp = k_uptime_get(),
    };

    // Trigger key press
    const struct device *behavior = zmk_behavior_get_binding(binding->behavior_dev);
    if (behavior) {
        const struct behavior_driver_api *api = (const struct behavior_driver_api *)behavior->api;
        if (api && api->binding_pressed) {
            api->binding_pressed((struct zmk_behavior_binding *)binding, evt);
        }
        // Trigger key release immediately after press
        if (api && api->binding_released) {
            api->binding_released((struct zmk_behavior_binding *)binding, evt);
        }
    }
}

static void check_and_trigger_movements(const struct behavior_move_to_keypress_config *config,
                                       struct behavior_move_to_keypress_data *data) {
    int64_t current_time = k_uptime_get();
    
    // Rate limiting: обмежуємо частоту генерації подій
    if (current_time - data->last_trigger_time < data->min_interval_ms) {
        return; // Занадто рано для наступної події
    }
    
    bool triggered = false;
    int max_events_per_cycle = 1; // Обмежуємо до 1 події за цикл
    int events_generated = 0;

    // Check X axis movement (left/right) - тільки одну подію за раз
    if (events_generated < max_events_per_cycle) {
        if (data->data.x_delta >= config->threshold) {
            // Move right
            trigger_key_press(&config->bindings[0]); // right binding
            data->data.x_delta -= config->threshold;
            events_generated++;
            triggered = true;
            LOG_DBG("Triggered RIGHT movement, remaining delta: %d", data->data.x_delta);
        } else if (data->data.x_delta <= -config->threshold) {
            // Move left  
            trigger_key_press(&config->bindings[1]); // left binding
            data->data.x_delta += config->threshold;
            events_generated++;
            triggered = true;
            LOG_DBG("Triggered LEFT movement, remaining delta: %d", data->data.x_delta);
        }
    }

    // Check Y axis movement (up/down) - тільки якщо ще не згенерували подію по X
    if (events_generated < max_events_per_cycle) {
        if (data->data.y_delta >= config->threshold) {
            // Move down
            trigger_key_press(&config->bindings[3]); // down binding
            data->data.y_delta -= config->threshold;
            events_generated++;
            triggered = true;
            LOG_DBG("Triggered DOWN movement, remaining delta: %d", data->data.y_delta);
        } else if (data->data.y_delta <= -config->threshold) {
            // Move up
            trigger_key_press(&config->bindings[2]); // up binding  
            data->data.y_delta += config->threshold;
            events_generated++;
            triggered = true;
            LOG_DBG("Triggered UP movement, remaining delta: %d", data->data.y_delta);
        }
    }
    
    // Оновлюємо час останньої події тільки якщо щось було згенеровано
    if (triggered) {
        data->last_trigger_time = current_time;
        
        // Обмежуємо накопичення delta для запобігання зависанню
        const int16_t max_delta = config->threshold * 2; // Зменшили з 3 до 2
        if (data->data.x_delta > max_delta) data->data.x_delta = max_delta;
        if (data->data.x_delta < -max_delta) data->data.x_delta = -max_delta;
        if (data->data.y_delta > max_delta) data->data.y_delta = max_delta;
        if (data->data.y_delta < -max_delta) data->data.y_delta = -max_delta;
    }
}

static int move_to_keypress_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_move_to_keypress_data *data = 
        (struct behavior_move_to_keypress_data *)dev->data;
    const struct behavior_move_to_keypress_config *config = dev->config;
    
    struct input_event *evt = (struct input_event *)event.position;
    
    // Listener вже відфільтрував події, просто обробляємо
    if (!evt->value) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    switch (evt->type) {
    case INPUT_EV_REL:
        handle_rel_code(config, data, evt);
        break;
    default:
        return ZMK_BEHAVIOR_TRANSPARENT;
    }

    if (data->data.mode == IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL) {
        check_and_trigger_movements(config, data);
        
        // Always consume the event since we're handling trackball input
        evt->value = 0;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return ZMK_BEHAVIOR_TRANSPARENT;
}

// ВИДАЛЕНО: binding_released викликається при відпусканні кнопок трекболу, 
// а не при зміні layer, що призводить до зависання. 
// Скористаємося підходом input_behavior_scaler - без released handler.

static int input_behavior_move_to_keypress_init(const struct device *dev) {
    struct behavior_move_to_keypress_data *data = dev->data;
    data->dev = dev;
    data->data.mode = IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_NONE;
    data->data.x_delta = 0;
    data->data.y_delta = 0;
    data->last_trigger_time = 0;
    const struct behavior_move_to_keypress_config *config = dev->config;
    data->min_interval_ms = config->rate_limit_ms;
    return 0;
};

static const struct behavior_driver_api behavior_move_to_keypress_driver_api = {
    .binding_pressed = move_to_keypress_keymap_binding_pressed,
    // НЕМАЄ .binding_released - як у input_behavior_scaler
};

#define MOVE_TO_KEYPRESS_BINDING(idx, node_id) \
    { \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(node_id, bindings, idx)), \
        .param1 = COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, bindings, idx, param1), \
                             (DT_PHA_BY_IDX(node_id, bindings, idx, param1)), (0)), \
        .param2 = COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, bindings, idx, param2), \
                             (DT_PHA_BY_IDX(node_id, bindings, idx, param2)), (0)), \
    }

#define IBMTK_INST(n)                                                                                \
    static struct behavior_move_to_keypress_data behavior_move_to_keypress_data_##n = {};                \
    static struct behavior_move_to_keypress_config behavior_move_to_keypress_config_##n = {              \
        .threshold = DT_INST_PROP(n, threshold),                                                   \
        .rate_limit_ms = DT_INST_PROP_OR(n, rate_limit_ms, 50),                                    \
        .x_invert = DT_INST_PROP_OR(n, x_invert, false),                                           \
        .y_invert = DT_INST_PROP_OR(n, y_invert, false),                                           \
        .bindings = {                                                                              \
            MOVE_TO_KEYPRESS_BINDING(0, DT_DRV_INST(n)), /* right */                                        \
            MOVE_TO_KEYPRESS_BINDING(1, DT_DRV_INST(n)), /* left */                                         \
            MOVE_TO_KEYPRESS_BINDING(2, DT_DRV_INST(n)), /* up */                                           \
            MOVE_TO_KEYPRESS_BINDING(3, DT_DRV_INST(n)), /* down */                                         \
        },                                                                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, input_behavior_move_to_keypress_init, NULL,                            \
                            &behavior_move_to_keypress_data_##n,                                      \
                            &behavior_move_to_keypress_config_##n,                                    \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_move_to_keypress_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IBMTK_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */ 