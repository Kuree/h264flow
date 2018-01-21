from h264flow import load_mv, dump_mv, create_mv
from os import path
import sys
import os
import glob
import numpy as np
import scipy.ndimage

# keep the aspect ratio
OUTPUT_WIDTH = 16 * 5
OUTPUT_HEIGHT = 9 * 5


def main():
    if len(sys.argv) != 3:
        print("Usage:", sys.argv[0], "<input_data> <output_data>")
        exit(1)
    input_dir = sys.argv[1]
    output_dir = sys.argv[2]
    if not path.isdir(input_dir):
        print(input_dir, "does not exist")
        exit(1)
    if not path.isdir(output_dir):
        print(output_dir, "does not exist. creating now...")
        os.makedirs(output_dir)
    input_files = glob.glob(input_dir + "/*.mv")

    for input_file in input_files:
        mv_frame, label = load_mv(input_file)
        matrix = mv_frame.mvL0
        if matrix.shape[0] != OUTPUT_HEIGHT or matrix.shape[1] != OUTPUT_WIDTH:
            matrix = scipy.ndimage.zoom(matrix,
                                        (OUTPUT_HEIGHT / matrix.shape[0],
                                         OUTPUT_WIDTH / matrix.shape[1], 2))
        prefix, ext = os.path.basename(input_file).split(".")
        output_filename = prefix + ".mv"
        output_filename = os.path.join(output_dir, output_filename)
        mv = create_mv(matrix)
        dump_mv(mv, label, output_filename)
        print("data dumped to", output_filename)
        break


if __name__ == "__main__":
    main()
