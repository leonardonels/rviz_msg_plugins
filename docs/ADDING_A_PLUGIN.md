# Authoring a new RViz2 display plugin

This guide explains how RViz custom displays work and walks through adding a new
one to this package for **any** message type. The shipped `SpeedProfileDisplay`
is the running example; read its source alongside this document —
[`include/mmr_rviz_plugins/speed_profile_display.hpp`](../include/mmr_rviz_plugins/speed_profile_display.hpp)
and [`src/speed_profile_display.cpp`](../src/speed_profile_display.cpp) are
commented to match the sections below.

---

## 1. The mental model

An RViz **display** owns a piece of OGRE's 3D scene graph — a `scene_node_` —
and its one job is to keep that sub-scene matching the latest ROS message it
received. RViz drives the OpenGL render loop; you only ever describe *what*
should be in the scene.

```
ROS topic ──► subscription ──► processMessage(msg) ──► create/update OGRE objects
                                                          under scene_node_
                                              RViz render loop ──► pixels
```

You subclass an RViz base class that handles the boilerplate (topic dropdown,
subscription lifecycle, frame transforms) and you override a few hooks:

| Hook                       | When it runs                  | What you do |
| -------------------------- | ----------------------------- | ----------- |
| `onInitialize()`           | once, after the scene exists  | create reusable visuals / wire up properties that need `context_` |
| `processMessage(msg)`      | per incoming message          | translate the message into OGRE objects |
| `reset()`                  | on "Reset" / topic change     | delete all visuals |

You get these member variables for free from the base class:

- `scene_manager_` — the OGRE scene manager (factory for objects).
- `scene_node_` — your node in the scene graph; parent all your visuals to it.
- `context_` — access to the frame manager, render queue, ROS node, etc.

---

## 2. Choose your base class — this is the key decision

Which base class you derive from depends on **whether your message has a
`std_msgs/Header`** (i.e. a `frame_id` + `stamp`).

### Case A — message HAS a Header → `MessageFilterDisplay<T>`

This is the common, idiomatic case (e.g. `PointCloud2`, `PoseStamped`,
`Path`...). `rviz_common::MessageFilterDisplay<T>` uses a `tf2_ros::MessageFilter`
to **defer** each message until its `frame_id` can be transformed into the fixed
frame at its `stamp`. By the time your `processMessage` runs, the transform is
guaranteed available. In `processMessage`, resolve the pose with:

```cpp
Ogre::Vector3 position;
Ogre::Quaternion orientation;
if (!context_->getFrameManager()->getTransform(msg->header, position, orientation)) {
  setMissingTransformToFixedFrame(msg->header.frame_id);
  return;
}
scene_node_->setPosition(position);
scene_node_->setOrientation(orientation);
```

### Case B — message has NO Header → `RosTopicDisplay<T>`

`SpeedProfilePoints` is this case: it carries only points, no frame or stamp.
`MessageFilterDisplay` **cannot** be used (it requires a header to filter on).
Instead derive from `rviz_common::RosTopicDisplay<T>`, which still gives you the
topic dropdown, subscription, and `processMessage`, but leaves frame handling to
you. Add a **frame property** so the user states what frame the data is in, and
resolve the latest transform yourself:

```cpp
const std::string frame = frame_property_->getFrameStd();
Ogre::Vector3 position;
Ogre::Quaternion orientation;
if (!context_->getFrameManager()->getTransform(frame, position, orientation)) {
  setMissingTransformToFixedFrame(frame);   // no stamp ⇒ "latest" overload
  return;
}
```

> **Best practice:** if you control the message and it represents something
> spatial, add a `std_msgs/Header` to it and use Case A. Case B exists for
> messages you can't change. `SpeedProfilePoints` chose Case B deliberately to
> avoid editing the shared `mmr_base` interface and its publisher.

There is also a third, lower-level base — `rviz_common::Display` — when you
aren't driven by a single topic (e.g. a tool, or a display that subscribes to
several topics). You then manage your own subscription.

---

## 3. Files to add for a new display

