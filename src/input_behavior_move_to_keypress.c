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
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

enum move_to_keypress_xy_data_mode {
    IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_NONE,
    IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL,
};

struct move_to_keypress_xy_data {
    enum move_to_keypress_xy_data_mode mode;
    int16_t x_delta;
    int16_t y_delta;
};

struct behavior_move_to_keypress_data {
    const struct device *dev;
    struct move_to_keypress_xy_data data;
    
    struct k_work_delayable key_press_work;
    struct k_work_delayable key_release_work;
    
    struct zmk_behavior_binding current_binding;
    struct zmk_behavior_binding_event current_event;
    
    int64_t last_trigger_time;
    bool work_scheduled;
    uint8_t active_layer;
};

struct behavior_move_to_keypress_config {
    int16_t threshold;
    int16_t x_threshold;
    int16_t y_threshold;
    int16_t rate_limit_ms;
    bool x_invert;
    bool y_invert;
    bool reset_other_axis;
    struct zmk_behavior_binding bindings[4]; // RIGHT, LEFT, UP, DOWN
};

static void handle_rel_code(const struct behavior_move_to_keypress_config *config,
                            struct behavior_move_to_keypress_data *data, 
                            struct input_event *evt) {
    switch (evt->code) {
    case INPUT_REL_X:
        data->data.mode = IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL;
        int16_t x_val = config->x_invert ? -evt->value : evt->value;
        data->data.x_delta += x_val;
        break;
    case INPUT_REL_Y:
        data->data.mode = IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL;
        int16_t y_val = config->y_invert ? -evt->value : evt->value;
        data->data.y_delta += y_val;
        break;
    default:
        break;
    }
    
    const int16_t x_max_delta = config->x_threshold * 3;
    const int16_t y_max_delta = config->y_threshold * 3;
    data->data.x_delta = CLAMP(data->data.x_delta, -x_max_delta, x_max_delta);
    data->data.y_delta = CLAMP(data->data.y_delta, -y_max_delta, y_max_delta);
}

static void key_press_work_cb(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct behavior_move_to_keypress_data *data = CONTAINER_OF(work_delayable,
                                                              struct behavior_move_to_keypress_data,
                                                              key_press_work);
    
    if (!zmk_keymap_layer_active(data->active_layer)) {
        data->work_scheduled = false;
        return;
    }
    
    const struct device *behavior = zmk_behavior_get_binding(data->current_binding.behavior_dev);
    if (!behavior) {
        data->work_scheduled = false;
        return;
    }
    
    const struct behavior_driver_api *api = behavior->api;
    if (!api || !api->binding_pressed) {
        data->work_scheduled = false;
        return;
    }
    
    api->binding_pressed(&data->current_binding, data->current_event);
}

static void key_release_work_cb(struct k_work *work) {
    struct k_work_delayable *work_delayable = (struct k_work_delayable *)work;
    struct behavior_move_to_keypress_data *data = CONTAINER_OF(work_delayable,
                                                              struct behavior_move_to_keypress_data,
                                                              key_release_work);
    
    if (!zmk_keymap_layer_active(data->active_layer)) {
        data->work_scheduled = false;
        return;
    }
    
    const struct device *behavior = zmk_behavior_get_binding(data->current_binding.behavior_dev);
    if (!behavior) {
        data->work_scheduled = false;
        return;
    }
    
    const struct behavior_driver_api *api = behavior->api;
    if (api && api->binding_released) {
        api->binding_released(&data->current_binding, data->current_event);
    }
    
    data->work_scheduled = false;
}

static void check_and_schedule_movements(const struct behavior_move_to_keypress_config *config,
                                        struct behavior_move_to_keypress_data *data,
                                        struct zmk_behavior_binding_event original_event) {
    bool movement_triggered = false;
    
    if (data->data.x_delta >= config->x_threshold) {
        data->current_binding = config->bindings[0];
        data->current_event = original_event;
        data->data.x_delta -= config->x_threshold;
        movement_triggered = true;
        
        if (config->reset_other_axis) {
            data->data.y_delta = 0;
        }
        
    } else if (data->data.x_delta <= -config->x_threshold) {
        data->current_binding = config->bindings[1];
        data->current_event = original_event;
        data->data.x_delta += config->x_threshold;
        movement_triggered = true;
        
        if (config->reset_other_axis) {
            data->data.y_delta = 0;
        }
    }
    
    if (!movement_triggered) {
        if (data->data.y_delta >= config->y_threshold) {
            data->current_binding = config->bindings[3];
            data->current_event = original_event;
            data->data.y_delta -= config->y_threshold;
            movement_triggered = true;
            
            if (config->reset_other_axis) {
                data->data.x_delta = 0;
            }
            
        } else if (data->data.y_delta <= -config->y_threshold) {
            data->current_binding = config->bindings[2];
            data->current_event = original_event;
            data->data.y_delta += config->y_threshold;
            movement_triggered = true;
            
            if (config->reset_other_axis) {
                data->data.x_delta = 0;
            }
        }
    }
    
    if (movement_triggered && !data->work_scheduled) {
        data->work_scheduled = true;
        data->last_trigger_time = k_uptime_get();
        
        k_work_schedule(&data->key_press_work, K_MSEC(0));
        k_work_schedule(&data->key_release_work, K_MSEC(10));
    }
}

