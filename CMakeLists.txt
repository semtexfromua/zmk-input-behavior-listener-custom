zephyr_library()

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

  zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_BEHAVIOR_LISTENER src/input_behavior_listener.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_BEHAVIOR_SCALER src/input_behavior_scaler.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_BEHAVIOR_TOG_LAYER src/input_behavior_tog_layer.c)
  zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_BEHAVIOR_MOVE_TO_KEYPRESS src/input_behavior_move_to_keypress.c)

  zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
endif() 