For a new display called `FooDisplay` rendering `my_msgs/msg/Foo`:

1. `include/mmr_rviz_plugins/foo_display.hpp` — the class declaration.
2. `src/foo_display.cpp` — the implementation + `PLUGINLIB_EXPORT_CLASS`.
3. Add a `<class>` entry to `plugins_description.xml`.
4. Add the `.cpp` to `add_library(...)` in `CMakeLists.txt`, and `find_package`
   + `ament_target_dependencies` for any new message package.
5. Add the new message package to `package.xml` as a `<depend>`.

You do **not** create a new `.so` per display — all displays in this package
compile into the single `libmmr_rviz_plugins.so`. Just list each `.cpp`.

---

## 4. Anatomy of the class

### Header (`*.hpp`)

```cpp
class FooDisplay : public rviz_common::RosTopicDisplay<my_msgs::msg::Foo>
{
  Q_OBJECT                       // needed: this class has Qt slots
public:
  FooDisplay();
  ~FooDisplay() override;
protected:
  void onInitialize() override;
  void reset() override;
  void processMessage(my_msgs::msg::Foo::ConstSharedPtr msg) override;
private Q_SLOTS:
  void updateStyle();            // re-render when a property changes
private:
  // ... visuals (Shape/BillboardLine/...) and Property* members ...
};
```

Notes:

- `RosTopicDisplay<T>` (and `MessageFilterDisplay<T>`) are **templates**, so moc
  cannot put `Q_OBJECT` on them — but your concrete subclass **must** have
  `Q_OBJECT` if it defines slots. The base provides the `RTDClass` /
  `MFDClass` typedef as a short name for the templated parent.
- Forward-declare `rviz_rendering` / property types in the header and include
  the full headers only in the `.cpp` to keep compiles fast.

### Constructor — declare properties

Properties are the rows users edit in the Displays panel. Create them in the
constructor and connect their `changed()` signal to a slot so edits take effect:

```cpp
radius_property_ = new rviz_common::properties::FloatProperty(
  "Sphere Radius", 0.15f, "Radius of each marker (m).",
  this, SLOT(updateStyle()));
radius_property_->setMin(0.0f);
```

Common property types: `FloatProperty`, `IntProperty`, `BoolProperty`,
`ColorProperty`, `EnumProperty`, `StringProperty`, `TfFrameProperty`,
`RosTopicProperty`. A `TfFrameProperty` needs its frame manager set in
`onInitialize` (not the constructor), because `context_` isn't valid yet:

```cpp
void FooDisplay::onInitialize() {
  RTDClass::onInitialize();      // ALWAYS call the base first
  frame_property_->setFrameManager(context_->getFrameManager());
}
```

### processMessage — build the scene

1. Resolve the transform (Case A or B above) and position `scene_node_`.
2. Rebuild or update your OGRE visuals to match `msg`.
3. Call `context_->queueRender()` so the change paints immediately.

Useful `rviz_rendering` objects (parent them to `scene_node_`):

- `rviz_rendering::Shape` — sphere / cube / cylinder / cone / arrow.
- `rviz_rendering::BillboardLine` — thick poly-lines.
- `rviz_rendering::PointCloud` — many points efficiently.
- `rviz_rendering::Arrow`, `Axes`, `MovableText`, ...

> **Performance note:** the example rebuilds all `Shape`s each message, which is
> fine for a trajectory. For high-rate or high-count data, pool and reuse
> objects, or use `PointCloud`, instead of allocating per message.

### reset / destructor — tidy up

`reset()` should call the base `reset()` and then drop your visuals. Holding
visuals in `std::shared_ptr` (or a `std::vector` of them) makes teardown a
`clear()`. The base class owns and destroys `scene_node_` itself.

---

## 5. Register the plugin

### `plugins_description.xml`

```xml
<library path="mmr_rviz_plugins">
  <class
    name="mmr_rviz_plugins/Foo"
    type="mmr_rviz_plugins::FooDisplay"
    base_class_type="rviz_common::Display">
    <description>What it visualizes.</description>
    <message_type>my_msgs/msg/Foo</message_type>
  </class>
</library>
```

