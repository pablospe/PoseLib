// Copyright (c) 2020, Viktor Larsson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of the copyright holder nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "gp4ps.h"
#include <re3q3/re3q3.h>

namespace poselib {

// Solves for camera pose such that: scale*p+lambda*x = R*X+t
int gp4ps(const std::vector<Eigen::Vector3d> &p, const std::vector<Eigen::Vector3d> &x,
          const std::vector<Eigen::Vector3d> &X, std::vector<CameraPose> *output) {

    Eigen::Matrix<double, 8, 13> A;

    for (int i = 0; i < 4; ++i) {
        // xx = [x3 0 -x1; 0 x3 -x2]
        // eqs = [xx kron(X',xx), -xx*p] * [t; scale; vec(R)]

        A.row(2 * i) << x[i](2), 0.0, -x[i](0), -p[i](0) * x[i](2) + p[i](2) * x[i](0), X[i](0) * x[i](2), 0.0, -X[i](0) * x[i](0), X[i](1) * x[i](2), 0.0, -X[i](1) * x[i](0), X[i](2) * x[i](2), 0.0, -X[i](2) * x[i](0);
        A.row(2 * i + 1) << 0.0, x[i](2), -x[i](1), -p[i](1) * x[i](2) + p[i](2) * x[i](1), 0.0, X[i](0) * x[i](2), -X[i](0) * x[i](1), 0.0, X[i](1) * x[i](2), -X[i](1) * x[i](1), 0.0, X[i](2) * x[i](2), -X[i](2) * x[i](1);
    }

    Eigen::Matrix4d B = A.block<4, 4>(0, 0).inverse();

    Eigen::Matrix<double, 3, 9> AR = A.block<3, 9>(4, 4) - A.block<3, 4>(4, 0) * B * A.block<4, 9>(0, 4);
    Eigen::Matrix<double, 3, 10> coeffs;

    re3q3::rotation_to_3q3(AR, &coeffs);

    Eigen::Matrix<double, 3, 8> solutions;

    int n_sols = re3q3::re3q3(coeffs, &solutions);

    Eigen::Vector4d ts;
    for (int i = 0; i < n_sols; ++i) {
        CameraPose pose;
        re3q3::cayley_param(solutions.col(i), &pose.R);
        ts = -B * (A.block<4, 9>(0, 4) * Eigen::Map<Eigen::Matrix<double, 9, 1>>(pose.R.data()));
        pose.t = ts.block<3, 1>(0, 0);
        pose.alpha = ts(3);
        output->push_back(pose);
    }

    return n_sols;
}

} // namespace poselib