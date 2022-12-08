import Adafruit_BBIO.UART as UART
from serial import Serial
from cv2 import VideoCapture, CascadeClassifier, CascadeClassifier, COLOR_BGR2GRAY, cvtColor, resize, CAP_PROP_BUFFERSIZE

# Huge help:  https://dontrepeatyourself.org/post/smile-detection-with-python-opencv-and-haar-cascade/

UART.setup("UART1")
print("Setup UART")

SMILE = 0x73
FACE = 0x55
NOTHING = 0x11

smile_bytes = SMILE.to_bytes(1, 'big')
nothing_bytes = NOTHING.to_bytes(1, 'big')
face_bytes = FACE.to_bytes(1, 'big')

ser = Serial(port = "/dev/ttyO1", baudrate=115200)
ser.close()
ser.open()

print("UART opened")

video_capture = VideoCapture(0)
#video_capture.set(CAP_PROP_BUFFERSIZE, 1)
print("Initialized camera")

face_detector = CascadeClassifier("haarcascade_frontalface_default.xml")
print("Imported face haar")

smile_detector = CascadeClassifier("haarcascade_smile.xml")
print("Imported smile haar")

print ("entering main loop")
while True:
    
    send_bytes = nothing_bytes

    for i in range(5):
        video_capture.grab()

    _, frame = video_capture.read()   
    gray = cvtColor(frame, COLOR_BGR2GRAY)
        
    gray_re =  resize(gray,(320,240))
    #frame_re =  frame[210:430, 160:320]
    
    # apply our face detector to the grayscale frame
    faces = face_detector.detectMultiScale(gray_re, 1.1, 8, minSize=(70,70))
    
    for (x, y, w, h) in faces:
        # get the region of the face
        roi = gray_re[y:y + h, x:x + w]
        
        # apply our smile detector to the region of the face
        smile_rects, rejectLevels, levelWeights = smile_detector.detectMultiScale3(
                                                        roi, 2.0, 20, outputRejectLevels=True)
                                                        
        if len(levelWeights) == 0:
            send_bytes = face_bytes
        else:
            if max(levelWeights) >= 2:
                send_bytes = smile_bytes
                
    if ser.isOpen():
        ser.write(send_bytes)
    
ser.close()

# Eventually, you'll want to clean up, but leave this commented for now, 
# as it doesn't work yet
#UART.cleanup()