- `type` must **exactly** match the `PLUGINLIB_EXPORT_CLASS` in your `.cpp`.
- `base_class_type` is always `rviz_common::Display` for a display.
- `name` is what RViz shows under **Add → By display type**; the text before the
  `/` is the group heading.

### `.cpp` footer

```cpp
#include <pluginlib/class_list_macros.hpp>
PLUGINLIB_EXPORT_CLASS(mmr_rviz_plugins::FooDisplay, rviz_common::Display)
```

### `CMakeLists.txt`

```cmake
add_library(${PROJECT_NAME} SHARED
  src/speed_profile_display.cpp
  include/mmr_rviz_plugins/speed_profile_display.hpp   # existing
  src/foo_display.cpp                                  # <-- add
  include/mmr_rviz_plugins/foo_display.hpp             # <-- add the header too!
)
# if my_msgs is new:
find_package(my_msgs REQUIRED)
ament_target_dependencies(${PROJECT_NAME} ... my_msgs)
```

`set(CMAKE_AUTOMOC ON)` (already in this package) runs moc on your `Q_OBJECT`
headers — **but only if it scans them.** AUTOMOC auto-scans a header only when
it sits next to a `.cpp` of the same basename. Our headers live under
`include/`, a different directory, so you **must list each `Q_OBJECT` header
explicitly in `add_library`** (as shown above). Forget this and moc never runs
for that class: the build succeeds, but RViz fails to `dlopen` the library with
`undefined symbol: ...YourDisplay...` (a missing vtable). `pluginlib_export_plugin_description_file(rviz_common
plugins_description.xml)` (already present) exposes the manifest to RViz.

### `package.xml`

Add `<depend>my_msgs</depend>` if you introduced a new message package.

---

## 6. Build, run, verify

```bash
cd ~/ros2_ws
colcon build --packages-select mmr_rviz_plugins
source install/setup.bash
rviz2
```

In RViz: **Add → By display type → mmr_rviz_plugins → Foo**, set the **Topic**
(and **Reference Frame** for a Case-B display). Publish a test message (see the
README's smoke-test snippet) and confirm geometry appears and the display's
**Status** row is green.

### Common pitfalls

- **Nothing in the Add dialog** → manifest not found. Check the `<rviz
  plugin=...>` export in `package.xml`, that the install step copies
  `plugins_description.xml`, and that you sourced `install/setup.bash`.
- **`Failed to load library` / undefined symbol** → the `type=` string in the
  manifest doesn't match `PLUGINLIB_EXPORT_CLASS`, or a `.cpp` wasn't added to
  `add_library`.
- **Crash on add** → you forgot `RTDClass::onInitialize()` (base call) before
  touching `context_` / `scene_node_`.
- **Properties don't apply until next message** → connect each property's
  `changed()` signal to a slot, cache the last message, and re-render there
  (see `updateStyle()` in the example).
- **Points invisible / "transform" error** → Fixed Frame and the data's frame
  aren't connected in TF, or (Case B) the Reference Frame is wrong.
- **`dlopen` `undefined symbol: ...YourDisplay` (missing vtable)** → moc didn't
  run for your header. Add the `Q_OBJECT` header to `add_library` (see §5) —
  AUTOMOC won't find headers under `include/` on its own. Confirm with
  `find build/<pkg> -name 'moc_*.cpp'`.
- **`Q_OBJECT` link errors** → missing `set(CMAKE_AUTOMOC ON)`, or `Q_OBJECT`
  placed on the templated base instead of your concrete subclass.

---

## 7. Reference

- RViz source (Humble): https://github.com/ros2/rviz/tree/humble
- Headers on this machine: `/opt/ros/humble/include/rviz_common/` and
  `/opt/ros/humble/include/rviz_rendering/` — grep these for exact signatures.
- Good built-in examples to copy from:
  `rviz_default_plugins` displays (`path_display`, `pose_display`,
  `point_cloud2_display`).
