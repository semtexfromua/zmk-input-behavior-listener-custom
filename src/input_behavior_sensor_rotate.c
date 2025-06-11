/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_behavior_sensor_rotate

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

enum sensor_rotate_xy_data_mode {
    IB_SENSOR_ROTATE_XY_DATA_MODE_NONE,
    IB_SENSOR_ROTATE_XY_DATA_MODE_REL,
    IB_SENSOR_ROTATE_XY_DATA_MODE_ABS,
};

struct sensor_rotate_xy_data {
    enum sensor_rotate_xy_data_mode mode;
    int16_t x_delta;
    int16_t y_delta;
};

struct behavior_sensor_rotate_data {
    const struct device *dev;
    struct sensor_rotate_xy_data data;
};

struct behavior_sensor_rotate_config {
    int16_t threshold;
    bool x_invert;
    bool y_invert;
    struct zmk_behavior_binding bindings[4]; // right, left, up, down
};

static void handle_rel_code(const struct behavior_sensor_rotate_config *config,
                            struct behavior_sensor_rotate_data *data, struct input_event *evt) {
    // Listener вже відфільтрував події, обробляємо X та Y
    switch (evt->code) {
    case INPUT_REL_X:
        data->data.mode = IB_SENSOR_ROTATE_XY_DATA_MODE_REL;
        int16_t x_val = evt->value;
        if (config->x_invert) {
            x_val = -x_val;
        }
        data->data.x_delta += x_val;
        break;
    case INPUT_REL_Y:
        data->data.mode = IB_SENSOR_ROTATE_XY_DATA_MODE_REL;
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

static void check_and_trigger_movements(const struct behavior_sensor_rotate_config *config,
                                       struct behavior_sensor_rotate_data *data) {
    bool triggered = false;

    // Check X axis movement (left/right)
    if (data->data.x_delta >= config->threshold) {
        // Move right
        trigger_key_press(&config->bindings[0]); // right binding
        data->data.x_delta -= config->threshold;
        triggered = true;
        LOG_DBG("Triggered RIGHT movement, remaining delta: %d", data->data.x_delta);
    } else if (data->data.x_delta <= -config->threshold) {
        // Move left  
        trigger_key_press(&config->bindings[1]); // left binding
        data->data.x_delta += config->threshold;
        triggered = true;
        LOG_DBG("Triggered LEFT movement, remaining delta: %d", data->data.x_delta);
    }

    // Check Y axis movement (up/down)
    if (data->data.y_delta >= config->threshold) {
        // Move down
        trigger_key_press(&config->bindings[3]); // down binding
        data->data.y_delta -= config->threshold;
        triggered = true;
        LOG_DBG("Triggered DOWN movement, remaining delta: %d", data->data.y_delta);
    } else if (data->data.y_delta <= -config->threshold) {
        // Move up
        trigger_key_press(&config->bindings[2]); // up binding  
        data->data.y_delta += config->threshold;
        triggered = true;
        LOG_DBG("Triggered UP movement, remaining delta: %d", data->data.y_delta);
    }
}

static int sensor_rotate_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_sensor_rotate_data *data = 
        (struct behavior_sensor_rotate_data *)dev->data;
    const struct behavior_sensor_rotate_config *config = dev->config;
    
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

    if (data->data.mode == IB_SENSOR_ROTATE_XY_DATA_MODE_REL) {
        check_and_trigger_movements(config, data);
        
        // Always consume the event since we're handling trackball input
        evt->value = 0;
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return ZMK_BEHAVIOR_TRANSPARENT;
}

static int input_behavior_sensor_rotate_init(const struct device *dev) {
    struct behavior_sensor_rotate_data *data = dev->data;
    data->dev = dev;
    data->data.mode = IB_SENSOR_ROTATE_XY_DATA_MODE_NONE;
    data->data.x_delta = 0;
    data->data.y_delta = 0;
    return 0;
};

static const struct behavior_driver_api behavior_sensor_rotate_driver_api = {
    .binding_pressed = sensor_rotate_keymap_binding_pressed,
};

#define SENSOR_BINDING(idx, node_id) \
    { \
        .behavior_dev = DEVICE_DT_NAME(DT_PHANDLE_BY_IDX(node_id, bindings, idx)), \
        .param1 = COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, bindings, idx, param1), \
                             (DT_PHA_BY_IDX(node_id, bindings, idx, param1)), (0)), \
        .param2 = COND_CODE_1(DT_PHA_HAS_CELL_AT_IDX(node_id, bindings, idx, param2), \
                             (DT_PHA_BY_IDX(node_id, bindings, idx, param2)), (0)), \
    }

#define IBSR_INST(n)                                                                                \
    static struct behavior_sensor_rotate_data behavior_sensor_rotate_data_##n = {};                \
    static struct behavior_sensor_rotate_config behavior_sensor_rotate_config_##n = {              \
        .threshold = DT_INST_PROP(n, threshold),                                                   \
        .x_invert = DT_INST_PROP_OR(n, x_invert, false),                                           \
        .y_invert = DT_INST_PROP_OR(n, y_invert, false),                                           \
        .bindings = {                                                                              \
            SENSOR_BINDING(0, DT_DRV_INST(n)), /* right */                                        \
            SENSOR_BINDING(1, DT_DRV_INST(n)), /* left */                                         \
            SENSOR_BINDING(2, DT_DRV_INST(n)), /* up */                                           \
            SENSOR_BINDING(3, DT_DRV_INST(n)), /* down */                                         \
        },                                                                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, input_behavior_sensor_rotate_init, NULL,                            \
                            &behavior_sensor_rotate_data_##n,                                      \
                            &behavior_sensor_rotate_config_##n,                                    \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                      \
                            &behavior_sensor_rotate_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IBSR_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */ 