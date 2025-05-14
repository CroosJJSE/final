import cv2
import os
import numpy as np
from natsort import natsorted

# Configuration
input_dir = "output_frames"
output_video = "output_comparison.mp4"
fps = 2  # Adjust based on your processing speed

# Get all frames
original_files = natsorted([f for f in os.listdir(input_dir) if f.startswith("original_")])
mask_files = natsorted([f for f in os.listdir(input_dir) if f.startswith("mask_")])
result_files = natsorted([f for f in os.listdir(input_dir) if f.startswith("result_")])

if not original_files:
    print("No frames found in output_frames directory!")
    exit()

# Determine frame size
sample = cv2.imread(os.path.join(input_dir, original_files[0]))
height, width = sample.shape[:2]

# Create video writer
fourcc = cv2.VideoWriter_fourcc(*'mp4v')
video_writer = cv2.VideoWriter(output_video, fourcc, fps, (width*3, height))

print(f"Creating video with {len(original_files)} frames at {fps} FPS...")

for orig_file, mask_file, result_file in zip(original_files, mask_files, result_files):
    # Read images
    original = cv2.imread(os.path.join(input_dir, orig_file))
    mask = cv2.imread(os.path.join(input_dir, mask_file))
    result = cv2.imread(os.path.join(input_dir, result_file))
    
    # Convert mask to 3-channel for consistent display
    if len(mask.shape) == 2:
        mask = cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR)
    
    # Create side-by-side frame
    combined = np.hstack((original, mask, result))
    
    # Add labels
    font = cv2.FONT_HERSHEY_SIMPLEX
    cv2.putText(combined, "Original", (10, 30), font, 1, (255,255,255), 2)
    cv2.putText(combined, "Mask", (width+10, 30), font, 1, (255,255,255), 2)
    cv2.putText(combined, "Result", (width*2+10, 30), font, 1, (255,255,255), 2)
    
    video_writer.write(combined)

video_writer.release()
print(f"Video saved as {output_video}")

# Optional: Create preview window
cap = cv2.VideoCapture(output_video)
while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break
    
    cv2.imshow('Side-by-Side Comparison', frame)
    if cv2.waitKey(30) & 0xFF == 27:  # ESC to exit
        break

cap.release()
cv2.destroyAllWindows()
