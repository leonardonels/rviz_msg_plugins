// Copyright 2026 MMR Driverless
//
// Implementation of SpeedProfileDisplay. See the header and
// docs/ADDING_A_PLUGIN.md for the conceptual walkthrough.

#include "mmr_rviz_plugins/speed_profile_display.hpp"

#include <algorithm>
#include <memory>

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include "rviz_common/display_context.hpp"
#include "rviz_common/frame_manager_iface.hpp"
#include "rviz_common/properties/bool_property.hpp"
#include "rviz_common/properties/float_property.hpp"
#include "rviz_common/properties/tf_frame_property.hpp"

#include "rviz_rendering/objects/billboard_line.hpp"
#include "rviz_rendering/objects/shape.hpp"

namespace mmr_rviz_plugins
{

SpeedProfileDisplay::SpeedProfileDisplay()
{
  // Properties created here appear under the display in the Displays panel.
  // Connecting their changed() signal to updateStyle() lets edits re-render the
  // cached message immediately. The TfFrameProperty needs its FrameManager set
  // later (in onInitialize), because context_ is not valid yet in the ctor.
  frame_property_ = new rviz_common::properties::TfFrameProperty(
    "Reference Frame", "map",
    "The TF frame the SpeedProfilePoints coordinates are expressed in. "
    "The message has no Header, so you must select it here.",
    this, nullptr, false, SLOT(updateStyle()), this);

  radius_property_ = new rviz_common::properties::FloatProperty(
    "Sphere Radius", 0.15f,
    "Radius of the sphere drawn at each waypoint (meters).",
    this, SLOT(updateStyle()));
  radius_property_->setMin(0.0f);

  line_width_property_ = new rviz_common::properties::FloatProperty(
    "Line Width", 0.05f,
    "Width of the path line connecting the waypoints (meters).",
    this, SLOT(updateStyle()));
  line_width_property_->setMin(0.0f);

  max_speed_property_ = new rviz_common::properties::FloatProperty(
    "Max Speed", 20.0f,
    "Speed (m/s) that maps to full red. Used to normalize the blue->red "
    "color gradient. Slower points are bluer, faster points redder.",
    this, SLOT(updateStyle()));
  max_speed_property_->setMin(0.01f);

  show_path_property_ = new rviz_common::properties::BoolProperty(
    "Show Path Line", true,
    "Whether to draw the connecting line between waypoints.",
    this, SLOT(updateStyle()));
}

SpeedProfileDisplay::~SpeedProfileDisplay()
{
  // Visuals are scene-node children; clearing them here is tidy. The base class
  // destroys scene_node_ itself.
  clearVisuals();
}

void SpeedProfileDisplay::onInitialize()
{
  // IMPORTANT: let the base class create scene_node_, wire up the topic
  // property, etc. Forgetting this super-call is the #1 cause of crashes.
  RTDClass::onInitialize();

  // Now that context_ exists, give the frame dropdown access to the live TF
  // tree so it can populate the list of frames.
  frame_property_->setFrameManager(context_->getFrameManager());
}

void SpeedProfileDisplay::reset()
{
  RTDClass::reset();
  clearVisuals();
  last_msg_.reset();
}

void SpeedProfileDisplay::clearVisuals()
{
  spheres_.clear();   // shared_ptrs detach the Ogre objects from the scene.
  path_line_.reset();
}

void SpeedProfileDisplay::processMessage(
  mmr_base::msg::SpeedProfilePoints::ConstSharedPtr msg)
{
  last_msg_ = msg;       // cache so updateStyle() can re-render on edits.
  render(*msg);
}

void SpeedProfileDisplay::updateStyle()
{
  // Re-render whatever we last received with the new property values.
  if (last_msg_) {
    render(*last_msg_);
  }
}

void SpeedProfileDisplay::render(const mmr_base::msg::SpeedProfilePoints & msg)
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

  // Anchor our whole sub-scene at that pose; children are then placed in the
  // message's own coordinates and inherit this transform.
  scene_node_->setPosition(frame_position);
  scene_node_->setOrientation(frame_orientation);

  // 2) Rebuild geometry from scratch. For the point counts here (a trajectory)
  //    this is simple and fast enough; a high-rate display would instead pool
  //    and reuse Shape objects.
  clearVisuals();

  const float radius = radius_property_->getFloat();
  const float max_speed = max_speed_property_->getFloat();
  const bool show_path = show_path_property_->getBool();

  if (show_path) {
    path_line_ = std::make_shared<rviz_rendering::BillboardLine>(
      scene_manager_, scene_node_);
    path_line_->setLineWidth(line_width_property_->getFloat());
    path_line_->setMaxPointsPerLine(static_cast<uint32_t>(msg.points.size()));
  }

  spheres_.reserve(msg.points.size());
  for (const auto & sp : msg.points) {
    const Ogre::Vector3 p(
      static_cast<float>(sp.point.x),
      static_cast<float>(sp.point.y),
      static_cast<float>(sp.point.z));

    // Map speed -> color: blue (slow) through to red (fast).
    const float t = std::clamp(
      static_cast<float>(sp.ackerman_point.speed) / max_speed, 0.0f, 1.0f);
    const Ogre::ColourValue color(t, 0.0f, 1.0f - t, 1.0f);

    auto shape = std::make_shared<rviz_rendering::Shape>(
      rviz_rendering::Shape::Sphere, scene_manager_, scene_node_);
    shape->setPosition(p);
    shape->setScale(Ogre::Vector3(radius, radius, radius));
    shape->setColor(color);
    spheres_.push_back(shape);

    if (show_path) {
      path_line_->addPoint(p, color);
    }
  }

  // Ask RViz to repaint now rather than waiting for the next frame tick.
  context_->queueRender();
}

}  // namespace mmr_rviz_plugins

// This macro registers the class with pluginlib so RViz can instantiate it by
// name. The string in plugins_description.xml must match this fully-qualified
// class name. Keep them in sync.
#include <pluginlib/class_list_macros.hpp>  // NOLINT
PLUGINLIB_EXPORT_CLASS(mmr_rviz_plugins::SpeedProfileDisplay, rviz_common::Display)
