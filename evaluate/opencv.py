import numpy as np
import cv2
import sys
from h264flow import h264

MACROBLOCK_SIZE = 16


def unit_vector(vector):
    """ Returns the unit vector of the vector.  """
    return vector / np.linalg.norm(vector)


def angle_diff(v1, v2):
    v1_u = unit_vector(v1)
    v2_u = unit_vector(v2)
    return np.arccos(np.clip(np.dot(v1_u, v2_u), -1.0, 1.0))


def compute_angle_diff(mvs, truth):
    result = np.zeros((mvs.shape[0], mvs.shape[1]))
    print(mvs.shape, truth.shape)
    for i in range(result.shape[0]):
        for j in range(result.shape[1]):
            result[i][j] = angle_diff(mvs[i][j], truth[i][j])
    return result


def read_optical_flow(cap, frame_num):
    if frame_num < 1:
        return None
    # notice it is backward motion vector
    cap.set(cv2.CAP_PROP_POS_FRAMES, frame_num - 1)
    ret, frame1 = cap.read()
    prev_frame = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
    ret, frame2 = cap.read()
    next_frame = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)
    flow = cv2.calcOpticalFlowFarneback(prev_frame, next_frame, None, 0.5, 3,
                                        15, 3, 5, 1.2, 0)
    result = np.zeros((flow.shape[0] // 16, flow.shape[1] // 16, 2))
    for y in range(result.shape[0]):
        for x in range(result.shape[1]):
            result[y][x] = flow[y * MACROBLOCK_SIZE: y * MACROBLOCK_SIZE + 16,
                     x * MACROBLOCK_SIZE: x*MACROBLOCK_SIZE + 16].mean(1).mean(0) \
                           * MACROBLOCK_SIZE
    return result


def main():
    if len(sys.argv) != 2:
        print("Usage:", sys.argv[0], "<filename>")
        exit(1)
    cap = cv2.VideoCapture(sys.argv[1])
    decoder = h264(sys.argv[1])
    num_of_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    results = []
    for frame_num in range(1, num_of_frames):
        mvs = decoder.load_frame(frame_num).mvL0
        truth = read_optical_flow(cap, frame_num)
        angle_d = compute_angle_diff(mvs, truth)
        results.append(angle_d)
    cap.release()
    with open("angle_diff", "w+") as f:
        np.save(f, results)

if __name__ == "__main__":
    main()