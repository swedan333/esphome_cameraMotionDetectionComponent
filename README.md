# ESPHome Motion Detection Component for Home Assistant

This repository contains a custom ESPHome component for motion detection that processes image data and triggers events when motion is detected within a defined region of interest (ROI). It uses a Xiao S3 with a 5MP camera module. This component is optimized for performance by using black-and-white (grayscale) processing and allows Home Assistant to dynamically change camera resolution when motion is detected.

## Features

- Custom motion detection using image-based pixel analysis.
- Configurable region of interest (ROI) for motion detection.
- Optimized by processing only grayscale (black-and-white) data.
- Supports dynamic resolution switching for detailed video capture.
- Works seamlessly with Home Assistant for automation.

## Prerequisites

1. Install and configure [ESPHome](https://esphome.io/).
2. Add this repository as an external component in your ESPHome YAML configuration.
3. Configure your ESPHome YAML to use the custom motion detection component.
4. Set up Home Assistant automations to trigger events based on motion detection.

## Installation

1. Clone this repository or copy the `motion_detection_component.h` file.
2. Reference this repository in your ESPHome YAML file using the `external_components` feature.

### Example ESPHome YAML Configuration

This example shows how to configure the camera and the motion detection component in your ESPHome configuration.

```yaml
external_components:
  - source: github://swedan333/esphome_cameraMotionDetectionComponent.git
    components: [binary_sensor]

camera:
  - platform: esp32
    model: OV2640
    name: "Xiao S3 Camera"
    resolution: 640x480   # Start with a lower resolution for motion detection
    max_fps: 10fps
    id: xiao_s3_camera

binary_sensor:
  - platform: custom
    id: motion_sensor
    name: "Motion Detected in ROI"
    lambda: |-
      return {new MotionDetectionComponent(
        id(xiao_s3_camera),
        0.1, 0.1,  # x_start and y_start in percentage
        0.5, 0.5,  # width and height in percentage
        10,        # Motion sensitivity threshold
        2,         # Pixel spacing (compare every second pixel)
        2.0        # FPS for motion detection
      )};
```


### Example Home Assistant Automations

These automations demonstrate how to change the camera resolution and start video streaming when motion is detected, and then revert back to a lower resolution once motion has stopped.

#### Automation to Detect Motion, Change Resolution, and Start Stream

When the custom motion detection component detects motion, Home Assistant can trigger an action to change the camera resolution to 5MP and start the video stream.

```yaml
automation:
  - alias: "Change to 5MP and Start Stream on Motion Detection"
    trigger:
      - platform: state
        entity_id: binary_sensor.motion_sensor  # The ID of your motion sensor
        to: 'on'
    action:
      - service: esphome.xiao_s3_camera_set_resolution  # Change resolution to 5MP
        data:
          resolution: 2560x1920  # 5MP resolution
      - service: camera.turn_on  # Start video stream
        entity_id: camera.xiao_s3_camera
      - delay: "00:00:10"  # Stream for 10 seconds (adjust as needed)
```

### Automation to Reset Resolution Back to Low After Streaming

This automation reverts the camera back to a lower resolution after no motion is detected for a specified period (e.g., 30 seconds).

```yaml
automation:
  - alias: "Change Back to Low Resolution After Stream"
    trigger:
      - platform: state
        entity_id: binary_sensor.motion_sensor  # Same motion sensor
        to: 'off'
      - for:
          seconds: 30  # Wait 30 seconds after no motion
    action:
      - service: esphome.xiao_s3_camera_set_resolution  # Change back to lower resolution
        data:
          resolution: 640x480  # Low resolution
```

### Steps Explanation
1. Trigger on Motion Detection:

- When motion is detected by the motion_sensor in ESPHome, the automation is triggered.
- The camera resolution is changed to 5MP (2560x1920), and the video stream is started using camera.turn_on.

2. Streaming Timeout:

- The stream continues for 10 seconds (delay: 00:00:10) after the resolution change.
- You can adjust the delay based on your needs.

3. Return to Low Resolution:

- When motion stops, the second automation triggers after 30 seconds of no motion.
- The resolution is reverted back to 640x480 to conserve resources and lower the load on the ESP32.
