//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017-20, Lawrence Livermore National Security, LLC
// and RAJA Performance Suite project contributors.
// See the RAJAPerf/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "MASS3DPA.hpp"

#include "RAJA/RAJA.hpp"

#include <iostream>

namespace rajaperf {
namespace apps {

#define D1D 4
#define Q1D 5
#define B_(x, y) B[x + Q1D * y]
#define Bt_(x, y) Bt[x + D1D * y]
#define X_(dx, dy, dz, e)                                                      \
  X[dx + D1D * dy + D1D * D1D * dz + D1D * D1D * D1D * e]
#define Y_(dx, dy, dz, e)                                                      \
  Y[dx + D1D * dy + D1D * D1D * dz + D1D * D1D * D1D * e]
#define D_(qx, qy, qz, e)                                                      \
  D[qx + Q1D * qy + Q1D * Q1D * qz + Q1D * Q1D * Q1D * e]

#define RAJA_DIRECT_PRAGMA(X) _Pragma(#X)
#define RAJA_UNROLL(N) RAJA_DIRECT_PRAGMA(unroll(N))
#define FOREACH_THREAD(i, k, N) for (int i = 0; i < N; i++)

void MASS3DPA::runOpenMPVariant(VariantID vid) {
  const Index_type run_reps = getRunReps();

  MASS3DPA_DATA_SETUP;

  switch (vid) {

  case Base_OpenMP: {

    startTimer();
    for (RepIndex_type irep = 0; irep < run_reps; ++irep) {

      for (int e = 0; e < NE; ++e) {

        FOREACH_THREAD(dy, y, D1D) {
          FOREACH_THREAD(dx, x, D1D) {
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; ++dz) {
              Xsmem[dz][dy][dx] = X_(dx, dy, dz, e);
            }
          }
          FOREACH_THREAD(dx, x, Q1D) { Bsmem[dx][dy] = B_(dx, dy); }
        }
        __syncthreads();
        FOREACH_THREAD(dy, y, D1D) {
          FOREACH_THREAD(qx, x, Q1D) {
            double u[D1D];
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; dz++) {
              u[dz] = 0;
            }
            RAJA_UNROLL(MD1)
            for (int dx = 0; dx < D1D; ++dx) {
              RAJA_UNROLL(MD1)
              for (int dz = 0; dz < D1D; ++dz) {
                u[dz] += Xsmem[dz][dy][dx] * Bsmem[qx][dx];
              }
            }
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; ++dz) {
              DDQ[dz][dy][qx] = u[dz];
            }
          }
        }
        __syncthreads();
        FOREACH_THREAD(qy, y, Q1D) {
          FOREACH_THREAD(qx, x, Q1D) {
            double u[D1D];
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; dz++) {
              u[dz] = 0;
            }
            RAJA_UNROLL(MD1)
            for (int dy = 0; dy < D1D; ++dy) {
              RAJA_UNROLL(MD1)
              for (int dz = 0; dz < D1D; dz++) {
                u[dz] += DDQ[dz][dy][qx] * Bsmem[qy][dy];
              }
            }
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; dz++) {
              DQQ[dz][qy][qx] = u[dz];
            }
          }
        }
        __syncthreads();
        FOREACH_THREAD(qy, y, Q1D) {
          FOREACH_THREAD(qx, x, Q1D) {
            double u[Q1D];

            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; qz++) {
              u[qz] = 0;
            }
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; ++dz) {
              RAJA_UNROLL(MQ1)
              for (int qz = 0; qz < Q1D; qz++) {
                u[qz] += DQQ[dz][qy][qx] * Bsmem[qz][dz];
              }
            }
            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; qz++) {
              QQQ[qz][qy][qx] = u[qz] * D_(qx, qy, qz, e);
            }
          }
        }

        __syncthreads();
        FOREACH_THREAD(d, y, D1D) {
          FOREACH_THREAD(q, x, Q1D) { Btsmem[d][q] = Bt_(q, d); }
        }

        __syncthreads();
        FOREACH_THREAD(qy, y, Q1D) {
          FOREACH_THREAD(dx, x, D1D) {
            double u[Q1D];
            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; ++qz) {
              u[qz] = 0;
            }
            RAJA_UNROLL(MQ1)
            for (int qx = 0; qx < Q1D; ++qx) {
              RAJA_UNROLL(MQ1)
              for (int qz = 0; qz < Q1D; ++qz) {
                u[qz] += QQQ[qz][qy][qx] * Btsmem[dx][qx];
              }
            }
            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; ++qz) {
              QQD[qz][qy][dx] = u[qz];
            }
          }
        }
        __syncthreads();

        FOREACH_THREAD(dy, y, D1D) {
          FOREACH_THREAD(dx, x, D1D) {
            double u[Q1D];
            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; ++qz) {
              u[qz] = 0;
            }
            RAJA_UNROLL(MQ1)
            for (int qy = 0; qy < Q1D; ++qy) {
              RAJA_UNROLL(MQ1)
              for (int qz = 0; qz < Q1D; ++qz) {
                u[qz] += QQD[qz][qy][dx] * Btsmem[dy][qy];
              }
            }
            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; ++qz) {
              QDD[qz][dy][dx] = u[qz];
            }
          }
        }

        __syncthreads();
        FOREACH_THREAD(dy, y, D1D) {
          FOREACH_THREAD(dx, x, D1D) {
            double u[D1D];
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; ++dz) {
              u[dz] = 0;
            }
            RAJA_UNROLL(MQ1)
            for (int qz = 0; qz < Q1D; ++qz) {
              RAJA_UNROLL(MD1)
              for (int dz = 0; dz < D1D; ++dz) {
                u[dz] += QDD[qz][dy][dx] * Btsmem[dz][qz];
              }
            }
            RAJA_UNROLL(MD1)
            for (int dz = 0; dz < D1D; ++dz) {
              Y_(dx, dy, dz, e) += u[dz];
            }
          }
        }

      } // element loop
    }
    stopTimer();

    break;
  }

  case RAJA_OpenMP: {

    return;
  }

  default:
    std::cout << "\n MASS3DPA : Unknown OpenMP variant id = " << vid
              << std::endl;
  }
}

} // end namespace apps
} // end namespace rajaperf
