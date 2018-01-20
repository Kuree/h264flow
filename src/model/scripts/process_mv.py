from h264flow import load_mv, dump_mv, create_mv
from os import path
import sys
import os
import scipy
import glob


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
        print(input_file)
        mv_frame, label = load_mv(input_file)
        matrix = mv_frame.mvL0
        print(matrix[0][0])
        break


if __name__ == "__main__":
    main()