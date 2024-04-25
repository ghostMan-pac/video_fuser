import numpy as np
import cv2
import sysv_ipc
import sys
from ultralytics import YOLO

# Constants
WIDTH = 1280
HEIGHT = 960
DEPTH = 4  # Assuming RGB color channels

# Connect to the shared memory
shm = sysv_ipc.SharedMemory(123456)
print(shm)

mutex = sysv_ipc.Semaphore(3104)
model=YOLO("yolov8n.pt")
model.export(format='engine')
tensorrt_model=YOLO('yolov8n.engine')
while True:
    # Attach the shared memory

     if mutex.acquire() == -1:
         print("Failed to acquire the semaphore")
         sys.exit(1)
     else:
        print("Acquired the semaphore")
        shared_memory = shm.read()

        if shared_memory is not None:
            # Reshape the shared memory data into an image array
            image = np.frombuffer(shared_memory, dtype=np.uint8).reshape((HEIGHT, WIDTH, DEPTH))
            mutex.release()
            # Create an OpenCV Mat from the image array
            img = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)

            result=tensorrt_model(img,show=True,conf = .5)
  #      print(result)
        # Display the image


       # cv2.imshow("Image", img)
       # cv2.imwrite("out.jpeg", img)
# Detach from the shared memory
shm.detach()