static int move_to_keypress_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_move_to_keypress_data *data = dev->data;
    const struct behavior_move_to_keypress_config *config = dev->config;
    
    struct input_event *evt = (struct input_event *)event.position;
    
    if (evt->type != INPUT_EV_REL) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }
    if (!evt->value) {
        return ZMK_BEHAVIOR_TRANSPARENT;
    }
    
    int64_t current_time = k_uptime_get();
    if (current_time - data->last_trigger_time < config->rate_limit_ms) {
        evt->value = 0;
        return ZMK_BEHAVIOR_OPAQUE;
    }
    
    data->active_layer = event.layer;
    
    handle_rel_code(config, data, evt);
    
    if (data->data.mode == IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_REL) {
        check_and_schedule_movements(config, data, event);
        
        evt->value = 0;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return ZMK_BEHAVIOR_TRANSPARENT;
}

static int move_to_keypress_layer_listener(const zmk_event_t *eh) {
    return ZMK_EV_EVENT_BUBBLE;
}

static int input_behavior_move_to_keypress_init(const struct device *dev) {
    struct behavior_move_to_keypress_data *data = dev->data;
    data->dev = dev;
    data->data.mode = IB_MOVE_TO_KEYPRESS_XY_DATA_MODE_NONE;
    data->data.x_delta = 0;
    data->data.y_delta = 0;
    data->last_trigger_time = 0;
    data->work_scheduled = false;
    data->active_layer = 0;
    
    k_work_init_delayable(&data->key_press_work, key_press_work_cb);
    k_work_init_delayable(&data->key_release_work, key_release_work_cb);
    
    return 0;
}

static const struct behavior_driver_api behavior_move_to_keypress_driver_api = {
    .binding_pressed = move_to_keypress_keymap_binding_pressed,
};

#define MOVE_TO_KEYPRESS_BINDING(idx, node_id) \
    { \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(node_id, bindings, idx)), \
        .param1 = COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, bindings, idx, param1), \
                             (DT_PHA_BY_IDX(node_id, bindings, idx, param1)), (0)), \
        .param2 = COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, bindings, idx, param2), \
                             (DT_PHA_BY_IDX(node_id, bindings, idx, param2)), (0)), \
    }

#define MTKLP_INST(n)                                                                       \
    static struct behavior_move_to_keypress_data behavior_move_to_keypress_data_##n = {};   \
    static struct behavior_move_to_keypress_config behavior_move_to_keypress_config_##n = { \
        .threshold = DT_INST_PROP(n, threshold),                                            \
        .x_threshold = DT_INST_PROP_OR(n, x_threshold, DT_INST_PROP(n, threshold)),         \
        .y_threshold = DT_INST_PROP_OR(n, y_threshold, DT_INST_PROP(n, threshold)),         \
        .rate_limit_ms = DT_INST_PROP_OR(n, rate_limit_ms, 50),                             \
        .x_invert = DT_INST_PROP_OR(n, x_invert, false),                                    \
        .y_invert = DT_INST_PROP_OR(n, y_invert, false),                                    \
        .reset_other_axis = DT_INST_PROP_OR(n, reset_other_axis, false),                    \
        .bindings = {                                                                       \
            MOVE_TO_KEYPRESS_BINDING(0, DT_DRV_INST(n)), /* RIGHT */                       \
            MOVE_TO_KEYPRESS_BINDING(1, DT_DRV_INST(n)), /* LEFT */                        \
            MOVE_TO_KEYPRESS_BINDING(2, DT_DRV_INST(n)), /* UP */                          \
            MOVE_TO_KEYPRESS_BINDING(3, DT_DRV_INST(n)), /* DOWN */                        \
        },                                                                                  \
    };                                                                                      \
    BEHAVIOR_DT_INST_DEFINE(n, input_behavior_move_to_keypress_init, NULL,                  \
                            &behavior_move_to_keypress_data_##n,                            \
                            &behavior_move_to_keypress_config_##n,                          \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,               \
                            &behavior_move_to_keypress_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MTKLP_INST)

ZMK_LISTENER(move_to_keypress_layer, move_to_keypress_layer_listener);
ZMK_SUBSCRIPTION(move_to_keypress_layer, zmk_layer_state_changed);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */ 