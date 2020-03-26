#include "benchmark.h"
#include "problem_generator.h"
#include <chrono>
#include <iomanip>
#include <iostream>
namespace poselib {

template <typename Solver>
BenchmarkResult benchmark(int n_problems, const ProblemOptions &options, double tol = 1e-6) {

  std::vector<ProblemInstance> problem_instances;
  generate_problems(n_problems, &problem_instances, options);

  BenchmarkResult result;
  result.instances_ = n_problems;
  result.name_ = Solver::name();
  result.options_ = options;

  // Run benchmark where we check solution quality
  for (const ProblemInstance &instance : problem_instances) {
    CameraPoseVector solutions;

    int sols = Solver::solve(instance, &solutions);

    double pose_error = std::numeric_limits<double>::max();

    result.solutions_ += sols;

    for (const CameraPose &pose : solutions) {
      if (Solver::validator::is_valid(instance, pose, tol))
        result.valid_solutions_++;

      pose_error = std::min(pose_error, Solver::validator::compute_pose_error(instance, pose));
    }

    if (pose_error < tol)
      result.found_gt_pose_++;
  }

  std::vector<long> runtimes;
  CameraPoseVector solutions;
  for (int iter = 0; iter < 10; ++iter) {
    int total_sols = 0;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (const ProblemInstance &instance : problem_instances) {
      solutions.clear();

      int sols = Solver::solve(instance, &solutions);

      total_sols += sols;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    runtimes.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());
  }

  std::sort(runtimes.begin(), runtimes.end());

  result.runtime_ns_ = runtimes[runtimes.size() / 2];
  return result;
}

} // namespace poselib

void print_runtime(double runtime_ns) {
  if (runtime_ns < 1e3) {
    std::cout << runtime_ns << " ns";
  } else if (runtime_ns < 1e6) {
    std::cout << runtime_ns / 1e3 << " us";
  } else if (runtime_ns < 1e9) {
    std::cout << runtime_ns / 1e6 << " ms";
  } else {
    std::cout << runtime_ns / 1e9 << " s";
  }
}

void display_result(std::vector<poselib::BenchmarkResult> &results) {

  int w = 13;
  // display header
  std::cout << std::setw(w) << "Solver";
  std::cout << std::setw(w) << "Solutions";
  std::cout << std::setw(w) << "Valid";
  std::cout << std::setw(w) << "GT found";
  std::cout << std::setw(w) << "Runtime"
            << "\n";
  for (int i = 0; i < w * 5; ++i)
    std::cout << "-";
  std::cout << "\n";

  int prec = 6;

  for (poselib::BenchmarkResult &result : results) {
    double num_tests = static_cast<double>(result.instances_);
    double solutions = result.solutions_ / num_tests;
    double valid_sols = result.valid_solutions_ / static_cast<double>(result.solutions_) * 100.0;
    double gt_found = result.found_gt_pose_ / num_tests * 100.0;
    double runtime_ns = result.runtime_ns_ / num_tests;

    std::cout << std::setprecision(prec) << std::setw(w) << result.name_;
    std::cout << std::setprecision(prec) << std::setw(w) << solutions;
    std::cout << std::setprecision(prec) << std::setw(w) << valid_sols;
    std::cout << std::setprecision(prec) << std::setw(w) << gt_found;
    std::cout << std::setprecision(prec) << std::setw(w - 3);
    print_runtime(runtime_ns);
    std::cout << "\n";
  }
}

