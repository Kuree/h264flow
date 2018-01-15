import numpy as np
import cv2
import sys
from h264flow import h264
import os

MACROBLOCK_SIZE = 16


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
    if len(sys.argv) != 3:
        print("Usage:", sys.argv[0], "<filename> <output>")
        exit(1)
    cap = cv2.VideoCapture(sys.argv[1])
    output_file = sys.argv[2]
    decoder = h264(sys.argv[1])
    num_of_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    # 0: mvs 1: truth
    results = [[], []]
    try:
        for frame_num in range(1, num_of_frames):
            mf = decoder.load_frame(frame_num)
            mvs = mf.mvL0
            if not mf.p_frame:
                continue
            truth = read_optical_flow(cap, frame_num)
            if mvs is None or truth is None:
                continue
            results[0].append(mvs)
            results[1].append(truth)
            if frame_num % 3 == 0:
                print("progress:", "{0:4f}%".format(frame_num /
                                                    num_of_frames * 100),
                      end="\r")
    except RuntimeError:
        print("unable to decode frame_num", frame_num)
    finally:
        print()
        cap.release()
        np.save(output_file, results)


if __name__ == "__main__":
    main()
