// -----------------------------------------------------------------------------
//
// Copyright (C) The BioDynaMo Project.
// All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
//
// See the LICENSE file distributed with this work for details.
// See the NOTICE file distributed with this work for additional information
// regarding copyright ownership.
//
// -----------------------------------------------------------------------------

#include <omp.h>

#include "core/container/math_array.h"
#include "core/environment/environment.h"
#include "core/functor.h"
#include "core/resource_manager.h"
#include "core/sim_object/sim_object.h"

namespace bdm {

Environment::~Environment() {}

bool Environment::HasGrown() const { return has_grown_; }

struct Environment::SimDimensionAndLargestSimObjectFunctor
    : public Functor<void, SimObject*, SoHandle> {
  using Type = std::vector<std::array<double, 8>>;

  SimDimensionAndLargestSimObjectFunctor(Type& xmin, Type& xmax, Type& ymin,
                                         Type& ymax, Type& zmin, Type& zmax,
                                         Type& largest)
      : xmin_(xmin),
        xmax_(xmax),
        ymin_(ymin),
        ymax_(ymax),
        zmin_(zmin),
        zmax_(zmax),
        largest_(largest) {}

  void operator()(SimObject* so, SoHandle) override {
    auto tid = omp_get_thread_num();
    const auto& position = so->GetPosition();
    // x
    if (position[0] < xmin_[tid][0]) {
      xmin_[tid][0] = position[0];
    }
    if (position[0] > xmax_[tid][0]) {
      xmax_[tid][0] = position[0];
    }
    // y
    if (position[1] < ymin_[tid][0]) {
      ymin_[tid][0] = position[1];
    }
    if (position[1] > ymax_[tid][0]) {
      ymax_[tid][0] = position[1];
    }
    // z
    if (position[2] < zmin_[tid][0]) {
      zmin_[tid][0] = position[2];
    }
    if (position[2] > zmax_[tid][0]) {
      zmax_[tid][0] = position[2];
    }
    // largest object
    auto diameter = so->GetDiameter();
    if (diameter > largest_[tid][0]) {
      largest_[tid][0] = diameter;
    }
  }

  Type& xmin_;
  Type& xmax_;
  Type& ymin_;
  Type& ymax_;
  Type& zmin_;
  Type& zmax_;

  Type& largest_;
};

void Environment::CalcSimDimensionsAndLargestSimObject(
    std::array<double, 6>* ret_grid_dimensions, double* largest_so) {
  auto* rm = Simulation::GetActive()->GetResourceManager();

  const auto max_threads = omp_get_max_threads();
  // allocate version for each thread - avoid false sharing by padding them
  // assumes 64 byte cache lines (8 * sizeof(double))
  std::vector<std::array<double, 8>> xmin(max_threads, {{Math::kInfinity}});
  std::vector<std::array<double, 8>> xmax(max_threads, {{-Math::kInfinity}});

  std::vector<std::array<double, 8>> ymin(max_threads, {{Math::kInfinity}});
  std::vector<std::array<double, 8>> ymax(max_threads, {{-Math::kInfinity}});

  std::vector<std::array<double, 8>> zmin(max_threads, {{Math::kInfinity}});
  std::vector<std::array<double, 8>> zmax(max_threads, {{-Math::kInfinity}});

  std::vector<std::array<double, 8>> largest(max_threads, {{0}});

  SimDimensionAndLargestSimObjectFunctor functor(xmin, xmax, ymin, ymax, zmin,
                                                 zmax, largest);
  rm->ApplyOnAllElementsParallelDynamic(1000, functor);

  // reduce partial results into global one
  double& gxmin = (*ret_grid_dimensions)[0];
  double& gxmax = (*ret_grid_dimensions)[1];
  double& gymin = (*ret_grid_dimensions)[2];
  double& gymax = (*ret_grid_dimensions)[3];
  double& gzmin = (*ret_grid_dimensions)[4];
  double& gzmax = (*ret_grid_dimensions)[5];
  for (int tid = 0; tid < max_threads; tid++) {
    // x
    if (xmin[tid][0] < gxmin) {
      gxmin = xmin[tid][0];
    }
    if (xmax[tid][0] > gxmax) {
      gxmax = xmax[tid][0];
    }
    // y
    if (ymin[tid][0] < gymin) {
      gymin = ymin[tid][0];
    }
    if (ymax[tid][0] > gymax) {
      gymax = ymax[tid][0];
    }
    // z
    if (zmin[tid][0] < gzmin) {
      gzmin = zmin[tid][0];
    }
    if (zmax[tid][0] > gzmax) {
      gzmax = zmax[tid][0];
    }
    // larget object
    if (largest[tid][0] > (*largest_so)) {
      (*largest_so) = largest[tid][0];
    }
  }
}

}  // namespace bdm