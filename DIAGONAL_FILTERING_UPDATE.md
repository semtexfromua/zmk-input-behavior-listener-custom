# 🎯 Update: Diagonal Movement Filtering

## ✨ New Feature: `reset-other-axis`

Added optional **diagonal movement filtering** to prevent accidental axis jumping when moving trackball primarily in one direction.

## 🎮 Problem Solved

When moving a trackball in a mostly straight line (e.g., horizontally), small imperfections in movement can cause unwanted activation of the perpendicular axis. This creates a "stepping" effect instead of smooth directional movement.

## 🔧 Solution: Axis Reset

New boolean parameter `reset-other-axis` that when enabled:
- **Resets opposite axis delta to 0** when any movement is triggered
- Ensures clean directional movement by eliminating cross-axis noise
- Maintains all existing functionality when disabled (default)

## 📋 Configuration Examples

### Default Behavior (No Filtering)
```c
ib_arrow_nav: ib_arrow_nav {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <50>;
    x-threshold = <80>;
    y-threshold = <40>;
    rate-limit-ms = <50>;
    // reset-other-axis = false; (default)
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### With Diagonal Filtering
```c
ib_arrow_nav: ib_arrow_nav {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <50>;
    x-threshold = <80>;
    y-threshold = <40>;
    rate-limit-ms = <50>;
    reset-other-axis;  // Enable diagonal filtering
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

## 🔄 How It Works

### Without `reset-other-axis` (default):
1. X delta accumulates: `x_delta = 85`
2. Y delta accumulates: `y_delta = 15` (noise)
3. X threshold reached (80) → RIGHT key triggered
4. **Y delta remains: `y_delta = 15`** (continues accumulating)
5. More movement → Y might trigger unexpectedly

### With `reset-other-axis` enabled:
1. X delta accumulates: `x_delta = 85`
2. Y delta accumulates: `y_delta = 15` (noise)  
3. X threshold reached (80) → RIGHT key triggered
4. **Y delta reset: `y_delta = 0`** ✨ (noise eliminated)
5. Clean directional movement continues

## 🎯 Use Cases

### Gaming Configuration
```c
// Precise directional movement for gaming
trackball_gaming: trackball_gaming {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <60>;
    x-threshold = <80>;
    y-threshold = <40>;
    rate-limit-ms = <30>;
    reset-other-axis;  // Clean directional movement
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

### Text Navigation
```c
// Smooth text navigation without axis jumping
trackball_text: trackball_text {
    compatible = "zmk,input-behavior-move-to-keypress";
    #binding-cells = <0>;
    threshold = <50>;
    x-threshold = <30>;    // Sensitive horizontal
    y-threshold = <70>;    // Less sensitive vertical
    rate-limit-ms = <50>;
    reset-other-axis;      // Prevent line jumping during horizontal navigation
    bindings = <&kp RIGHT>, <&kp LEFT>, <&kp UP>, <&kp DOWN>;
};
```

## ⚡ Technical Implementation

### Filtering Logic
- **X Movement Triggered** → `y_delta = 0` (if enabled)
- **Y Movement Triggered** → `x_delta = 0` (if enabled)
- **No Movement** → No delta reset occurs
- **Movement Priority** → X axis checked first, then Y axis

### Safety Mechanisms
- ✅ Only applies when movement actually triggers
- ✅ Preserves all existing threshold logic
- ✅ No impact on performance
- ✅ Fully backward compatible (default: disabled)

## 🔒 Backward Compatibility

- ✅ **Default behavior unchanged** (`reset-other-axis = false`)
- ✅ **All existing configurations work** without modification
- ✅ **No breaking changes** to API or core functionality
- ✅ **Optional feature** - enable only when needed

## 🚀 Migration Guide

### Enable Diagonal Filtering
```c
// Add this line to existing configuration
reset-other-axis;

// Or explicitly
reset-other-axis = <true>;
```

### Keep Current Behavior
```c
// No changes needed - default is disabled
// Or explicitly disable
reset-other-axis = <false>;
```

This feature provides cleaner directional control while maintaining the rock-solid stability of the Work Queue implementation. 