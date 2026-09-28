#include "joint_pose_interpolation.h"

#include <cmath>
#include <cstdio>
#include <limits>

namespace {

constexpr double kPi = 3.14159265358979323846;

d2_pose::JointMatrix MakeZRotation(double degrees, float x, float y, float z) {
    const double radians = degrees * kPi / 180.0;
    const float cosine = static_cast<float>(std::cos(radians));
    const float sine = static_cast<float>(std::sin(radians));
    return {{cosine, -sine, 0.0f, x,
             sine, cosine, 0.0f, y,
             0.0f, 0.0f, 1.0f, z}};
}

bool Near(double actual, double expected, double tolerance = 1.0e-4) {
    return std::abs(actual - expected) <= tolerance;
}

bool Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
    }
    return condition;
}

}  // namespace

int main() {
    bool passed = true;
    const d2_pose::JointMatrix previous[2] = {
        MakeZRotation(0.0, 0.0f, 2.0f, 4.0f),
        MakeZRotation(170.0, -2.0f, 0.0f, 1.0f),
    };
    const d2_pose::JointMatrix current[2] = {
        MakeZRotation(90.0, 10.0f, 6.0f, 8.0f),
        MakeZRotation(-170.0, 2.0f, 4.0f, 3.0f),
    };
    d2_pose::JointMatrix output[2]{};
    passed &= Check(d2_pose::InterpolateJointPalettes(previous, current, 2,
                                                       0.5, output),
                    "valid two-joint palette was rejected");
    passed &= Check(Near(output[0].values[3], 5.0) &&
                        Near(output[0].values[7], 4.0) &&
                        Near(output[0].values[11], 6.0),
                    "translation midpoint is incorrect");
    const double rootAngle =
        std::atan2(output[0].values[4], output[0].values[0]) * 180.0 / kPi;
    passed &= Check(Near(rootAngle, 45.0, 1.0e-3),
                    "rotation midpoint is incorrect");
    passed &= Check(output[1].values[0] < -0.999f &&
                        std::abs(output[1].values[4]) < 1.0e-3f,
                    "quaternion interpolation did not take the shortest arc");

    d2_pose::JointMatrix invalid = previous[0];
    invalid.values[0] = 2.0f;
    d2_pose::JointMatrix sentinel =
        MakeZRotation(0.0, 123.0f, 0.0f, 0.0f);
    output[0] = sentinel;
    passed &= Check(!d2_pose::InterpolateJointPalettes(
                         &invalid, current, 1, 0.5, output) &&
                        output[0].values[3] == sentinel.values[3],
                    "non-rigid input modified output");
    invalid = previous[0];
    invalid.values[3] = std::numeric_limits<float>::quiet_NaN();
    passed &= Check(!d2_pose::InterpolateJointPalettes(
                         &invalid, current, 1, 0.5, output),
                    "non-finite input was accepted");
    passed &= Check(!d2_pose::InterpolateJointPalettes(
                         previous, current, 257, 0.5, output),
                    "oversized palette was accepted");

    if (!passed) {
        return 1;
    }
    std::puts("DOTO joint-pose interpolation tests passed.");
    return 0;
}
