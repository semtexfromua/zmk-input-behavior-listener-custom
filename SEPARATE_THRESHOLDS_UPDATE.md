# 🎯 Update: Separate X/Y Thresholds Support

## ✨ New Features

Added support for **separate threshold values** for X and Y axes in the `move_to_keypress` behavior, while maintaining full backward compatibility.

## 🔧 Configuration Options

### Basic Configuration (Backward Compatible)
```c
ib_arrow_nav: ib_arrow_nav {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <60>;  // Used for both X and Y axes
    rate-limit-ms = <50>;
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### Advanced Configuration (Separate Thresholds)
```c
ib_arrow_nav: ib_arrow_nav {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <50>;      // Default threshold (fallback)
    x-threshold = <80>;    // Specific threshold for horizontal movement
    y-threshold = <40>;    // Specific threshold for vertical movement
    rate-limit-ms = <50>;
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### Partial Configuration (Mixed)
```c
ib_arrow_nav: ib_arrow_nav {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <60>;      // Used as default
    y-threshold = <30>;    // Override only Y axis
    // X axis will use threshold = 60
    rate-limit-ms = <50>;
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

## 📋 Property Definitions

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `threshold` | int | 50 | Default threshold for both axes |
| `x-threshold` | int | threshold | Override threshold for X axis (horizontal) |
| `y-threshold` | int | threshold | Override threshold for Y axis (vertical) |
| `rate-limit-ms` | int | 50 | Minimum interval between key events |
| `x-invert` | boolean | false | Invert X axis direction |
| `y-invert` | boolean | false | Invert Y axis direction |
| `bindings` | phandle-array | required | RIGHT, LEFT, UP, DOWN key bindings |

## 🔄 Fallback Logic

1. If `x-threshold` is specified → use it for X axis
2. If `x-threshold` is NOT specified → use `threshold` for X axis
3. If `y-threshold` is specified → use it for Y axis  
4. If `y-threshold` is NOT specified → use `threshold` for Y axis

## 🎮 Use Cases

### Gaming Configuration
```c
// More sensitive vertical movement for quick aim adjustments
// Less sensitive horizontal movement for stability
trackball_gaming: trackball_gaming {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <60>;      // Default
    x-threshold = <80>;    // Less sensitive horizontal (more stable)
    y-threshold = <40>;    // More sensitive vertical (quicker aim)
    rate-limit-ms = <30>;  // Faster response
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### Text Editing Configuration
```c
// More sensitive horizontal movement for character navigation
// Less sensitive vertical movement for line stability
trackball_editing: trackball_editing {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <50>;      // Default
    x-threshold = <30>;    // More sensitive horizontal (fast char navigation)
    y-threshold = <70>;    // Less sensitive vertical (stable line navigation)
    rate-limit-ms = <50>;
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

## ⚡ Technical Implementation

- **Overflow Protection**: Each axis uses its own threshold multiplier (threshold × 3)
- **Independent Processing**: X and Y movements are evaluated independently
- **Work Queue Stability**: All existing safety mechanisms preserved
- **Memory Efficiency**: Minimal overhead (2 additional int16_t fields)

## ✅ Backward Compatibility

- ✅ All existing configurations work unchanged
- ✅ Single `threshold` parameter still supported
- ✅ No breaking changes to API or behavior
- ✅ Default values maintain original behavior

## 🚀 Migration Guide

### From Single Threshold
```c
// OLD
threshold = <60>;

// NEW (same behavior)
threshold = <60>;  // Both axes use 60

// NEW (customized)
threshold = <60>;    // Fallback
x-threshold = <80>;  // Horizontal uses 80
y-threshold = <40>;  // Vertical uses 40
```

This update provides fine-grained control over trackball sensitivity while maintaining the rock-solid stability of the Work Queue implementation. 