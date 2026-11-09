# rviz_msg_plugins

Custom **RViz2 display plugins** for custom messages (ROS 2 Humble,
`ament_cmake`).

This package ships one display today and is structured to make adding more
straightforward. If you want to author a new display, read
**[docs/ADDING_A_PLUGIN.md](docs/ADDING_A_PLUGIN.md)** — the existing
`LivoxCustomMsgDisplay` (`src/livox_custom_msg_display.cpp`) is commented to
serve as a worked example.

## Displays in this package

| Display (RViz name)               | Message                           | What it draws |
| --------------------------------- | --------------------------------- | ------------- |
| `rviz_msg_plugins/LivoxCustomMsg` | `livox_ros_driver2/msg/CustomMsg` | The Livox point cloud, converted to `PointCloud2` inside RViz and rendered with the stock point-cloud renderer (all Style / Size / Color Transformer options). |

## Build

```bash
cd ~/fincantieri/livox
colcon build --packages-select rviz_msg_plugins
source install/setup.bash
```

`livox_ros_driver2` must be built/installed first (it provides the message).
RViz, Qt5 and the `rviz_*` libraries come from the ROS Humble desktop install.

## Run

```bash
rviz2
```

Then in RViz:

1. **Add** (bottom-left of the Displays panel) → **By topic** → `/livox/lidar`
   → **LivoxCustomMsg**, or **By display type** → `rviz_msg_plugins` →
   **LivoxCustomMsg** and set the **Topic**.
2. Set **Fixed Frame** (Global Options) to `livox_frame` (the driver's
   `frame_id`), or to any frame TF can connect to it.

### Display properties

The message has a Header, so the frame comes from `header.frame_id`; there is no
frame property. The usual point-cloud properties are provided by RViz:

| Property          | Meaning |
| ----------------- | ------- |
| Topic             | The `CustomMsg` topic, e.g. `/livox/lidar`. |
| Style / Size      | Points, Squares, Spheres, Boxes, ... and their size. |
| Decay Time        | Keep past frames on screen (useful: Livox scans are non-repetitive, try 0.5–1 s). |
| Color Transformer | Color by `intensity` (reflectivity), `tag`, `line`, `offset_time`, or by axis. |
