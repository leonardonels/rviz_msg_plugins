// Copyright 2026 MMR Driverless
//
// Custom RViz2 display for mmr_base/msg/SpeedProfilePoints.
//
// This file is intentionally over-commented: it doubles as a worked example for
// authoring new RViz displays. See docs/ADDING_A_PLUGIN.md for the full guide.

#ifndef MMR_RVIZ_PLUGINS__SPEED_PROFILE_DISPLAY_HPP_
#define MMR_RVIZ_PLUGINS__SPEED_PROFILE_DISPLAY_HPP_

#include <memory>
#include <vector>

#include "rviz_common/ros_topic_display.hpp"

#include "mmr_base/msg/speed_profile_points.hpp"

// Forward declarations keep the header light: we only need pointers here, so we
// avoid pulling the full rviz_rendering / properties headers into every
// translation unit that includes us. The full includes live in the .cpp.
namespace rviz_rendering
{
class Shape;
class BillboardLine;
}  // namespace rviz_rendering

namespace rviz_common
{
namespace properties
{
class FloatProperty;
class BoolProperty;
class TfFrameProperty;
}  // namespace properties
}  // namespace rviz_common

namespace mmr_rviz_plugins
{

// We derive from RosTopicDisplay (NOT MessageFilterDisplay) because
// SpeedProfilePoints carries no std_msgs/Header. RosTopicDisplay gives us the
// topic dropdown, the subscription lifecycle, and the processMessage() hook,
// but leaves frame resolution to us — which is exactly why we add our own
// "Reference Frame" property below.
class SpeedProfileDisplay
  : public rviz_common::RosTopicDisplay<mmr_base::msg::SpeedProfilePoints>
{
  Q_OBJECT

public:
  SpeedProfileDisplay();
  ~SpeedProfileDisplay() override;

protected:
  // Called once, after scene_manager_/scene_node_/context_ are valid.
  void onInitialize() override;

  // Called on "Reset" or when the topic changes: drop all rendered geometry.
  void reset() override;

  // Called for every incoming message: rebuild the scene to match it.
  void processMessage(mmr_base::msg::SpeedProfilePoints::ConstSharedPtr msg) override;

private Q_SLOTS:
  // Re-render the last message when a style property (radius, width, color
  // scale, frame) changes, so edits take effect without waiting for a new msg.
  void updateStyle();

private:
  // Tear down the spheres + path line without resetting message counters.
  void clearVisuals();

  // The per-message rendering, factored out so updateStyle() can reuse it.
  void render(const mmr_base::msg::SpeedProfilePoints & msg);

  // One sphere per waypoint.
  std::vector<std::shared_ptr<rviz_rendering::Shape>> spheres_;

  // A single poly-line connecting the waypoints in order.
  std::shared_ptr<rviz_rendering::BillboardLine> path_line_;

  // Cached copy of the most recent message, so style edits can re-render.
  mmr_base::msg::SpeedProfilePoints::ConstSharedPtr last_msg_;

  // User-tunable properties (shown in the RViz Displays panel).
  rviz_common::properties::TfFrameProperty * frame_property_;
  rviz_common::properties::FloatProperty * radius_property_;
  rviz_common::properties::FloatProperty * line_width_property_;
  rviz_common::properties::FloatProperty * max_speed_property_;
  rviz_common::properties::BoolProperty * show_path_property_;
};

}  // namespace mmr_rviz_plugins

#endif  // MMR_RVIZ_PLUGINS__SPEED_PROFILE_DISPLAY_HPP_
