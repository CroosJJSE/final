import requests
import cv2
import numpy as np
import base64
import json

# Load a test image
img = cv2.imread('test.jpg')
_, img_encoded = cv2.imencode('.png', img)

# Send to server
response = requests.post(
    'http://192.248.10.70:8000/segment',
    files={'image': ('image.png', img_encoded.tobytes(), 'image/png')}
)

# Print raw response
print("Status:", response.status_code)
print("Response:", response.text[:200])

# Parse JSON
data = response.json()
print("JSON keys:", data.keys())

# Check for masks
if 'masks' in data and data['masks']:
    # Decode the first mask
    mask_data = base64.b64decode(data['masks'][0])
    mask = cv2.imdecode(np.frombuffer(mask_data, np.uint8), cv2.IMREAD_UNCHANGED)
    print("Mask shape:", mask.shape)
    cv2.imwrite('mask.png', mask)
else:
    print("No masks found")