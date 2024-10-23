#ifndef MOTION_DETECTION_COMPONENT_H
#define MOTION_DETECTION_COMPONENT_H

#include "esphome.h"
#include "esp_camera.h"
#include <vector>

class MotionDetectionComponent : public Component, public BinarySensor {
 public:
  camera::Camera *camera_;
  float x_start_percent, y_start_percent, width_percent, height_percent;
  int motion_threshold;  // Sensitivity threshold for motion
  int pixel_spacing;     // How many pixels to skip (e.g., 2 = every second pixel)
  float fps;             // Frames per second for motion detection

  unsigned long last_frame_time = 0;  // To track FPS timing
  std::vector<uint8_t> previous_frame;  // Store the previous frame's pixels for comparison

  // Constructor
  MotionDetectionComponent(camera::Camera *camera, float x_start, float y_start, float width, float height, int threshold, int pixel_spacing, float fps) {
    this->camera_ = camera;
    this->x_start_percent = x_start;
    this->y_start_percent = y_start;
    this->width_percent = width;
    this->height_percent = height;
    this->motion_threshold = threshold;
    this->pixel_spacing = pixel_spacing;
    this->fps = fps;
  }

  void setup() override {
    // Any initialization logic can go here
  }

  void loop() override {
    // Handle FPS limit by checking time since last frame
    unsigned long current_time = millis();
    if (current_time - last_frame_time < (1000.0 / fps)) {
      return;  // Skip frame if below desired FPS
    }
    last_frame_time = current_time;

    // Get a frame from the camera
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) {
      ESP_LOGE("MotionDetection", "Failed to get frame");
      return;
    }

    // Calculate the region of interest (ROI) based on percentage and current resolution
    int x_start = (int)(x_start_percent * frame->width);
    int y_start = (int)(y_start_percent * frame->height);
    int roi_width = (int)(width_percent * frame->width);
    int roi_height = (int)(height_percent * frame->height);

    // Perform motion detection in the specified region of interest
    if (detect_motion_in_roi(frame, x_start, y_start, roi_width, roi_height)) {
      publish_state(true);  // Motion detected
    } else {
      publish_state(false); // No motion
    }

    // Return the frame buffer to the camera driver
    esp_camera_fb_return(frame);
  }

  bool detect_motion_in_roi(camera_fb_t *frame, int x_start, int y_start, int width, int height) {
    // Calculate the number of pixels we'll test based on pixel spacing
    int num_test_pixels = (width / pixel_spacing) * (height / pixel_spacing);

    // Initialize previous frame buffer if it doesn't match the current number of test pixels
    if (previous_frame.size() != num_test_pixels) {
      previous_frame.resize(num_test_pixels);
    }

    int pixel_diff = 0;  // Count how many pixels differ significantly
    int pixel_count = 0; // Index for the test pixels in the previous_frame array

    // Loop through the pixels in the ROI, skipping pixels based on pixel spacing
    for (int y = y_start; y < y_start + height; y += pixel_spacing) {
      for (int x = x_start; x < x_start + width; x += pixel_spacing) {
        // Calculate the pixel index in the frame buffer (RGB565 format uses 2 bytes per pixel)
        int index = (y * frame->width + x) * 2;
        uint16_t pixel = (frame->buf[index] << 8) | frame->buf[index + 1];

        // Convert RGB565 pixel to grayscale using a simple weighted average of RGB values
        uint8_t r = (pixel >> 11) & 0x1F;   // Red (5 bits)
        uint8_t g = (pixel >> 5) & 0x3F;    // Green (6 bits)
        uint8_t b = pixel & 0x1F;           // Blue (5 bits)
        uint8_t grayscale_value = (r * 299 + g * 587 + b * 114) / 1000;  // Grayscale conversion

        // Compare with the previous frame's grayscale value
        if (abs(grayscale_value - previous_frame[pixel_count]) > motion_threshold) {
          pixel_diff++;
        }

        // Update previous frame with the current grayscale value
        previous_frame[pixel_count] = grayscale_value;
        pixel_count++;
      }
    }

    // Return true if a significant number of pixels differ, indicating motion
    return (pixel_diff > (num_test_pixels * 0.01));  // 1% of pixels must differ to trigger motion
  }
};

#endif  // MOTION_DETECTION_COMPONENT_H
