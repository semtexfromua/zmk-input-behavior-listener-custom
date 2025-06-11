# ZMK Input Behavior Sensor Rotate

Новий behavior для ZMK, який перетворює рух трекболу в натискання клавіш стрілок (або будь-яких інших клавіш).

## Особливості

- ✅ **Накопичення руху** - збирає delta значення до порогу
- ✅ **Налаштовуваний поріг** - через параметр `threshold`
- ✅ **Інверсія осей** - підтримка `x-invert` та `y-invert`
- ✅ **Гнучкі bindings** - будь-які клавіші (стрілки, WASD, HJKL)
- ✅ **Точне позиціонування** - ідеально для редакторів коду

## Використання

### 1. Визначення behavior

```dts
ib_move_to_arrow: ib_move_to_arrow {
    compatible = "zmk,input-behavior-sensor-rotate";
    #sensor-binding-cells = <4>;
    threshold = <30>; // Поріг чутливості
    // x-invert; // Інверсія X (розкоментувати при потребі)
    // y-invert; // Інверсія Y (розкоментувати при потребі)
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### 2. Підключення до listener

```dts
trackball_arrow_listener {
    compatible = "zmk,input-behavior-listener";
    device = <&vtrackball>;
    layers = <2>; // Активний тільки на шарі 2
    evt-type = <INPUT_EV_REL>;
    x-input-code = <INPUT_REL_X>;
    y-input-code = <INPUT_REL_Y>;
    scale-divisor = <1>;
    bindings = <&ib_move_to_arrow>;
};
```

### 3. Альтернативні варіанти

**WASD навігація:**
```dts
ib_move_to_wasd: ib_move_to_wasd {
    compatible = "zmk,input-behavior-sensor-rotate";
    #sensor-binding-cells = <4>;
    threshold = <50>;
    bindings = <&kp D>, <&kp A>, <&kp W>, <&kp S>;
};
```

**Vim навігація:**
```dts
ib_move_to_vim: ib_move_to_vim {
    compatible = "zmk,input-behavior-sensor-rotate";
    #sensor-binding-cells = <4>;
    threshold = <40>;
    bindings = <&kp L>, <&kp H>, <&kp K>, <&kp J>;
};
```

## Параметри

| Параметр | Тип | За замовчуванням | Опис |
|----------|-----|------------------|------|
| `threshold` | int | 50 | Поріг накопичення для генерації події |
| `x-invert` | boolean | false | Інверсія напрямку X осі |
| `y-invert` | boolean | false | Інверсія напрямку Y осі |
| `bindings` | phandle-array | обов'язковий | Bindings для: право, ліво, вгору, вниз |

## Логіка роботи

1. **Накопичення**: behavior накопичує delta значення по X/Y осям
2. **Перевірка порогу**: при досягненні `threshold` генерується key event
3. **Віднімання порогу**: з delta віднімається `threshold`, залишок зберігається
4. **Повторення**: процес продовжується для плавної навігації

## Приклад keymap

```dts
#include <behaviors.dtsi>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/input-event-codes.h>

/ {
    trackball_arrow_listener {
        compatible = "zmk,input-behavior-listener";
        device = <&vtrackball>;
        layers = <2>;
        evt-type = <INPUT_EV_REL>;
        x-input-code = <INPUT_REL_X>;
        y-input-code = <INPUT_REL_Y>;
        bindings = <&ib_move_to_arrow>;
    };

    ib_move_to_arrow: ib_move_to_arrow {
        compatible = "zmk,input-behavior-sensor-rotate";
        #sensor-binding-cells = <4>;
        threshold = <30>;
        bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
    };

    keymap {
        compatible = "zmk,keymap";
        
        default_layer {
            bindings = <
                &mo 2    // Перехід на arrow layer
                // ... інші клавіші
            >;
        };
        
        arrow_layer { // Layer 2
            bindings = <
                &trans
                // Тут трекбол працює як стрілки
            >;
        };
    };
};
``` 