int main() {

  std::vector<poselib::BenchmarkResult> results;

  poselib::ProblemOptions options;
  // options.camera_fov_ = 45; // Narrow
  // options.camera_fov_ = 75; // Medium
  options.camera_fov_ = 120; // Wide

  double tol = 1e-6;

  // P3P
  poselib::ProblemOptions p3p_opt = options;
  p3p_opt.n_point_point_ = 3;
  p3p_opt.n_point_line_ = 0;
  results.push_back(poselib::benchmark<poselib::SolverP3P>(1e5, p3p_opt, tol));

  // gP3P
  poselib::ProblemOptions gp3p_opt = options;
  gp3p_opt.n_point_point_ = 3;
  gp3p_opt.n_point_line_ = 0;
  gp3p_opt.generalized_ = true;
  results.push_back(poselib::benchmark<poselib::SolverGP3P>(1e4, gp3p_opt, tol));

  // gP4Ps
  poselib::ProblemOptions gp4p_opt = options;
  gp4p_opt.n_point_point_ = 4;
  gp4p_opt.n_point_line_ = 0;
  gp4p_opt.generalized_ = true;
  gp4p_opt.unknown_scale_ = true;
  results.push_back(poselib::benchmark<poselib::SolverGP4PS>(1e4, gp4p_opt, tol));

  // P4Pf
  poselib::ProblemOptions p4pf_opt = options;
  p4pf_opt.n_point_point_ = 4;
  p4pf_opt.n_point_line_ = 0;
  p4pf_opt.unknown_focal_ = true;
  results.push_back(poselib::benchmark<poselib::SolverP4PF>(1e4, p4pf_opt, tol));

  // P2P2L
  poselib::ProblemOptions p2p2l_opt = options;
  p2p2l_opt.n_point_point_ = 2;
  p2p2l_opt.n_point_line_ = 2;
  results.push_back(poselib::benchmark<poselib::SolverP2P2PL>(1e3, p2p2l_opt, tol));

  // P6LP
  poselib::ProblemOptions p6lp_opt = options;
  p6lp_opt.n_line_point_ = 6;
  results.push_back(poselib::benchmark<poselib::SolverP6LP>(1e4, p6lp_opt, tol));

  // P5LP Radial
  poselib::ProblemOptions p5lp_radial_opt = options;
  p5lp_radial_opt.n_line_point_ = 5;
  p5lp_radial_opt.radial_lines_ = true;
  results.push_back(poselib::benchmark<poselib::SolverP5LP_Radial>(1e5, p5lp_radial_opt, tol));

  // P2P1LL
  poselib::ProblemOptions p2p1ll_opt = options;
  p2p1ll_opt.n_point_point_ = 2;
  p2p1ll_opt.n_line_line_ = 1;
  results.push_back(poselib::benchmark<poselib::SolverP2P1LL>(1e4, p2p1ll_opt, tol));

  // P1P2LL
  poselib::ProblemOptions p1p2ll_opt = options;
  p1p2ll_opt.n_point_point_ = 1;
  p1p2ll_opt.n_line_line_ = 2;
  results.push_back(poselib::benchmark<poselib::SolverP1P2LL>(1e4, p1p2ll_opt, tol));

  // P3LL
  poselib::ProblemOptions p3ll_opt = options;
  p3ll_opt.n_line_line_ = 3;
  results.push_back(poselib::benchmark<poselib::SolverP3LL>(1e4, p3ll_opt, tol));

  // uP2P
  poselib::ProblemOptions up2p_opt = options;
  up2p_opt.n_point_point_ = 2;
  up2p_opt.n_point_line_ = 0;
  up2p_opt.upright_ = true;
  results.push_back(poselib::benchmark<poselib::SolverUP2P>(1e6, up2p_opt, tol));

  // uGP2P
  poselib::ProblemOptions ugp2p_opt = options;
  ugp2p_opt.n_point_point_ = 2;
  ugp2p_opt.n_point_line_ = 0;
  ugp2p_opt.upright_ = true;
  ugp2p_opt.generalized_ = true;
  results.push_back(poselib::benchmark<poselib::SolverUGP2P>(1e6, ugp2p_opt, tol));

  // uGP3Ps
  poselib::ProblemOptions ugp3ps_opt = options;
  ugp3ps_opt.n_point_point_ = 3;
  ugp3ps_opt.n_point_line_ = 0;
  ugp3ps_opt.upright_ = true;
  ugp3ps_opt.generalized_ = true;
  ugp3ps_opt.unknown_scale_ = true;
  results.push_back(poselib::benchmark<poselib::SolverUGP3PS>(1e6, ugp3ps_opt, tol));

  // uP1P2L
  poselib::ProblemOptions up1p2l_opt = options;
  up1p2l_opt.n_point_point_ = 1;
  up1p2l_opt.n_point_line_ = 2;
  up1p2l_opt.upright_ = true;
  results.push_back(poselib::benchmark<poselib::SolverUP1P2PL>(1e5, up1p2l_opt, tol));

  // uP4L
  poselib::ProblemOptions up4l_opt = options;
  up4l_opt.n_point_point_ = 0;
  up4l_opt.n_point_line_ = 4;
  up4l_opt.upright_ = true;
  results.push_back(poselib::benchmark<poselib::SolverUP4PL>(1e3, up4l_opt, tol));

  display_result(results);

  return 0;
}