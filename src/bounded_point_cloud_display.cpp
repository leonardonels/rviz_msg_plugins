// Copyright 2026 MMR Driverless
//
// Implementation of BoundedPointCloudDisplay.

#include "mmr_rviz_plugins/bounded_point_cloud_display.hpp"

#include <memory>
#include <cstring>

#include "rviz_common/display_context.hpp"
#include "rviz_common/frame_manager_iface.hpp"
#include "rviz_common/properties/tf_frame_property.hpp"
#include "rviz_default_plugins/displays/pointcloud/point_cloud_common.hpp"

namespace mmr_rviz_plugins
{

BoundedPointCloudDisplay::BoundedPointCloudDisplay()
// Passing `this` lets PointCloudCommon attach its Style/Size/Color properties
// under our display in the panel. That single line is how the user gets all
// shape + color choices.
: point_cloud_common_(
    std::make_unique<rviz_default_plugins::PointCloudCommon>(this))
{
  // Properties created here appear under the display in the Displays panel.
  // The TfFrameProperty needs its FrameManager set
  // later (in onInitialize), because context_ is not valid yet in the ctor.
  frame_property_ = new rviz_common::properties::TfFrameProperty(
    "Reference Frame", "lidar_link",
    "The TF frame the BoundedPointcloud coordinates are expressed in. "
    "The message has no Header, so you must select it here.",
    this, nullptr, false, nullptr, this);

  // Set up static metadata for the PointCloud2 message
  static_pc2_msg_.is_dense = false;
  static_pc2_msg_.point_step = 26;
  static_pc2_msg_.height = 1;
  static_pc2_msg_.is_bigendian = false;

  static_pc2_msg_.fields.clear();
  static_pc2_msg_.fields.reserve(6);

  auto add_field = [&](const char * name, uint8_t datatype, uint32_t off) {
    sensor_msgs::msg::PointField f;
    f.name = name;
    f.offset = off;
    f.datatype = datatype;
    f.count = 1;
    static_pc2_msg_.fields.push_back(f);
  };

  uint32_t offset = 0;
  // Build a sensor_msgs::msg::PointCloud2 once at startup with the fixed PointField metadata
  add_field("x",         sensor_msgs::msg::PointField::FLOAT32, offset); offset += 4;
  add_field("y",         sensor_msgs::msg::PointField::FLOAT32, offset); offset += 4;
  add_field("z",         sensor_msgs::msg::PointField::FLOAT32, offset); offset += 4;
  add_field("intensity", sensor_msgs::msg::PointField::FLOAT32, offset); offset += 4;
  add_field("ring",      sensor_msgs::msg::PointField::UINT16,  offset); offset += 2;
  add_field("timestamp", sensor_msgs::msg::PointField::FLOAT64, offset); offset += 8;
}

BoundedPointCloudDisplay::~BoundedPointCloudDisplay() = default;

void BoundedPointCloudDisplay::onInitialize()
{
  // IMPORTANT: let the base class create scene_node_, wire up the topic
  // property, etc. Forgetting this super-call is the #1 cause of crashes.
  RTDClass::onInitialize();

  // Now that context_ exists, give the frame dropdown access to the live TF
  // tree so it can populate the list of frames.
  frame_property_->setFrameManager(context_->getFrameManager());

  // PointCloudCommon must be initialized with the context and scene node
  point_cloud_common_->initialize(context_, scene_node_);
}

void BoundedPointCloudDisplay::reset()
{
  RTDClass::reset();
  point_cloud_common_->reset();
}

void BoundedPointCloudDisplay::update(float wall_dt, float ros_dt)
{
  point_cloud_common_->update(wall_dt, ros_dt);
}

void BoundedPointCloudDisplay::processMessage(
  mmr_base::msg::BoundedPointcloud::ConstSharedPtr msg)
{
  // 1) Resolve where the chosen reference frame sits relative to the RViz fixed
  //    frame, *now*. Because the message has no stamp, we ask for the latest
  //    transform. If TF can't answer, surface a status and bail.
  Ogre::Vector3 frame_position;
  Ogre::Quaternion frame_orientation;
  const std::string frame = frame_property_->getFrameStd();
  if (!context_->getFrameManager()->getTransform(
      frame, frame_position, frame_orientation))
  {
    setMissingTransformToFixedFrame(frame);
    return;
  }
  setTransformOk();

  // 2) On each callback, copy width * 26 bytes from msg->data into pc2->data, 
  // set pc2->width = msg->width, copy the timestamp, set frame_id, and pass to PointCloudCommon.
  auto pc2 = std::make_shared<sensor_msgs::msg::PointCloud2>(static_pc2_msg_);

  uint32_t valid_points = msg->width;
  uint32_t required_bytes = valid_points * 26;

  pc2->width = valid_points;
  pc2->row_step = required_bytes;

  pc2->header.stamp = msg->stamp;
  pc2->header.frame_id = frame;

  pc2->data.resize(required_bytes);

  std::memcpy(pc2->data.data(), msg->data.data(), required_bytes);

  // 3) Hand off. From here RViz transforms (via frame_id), colors, shapes,
  // sizes, and renders the cloud entirely on its own.
  point_cloud_common_->addMessage(pc2);
}

}  // namespace mmr_rviz_plugins

// This macro registers the class with pluginlib so RViz can instantiate it by
// name. The string in plugins_description.xml must match this fully-qualified
// class name. Keep them in sync.
#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(mmr_rviz_plugins::BoundedPointCloudDisplay, rviz_common::Display)
