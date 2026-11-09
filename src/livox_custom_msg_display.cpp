// Copyright 2026 MMR Driverless
//
// Implementation of LivoxCustomMsgDisplay.

#include "rviz_msg_plugins/livox_custom_msg_display.hpp"

#include <cstring>
#include <memory>

#include "rviz_default_plugins/displays/pointcloud/point_cloud_common.hpp"

namespace rviz_msg_plugins
{

namespace
{
// Packed layout of one point in the generated PointCloud2:
//   x, y, z      float32  (m)
//   intensity    float32  (CustomPoint::reflectivity, 0..255)
//   tag          uint8    (Livox noise / return flags)
//   line         uint8    (laser number)
//   [2 bytes padding]
//   offset_time  uint32   (ns from CustomMsg::timebase)
// Every field is exposed so RViz's Color Transformer can color by any of them.
constexpr uint32_t kOffX = 0;
constexpr uint32_t kOffY = 4;
constexpr uint32_t kOffZ = 8;
constexpr uint32_t kOffIntensity = 12;
constexpr uint32_t kOffTag = 16;
constexpr uint32_t kOffLine = 17;
constexpr uint32_t kOffOffsetTime = 20;
constexpr uint32_t kPointStep = 24;
}  // namespace

LivoxCustomMsgDisplay::LivoxCustomMsgDisplay()
// Passing `this` lets PointCloudCommon attach its Style/Size/Color properties
// under our display in the panel. That single line is how the user gets all
// shape + color choices.
: point_cloud_common_(
    std::make_unique<rviz_default_plugins::PointCloudCommon>(this))
{
  static_pc2_msg_.is_dense = false;
  static_pc2_msg_.point_step = kPointStep;
  static_pc2_msg_.height = 1;  // unorganized cloud
  static_pc2_msg_.is_bigendian = false;

  auto add_field = [&](const char * name, uint8_t datatype, uint32_t off) {
      sensor_msgs::msg::PointField f;
      f.name = name;
      f.offset = off;
      f.datatype = datatype;
      f.count = 1;
      static_pc2_msg_.fields.push_back(f);
    };

  using PF = sensor_msgs::msg::PointField;
  add_field("x", PF::FLOAT32, kOffX);
  add_field("y", PF::FLOAT32, kOffY);
  add_field("z", PF::FLOAT32, kOffZ);
  add_field("intensity", PF::FLOAT32, kOffIntensity);
  add_field("tag", PF::UINT8, kOffTag);
  add_field("line", PF::UINT8, kOffLine);
  add_field("offset_time", PF::UINT32, kOffOffsetTime);
}

LivoxCustomMsgDisplay::~LivoxCustomMsgDisplay() = default;

void LivoxCustomMsgDisplay::onInitialize()
{
  // IMPORTANT: let the base class create scene_node_, the topic property and
  // the TF message filter. Forgetting this super-call is the #1 cause of crashes.
  MFDClass::onInitialize();

  // PointCloudCommon must be initialized with the context and scene node.
  point_cloud_common_->initialize(context_, scene_node_);
}

void LivoxCustomMsgDisplay::reset()
{
  MFDClass::reset();
  point_cloud_common_->reset();
}

void LivoxCustomMsgDisplay::update(float wall_dt, float ros_dt)
{
  point_cloud_common_->update(wall_dt, ros_dt);
}

void LivoxCustomMsgDisplay::processMessage(
  livox_ros_driver2::msg::CustomMsg::ConstSharedPtr msg)
{
  // TF resolution already happened in the MessageFilter: by the time we get
  // here header.frame_id is known to be transformable into the fixed frame.
  auto pc2 = std::make_shared<sensor_msgs::msg::PointCloud2>(static_pc2_msg_);
  pc2->header = msg->header;

  // CustomPoint is a generated C++ struct, not a guaranteed packed buffer, so
  // we pack field by field instead of memcpy-ing the whole vector.
  const uint32_t n = static_cast<uint32_t>(msg->points.size());
  pc2->width = n;
  pc2->row_step = n * kPointStep;
  pc2->data.assign(pc2->row_step, 0);

  uint8_t * out = pc2->data.data();
  for (const auto & p : msg->points) {
    const float intensity = static_cast<float>(p.reflectivity);
    std::memcpy(out + kOffX, &p.x, sizeof(float));
    std::memcpy(out + kOffY, &p.y, sizeof(float));
    std::memcpy(out + kOffZ, &p.z, sizeof(float));
    std::memcpy(out + kOffIntensity, &intensity, sizeof(float));
    out[kOffTag] = p.tag;
    out[kOffLine] = p.line;
    std::memcpy(out + kOffOffsetTime, &p.offset_time, sizeof(uint32_t));
    out += kPointStep;
  }

  // Hand off. From here RViz transforms (via frame_id), colors, shapes,
  // sizes, and renders the cloud entirely on its own.
  point_cloud_common_->addMessage(pc2);
}

}  // namespace rviz_msg_plugins

// This macro registers the class with pluginlib so RViz can instantiate it by
// name. The string in plugins_description.xml must match this fully-qualified
// class name. Keep them in sync.
#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(rviz_msg_plugins::LivoxCustomMsgDisplay, rviz_common::Display)
