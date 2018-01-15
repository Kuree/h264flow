import cv2
import sys
import os
import json


win_name = "video" # Window name
data = {}
output_file = ""

def save_data():
    global data, output_file
    print(data)
    if len(output_file) > 0:
        with open(output_file, "w+") as f:
            json.dump(data, f)

def define_rect(frame):
    rect_pts = [] # Starting and ending points

    def select_points(event, x, y, flags, param):

        clone = frame.copy()
        nonlocal rect_pts
        if event == cv2.EVENT_LBUTTONDOWN:
            clone = frame.copy()
            cv2.imshow(win_name, clone)
            rect_pts = [(x, y)]

        if event == cv2.EVENT_LBUTTONUP:
            rect_pts.append((x, y))

            # draw a rectangle around the region of interest
            cv2.rectangle(clone, rect_pts[0], rect_pts[1], (0, 255, 0), 2)
            cv2.imshow(win_name, clone)

    cv2.setMouseCallback(win_name, select_points)

    while True:
        # display the image and wait for a keypress
        cpy = frame.copy()
        cv2.imshow(win_name, cpy)
        key = cv2.waitKey(0) & 0xFF

        if key == ord("r"): # Hit 'r' to replot the image
            clone = image.copy()

        elif key == ord("c"): # Hit 'c' to confirm the selection
            break
        elif key == ord("n"): # Next
            return None
        elif key == ord("q"): # quit
            save_data()
            exit(0)
        elif key == ord("j"): # jump
            return 1

    return rect_pts


def main():
    global data, output_file
    if len(sys.argv) != 3:
        print("usage:", sys.argv[0], "<media_file> <data_file>")
    media_file = sys.argv[1]
    output_file = sys.argv[2]
    cap = cv2.VideoCapture(sys.argv[1])
    if cap is None:
        print("cannot open", media_file)
        exit(1)
    # load data file if possible
    cv2.namedWindow(win_name)

    if os.path.isfile(output_file):
        with open(output_file) as f:
            data = json.load(f)

    frame_num = 0
    total_frame_num = num_of_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    while frame_num < total_frame_num:
        cap.set(cv2.CAP_PROP_POS_FRAMES, frame_num)
        ret, frame = cap.read()
        points = define_rect(frame)
        if points != None:
            if points == 1:
                frame_num += 9
            else:
                data[frame_num] = points
        frame_num += 1
    save_data()

if __name__ == "__main__":
    main()
