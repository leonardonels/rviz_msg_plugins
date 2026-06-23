# mmr_rviz_plugins

Custom **RViz2 display plugins** for MMR Driverless messages (ROS 2 Humble,
`ament_cmake`).

This package ships one display today and is structured to make adding more
straightforward. If you want to author a new display, read
**[docs/ADDING_A_PLUGIN.md](docs/ADDING_A_PLUGIN.md)** — the existing
`SpeedProfileDisplay` is heavily commented to serve as the worked example.

## Displays in this package

| Display (RViz name)            | Message                              | What it draws |
| ------------------------------ | ------------------------------------ | ------------- |
| `mmr_rviz_plugins/SpeedProfile` | `mmr_base/msg/SpeedProfilePoints`    | One sphere per waypoint colored by speed (blue = slow, red = fast), connected by a path line. |

## Build

```bash
cd ~/ros2_ws
colcon build --packages-select mmr_rviz_plugins
source install/setup.bash
```

`mmr_base` must be built/installed first (it provides the message). RViz, Qt5
and the `rviz_*` libraries come from the ROS Humble desktop install.

## Run

```bash
rviz2
```

Then in RViz:

1. **Add** (bottom-left of the Displays panel) → **By display type** →
   `mmr_rviz_plugins` → **SpeedProfile** → OK.
2. Set the display's **Topic** to where your profile is published, e.g.
   `/planning/speedProfilePoints`.
3. Set **Reference Frame** to the frame the points are expressed in (e.g. `map`
   or `odom`). `SpeedProfilePoints` has **no Header**, so RViz cannot infer the
   frame — you choose it here. Make sure RViz's **Fixed Frame** (Global Options)
   can be connected to it via TF.

### Display properties

| Property          | Default | Meaning |
| ----------------- | ------- | ------- |
| Topic             | —       | The `SpeedProfilePoints` topic to subscribe to. |
| Reference Frame   | `map`   | TF frame the point coordinates are in. |
| Sphere Radius     | 0.15 m  | Size of the marker at each waypoint. |
| Line Width        | 0.05 m  | Width of the connecting path line. |
| Max Speed         | 20 m/s  | Speed mapped to full red; normalizes the color gradient. |
| Show Path Line    | true    | Toggle the connecting line. |

