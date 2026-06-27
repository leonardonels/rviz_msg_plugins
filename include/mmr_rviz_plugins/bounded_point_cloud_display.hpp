// Copyright 2026 MMR Driverless
//
// Custom RViz2 display for mmr_base/msg/BoundedPointcloud.
//

#ifndef MMR_RVIZ_PLUGINS__BOUNDED_POINT_CLOUD_DISPLAY_HPP_
#define MMR_RVIZ_PLUGINS__BOUNDED_POINT_CLOUD_DISPLAY_HPP_

#include <memory>

#include "rviz_common/ros_topic_display.hpp"
#include "mmr_base/msg/bounded_pointcloud.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

namespace rviz_common
{
namespace properties
{
class TfFrameProperty;
}
}

namespace rviz_default_plugins
{
class PointCloudCommon;
}

namespace mmr_rviz_plugins
{

// We derive from RosTopicDisplay (NOT MessageFilterDisplay) because
// BoundedPointcloud carries no std_msgs/Header. RosTopicDisplay gives us the
// topic dropdown, the subscription lifecycle, and the processMessage() hook,
// but leaves frame resolution to us — which is exactly why we add our own
// "Reference Frame" property below.
class BoundedPointCloudDisplay
  : public rviz_common::RosTopicDisplay<mmr_base::msg::BoundedPointcloud>
{
  Q_OBJECT

public:
  BoundedPointCloudDisplay();
  ~BoundedPointCloudDisplay() override;

protected:
  // Called once, after scene_manager_/scene_node_/context_ are valid.
  void onInitialize() override;

  // Called on "Reset" or when the topic changes: drop all rendered geometry.
  void reset() override;

  // PointCloudCommon must be pumped every frame, or the cloud never renders.
  void update(float wall_dt, float ros_dt) override;

  // Called for every incoming message: rebuild the scene to match it.
  void processMessage(mmr_base::msg::BoundedPointcloud::ConstSharedPtr msg) override;

private:
  std::unique_ptr<rviz_default_plugins::PointCloudCommon> point_cloud_common_;

  // User-tunable properties (shown in the RViz Displays panel).
  rviz_common::properties::TfFrameProperty * frame_property_;

  // Cached sensor_msgs::msg::PointCloud2 to avoid re-allocating fields/metadata on every message.
  sensor_msgs::msg::PointCloud2 static_pc2_msg_;
};

}  // namespace mmr_rviz_plugins

#endif  // MMR_RVIZ_PLUGINS__BOUNDED_POINT_CLOUD_DISPLAY_HPP_
