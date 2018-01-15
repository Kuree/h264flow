import matplotlib.pyplot as plt
import numpy as np
import sys
import os


def angle_diff(v1, v2):
    v1_norm = np.linalg.norm(v1)
    v2_norm = np.linalg.norm(v2)
    if v1_norm == 0 or v2_norm == 0:
        return 0
    v1_u = v1 / v1_norm
    v2_u = v2 / v2_norm
    return np.arccos(np.clip(np.dot(v1_u, v2_u), -1.0, 1.0))


def compute_angle_diff(mvs, truth):
    result = []
    for frame in range(len(mvs)):
        mv = mvs[frame]
        t = truth[frame]
        entry = np.zeros((mv.shape[0], mv.shape[1]))
        for i in range(entry.shape[0]):
            for j in range(entry.shape[1]):
                entry[i][j] = angle_diff(mv[i][j], t[i][j])
        result.append(entry)
    return np.array(result)


def compute_norm_diff(mvs, truth):
    result = []
    for frame in range(len(mvs)):
        mv = mvs[frame]
        t = truth[frame]
        entry = np.zeros((mv.shape[0], mv.shape[1]))
        for i in range(entry.shape[0]):
            for j in range(entry.shape[1]):
                entry[i][j] = abs(np.linalg.norm(mv[i][j]) -
                                   np.linalg.norm(t[i][j]))
        result.append(entry)
    return np.array(result)


def get_data_files(dir_name):
    files = [f for f in os.listdir(dir_name) if
             os.path.isfile(os.path.join(dir_name, f)) and f.endswith(".npy")]
    return files


def get_mp4_name(filename):
    basename = os.path.basename(filename)
    name = basename.split(".")[0]
    name = name.replace("_", " ")
    return name.title()

def main():
    if len(sys.argv) != 2:
        print("Usage:", sys.argv[0], "<data_file>")
        print("    <data_file>: data_file data generated \
by opencv.py")

    f, axes = plt.subplots(2, 2, figsize=(9, 6),
                           gridspec_kw={'width_ratios': [3, 1]})

    ((ax1, ax2), (ax3, ax4)) = axes
    filename = sys.argv[1]
    data = np.load(filename)

    mvs = data[0]
    truth = data[1]

    truth_int = np.around(truth)

    angle_data = compute_angle_diff(mvs, truth)
    norm_data = compute_norm_diff(mvs, truth) * 16
    angle_data_int = compute_angle_diff(mvs, truth_int)
    norm_data_int = compute_norm_diff(mvs, truth_int) * 16

    angle_diff_data = angle_data.flatten()
    norm_diff_data = norm_data.flatten()
    angle_diff_int_data = angle_data_int.flatten()
    norm_diff_int_data = norm_data_int.flatten()

    weights = np.ones_like(angle_diff_data)/float(len(angle_diff_data))
    ax1.hist(angle_diff_data, 10, weights=weights)
    ax1.set_title("Angle difference")

    ax2.boxplot(norm_diff_data, showfliers=False)
    ax2.set_title("Magnitude difference")

    ax3.hist(angle_diff_int_data, 10, weights=weights)
    ax3.set_title("Angle difference (quantized)")

    ax4.boxplot(norm_diff_int_data, showfliers=False)
    ax4.set_title("Magnitude difference (quantized)")

    ax1.set_ylabel("Percentage %")
    ax1.set_xlabel("Radians")
    ax3.set_ylabel("Percentage %")
    ax3.set_xlabel("Radians")
    ax2.set_ylabel("Pixels")
    ax4.set_ylabel("Pixels")
    ax2.set_xticks([], [])
    ax4.set_xticks([], [])

    f.suptitle("H.264 Motion vectors compared with ground truth (Farneback). " +
               get_mp4_name(filename) + ": " +
               str(len(truth)) + " frames")

    f.tight_layout(rect=[0, 0.00, 1, 0.96])
    plt.show()


if __name__ == "__main__":
    main()
