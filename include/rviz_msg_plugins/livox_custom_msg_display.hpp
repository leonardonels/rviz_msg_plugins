// Copyright 2026 MMR Driverless
//
// Custom RViz2 display for livox_ros_driver2/msg/CustomMsg.
//

#ifndef RVIZ_MSG_PLUGINS__LIVOX_CUSTOM_MSG_DISPLAY_HPP_
#define RVIZ_MSG_PLUGINS__LIVOX_CUSTOM_MSG_DISPLAY_HPP_

#include <memory>

#include "rviz_common/message_filter_display.hpp"
#include "livox_ros_driver2/msg/custom_msg.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

namespace rviz_default_plugins
{
class PointCloudCommon;
}

namespace rviz_msg_plugins
{

// CustomMsg carries a std_msgs/Header (frame_id is
// "livox_frame" by default), so we derive from MessageFilterDisplay: it drops
// messages until TF can place header.frame_id in the fixed frame, exactly like
// the stock PointCloud2 display. No "Reference Frame" property is needed.
class LivoxCustomMsgDisplay
  : public rviz_common::MessageFilterDisplay<livox_ros_driver2::msg::CustomMsg>
{
  Q_OBJECT

public:
  LivoxCustomMsgDisplay();
  ~LivoxCustomMsgDisplay() override;

protected:
  // Called once, after scene_manager_/scene_node_/context_ are valid.
  void onInitialize() override;

  // Called on "Reset" or when the topic changes: drop all rendered geometry.
  void reset() override;

  // PointCloudCommon must be pumped every frame, or the cloud never renders.
  void update(float wall_dt, float ros_dt) override;

  // Called for every incoming message: convert it to PointCloud2 and hand off.
  void processMessage(livox_ros_driver2::msg::CustomMsg::ConstSharedPtr msg) override;

private:
  std::unique_ptr<rviz_default_plugins::PointCloudCommon> point_cloud_common_;

  // PointCloud2 template with the fixed PointField metadata, built once.
  sensor_msgs::msg::PointCloud2 static_pc2_msg_;
};

}  // namespace rviz_msg_plugins

#endif  // RVIZ_MSG_PLUGINS__LIVOX_CUSTOM_MSG_DISPLAY_HPP_
