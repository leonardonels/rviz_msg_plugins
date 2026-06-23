```cpp
// ============================================================================
// PointCloudZcDisplay — minimal "RViz owns the visuals" take.
//
// Division of labor:
//   * PLUGIN's job: convert the packed 26-byte PointCloudZc buffer into a
//                   sensor_msgs::PointCloud2, and run the display lifecycle.
//   * RVIZ's job:   EVERYTHING visual. PointCloudCommon auto-adds Style (Points/
//                   Spheres/Boxes/Flat Squares/...), Size, and Color Transformer
//                   (Intensity/AxisColor/FlatColor) properties under this display.
//                   We write no color or shape code — the user chooses in-panel.
// ============================================================================

class PointCloudZcDisplay
  : public rviz_common::RosTopicDisplay<mmr_msgs::msg::PointCloudZc>
{
  Q_OBJECT
public:
  PointCloudZcDisplay()
  // Passing `this` lets PointCloudCommon attach its Style/Size/Color properties
  // under our display in the panel. That single line is how the user gets all
  // shape + color choices.
  : point_cloud_common_(
      std::make_unique<rviz_default_plugins::PointCloudCommon>(this))
  {
    // The message carries no frame_id, so the user supplies one.
    // Plain StringProperty (no TfFrameProperty dropdown to pick one from the 
    // live TF tree - it will probbaly be empty).
    frame_property_ = new rviz_common::properties::StringProperty(
      "Frame", "lidar",
      "TF frame to render the cloud in (message carries no frame_id).", this);
  }

  void onInitialize() override
  {
    RosTopicDisplay<mmr_msgs::msg::PointCloudZc>::onInitialize();  // topic/QoS/sub
    point_cloud_common_->initialize(context_, scene_node_);        // REQUIRED
  }

  // PointCloudCommon must be pumped every frame, or the cloud never renders.
  void update(float wall_dt, float ros_dt) override
  {
    point_cloud_common_->update(wall_dt, ros_dt);
  }

  void reset() override
  {
    RosTopicDisplay<mmr_msgs::msg::PointCloudZc>::reset();
    point_cloud_common_->reset();
  }

protected:
  void processMessage(
    mmr_msgs::msg::PointCloudZc::ConstSharedPtr msg) override
  {
    // The 26-byte layout is fixed by the producer. We declare the fields so
    // RViz's Color Transformer can read them (e.g. color by "intensity" or
    // "ring"), then bulk-copy the bytes straight through (zero parsing).
    auto pc2 = std::make_shared<sensor_msgs::msg::PointCloud2>();
    pc2->header.stamp    = msg->timestamp;
    pc2->header.frame_id = frame_property_->getStdString();
    pc2->height          = 1;                 // unorganized cloud
    pc2->width           = msg->width;
    pc2->is_bigendian    = false;
    pc2->is_dense        = false;

    using PF = sensor_msgs::msg::PointField;
    auto f = [](const std::string & n, uint32_t off, uint8_t dt) {
      PF p; p.name = n; p.offset = off; p.datatype = dt; p.count = 1; return p;
    };
    pc2->fields = {                            // these names show up as
      f("x",         0,  PF::FLOAT32),         //   "color by" choices in RViz
      f("y",         4,  PF::FLOAT32),
      f("z",         8,  PF::FLOAT32),
      f("intensity", 12, PF::FLOAT32),
      f("ring",      16, PF::UINT16),
      f("timestamp", 18, PF::FLOAT64),
    };
    pc2->point_step = 26;
    pc2->row_step   = pc2->point_step * pc2->width;
    pc2->data.resize(pc2->row_step);
    std::memcpy(pc2->data.data(), msg->data.data(), pc2->row_step);

    // Hand off. From here RViz transforms (via frame_id), colors, shapes,
    // sizes, and renders the cloud entirely on its own.
    point_cloud_common_->addMessage(pc2);
  }

private:
  std::unique_ptr<rviz_default_plugins::PointCloudCommon> point_cloud_common_;
  rviz_common::properties::StringProperty * frame_property_;
};
```