#include <algorithm>
#include <limits>

#include "chrono_parallel/ChConfigParallel.h"
#include "chrono_parallel/constraints/ChConstraintRigidRigid.h"
#include "chrono_parallel/math/quartic.h"
using namespace chrono;

void chrono::Orthogonalize(real3& Vx, real3& Vy, real3& Vz) {
  real3 mVsingular = R3(0, 1, 0);
  Vz = cross(Vx, mVsingular);
  real mzlen = Vz.length();
  // was near singularity? change singularity reference vector!
  if (mzlen < real(0.0001)) {
    mVsingular = R3(1, 0, 0);
    Vz = cross(Vx, mVsingular);
    mzlen = Vz.length();
  }
  Vz = Vz / mzlen;
  Vy = cross(Vz, Vx);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool Cone_generalized(real& gamma_n, real& gamma_u, real& gamma_v, const real& mu) {
  real f_tang = sqrt(gamma_u * gamma_u + gamma_v * gamma_v);

  // inside upper cone? keep untouched!
  if (f_tang < (mu * gamma_n)) {
    return false;
  }

  // inside lower cone? reset  normal,u,v to zero!
  if ((f_tang) < -(1.0 / mu) * gamma_n || (fabs(gamma_n) < 10e-15)) {
    gamma_n = 0;
    gamma_u = 0;
    gamma_v = 0;
    return false;
  }

  // remaining case: project orthogonally to generator segment of upper cone
  gamma_n = (f_tang * mu + gamma_n) / (mu * mu + 1);
  real tproj_div_t = (gamma_n * mu) / f_tang;
  gamma_u *= tproj_div_t;
  gamma_v *= tproj_div_t;

  return true;
}

void Cone_single(real& gamma_n, real& gamma_s, const real& mu) {
  real f_tang = fabs(gamma_s);

  // inside upper cone? keep untouched!
  if (f_tang < (mu * gamma_n)) {
    return;
  }

  // inside lower cone? reset  normal,u,v to zero!
  if ((f_tang) < -(1.0 / mu) * gamma_n || (fabs(gamma_n) < 10e-15)) {
    gamma_n = 0;
    gamma_s = 0;
    return;
  }

  // remaining case: project orthogonally to generator segment of upper cone
  gamma_n = (f_tang * mu + gamma_n) / (mu * mu + 1);
  real tproj_div_t = (gamma_n * mu) / f_tang;
  gamma_s *= tproj_div_t;
}

void ChConstraintRigidRigid::func_Project_normal(int index, const int2* ids, const real* cohesion, real* gamma) {

  real gamma_x = gamma[index * 1 + 0];
  int2 body_id = ids[index];
  real coh = cohesion[index];

  gamma_x += coh;
  gamma_x = gamma_x < 0 ? 0 : gamma_x - coh;
  gamma[index * 1 + 0] = gamma_x;
  if (data_container->settings.solver.solver_mode == SLIDING) {
    gamma[data_container->num_contacts + index * 2 + 0] = 0;
    gamma[data_container->num_contacts + index * 2 + 1] = 0;
  }
  if (data_container->settings.solver.solver_mode == SPINNING) {
    gamma[3 * data_container->num_contacts + index * 3 + 0] = 0;
    gamma[3 * data_container->num_contacts + index * 3 + 1] = 0;
    gamma[3 * data_container->num_contacts + index * 3 + 2] = 0;
  }
}

void ChConstraintRigidRigid::func_Project_sliding(int index, const int2* ids, const real3* fric, const real* cohesion, real* gam) {
  real3 gamma;
  gamma.x = gam[index * 1 + 0];
  gamma.y = gam[data_container->num_contacts + index * 2 + 0];
  gamma.z = gam[data_container->num_contacts + index * 2 + 1];

  if (data_container->settings.solver.solver_mode == SPINNING) {
    gamma[3 * data_container->num_contacts + index * 3 + 0] = 0;
    gamma[3 * data_container->num_contacts + index * 3 + 1] = 0;
    gamma[3 * data_container->num_contacts + index * 3 + 2] = 0;
  }

  real coh = cohesion[index];
  gamma.x += coh;

  real mu = fric[index].x;
  if (mu == 0) {
    gamma.x = gamma.x < 0 ? 0 : gamma.x - coh;
    gamma.y = gamma.z = 0;

    gam[index * 1 + 0] = gamma.x;
    gam[data_container->num_contacts + index * 2 + 0] = gamma.y;
    gam[data_container->num_contacts + index * 2 + 1] = gamma.z;

    return;
  }

  if (Cone_generalized(gamma.x, gamma.y, gamma.z, mu)) {
  }

  gam[index * 1 + 0] = gamma.x - coh;
  gam[data_container->num_contacts + index * 2 + 0] = gamma.y;
  gam[data_container->num_contacts + index * 2 + 1] = gamma.z;
}
void ChConstraintRigidRigid::func_Project_spinning(int index, const int2* ids, const real3* fric, real* gam) {
  // real3 gamma_roll = R3(0);
  real rollingfriction = fric[index].y;
  real spinningfriction = fric[index].z;

  //	if(rollingfriction||spinningfriction){
  //		gam[index + number_of_contacts * 1] = 0;
  //		gam[index + number_of_contacts * 2] = 0;
  //	}

  real gamma_n = fabs(gam[index * 1 + 0]);
  real gamma_s  = gam[3 * data_container->num_contacts + index * 3 + 0];
  real gamma_tu = gam[3 * data_container->num_contacts + index * 3 + 1];
  real gamma_tv = gam[3 * data_container->num_contacts + index * 3 + 2];

  if (spinningfriction == 0) {
    gamma_s = 0;

  } else {
    Cone_single(gamma_n, gamma_s, spinningfriction);
  }

  if (rollingfriction == 0) {
    gamma_tu = 0;
    gamma_tv = 0;
    //		if (gamma_n < 0) {
    //			gamma_n = 0;
    //		}
  } else {
    Cone_generalized(gamma_n, gamma_tu, gamma_tv, rollingfriction);
  }
  // gam[index + number_of_contacts * 0] = gamma_n;
  gam[3 * data_container->num_contacts + index * 3 + 0] = gamma_s;
  gam[3 * data_container->num_contacts + index * 3 + 1] = gamma_tu;
  gam[3 * data_container->num_contacts + index * 3 + 2] = gamma_tv;
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void ChConstraintRigidRigid::host_Project_single(int index, int2* ids, real3* friction, real* cohesion, real* gamma) {
  // always project normal
  switch (data_container->settings.solver.local_solver_mode) {
    case NORMAL: {
      func_Project_normal(index, ids, cohesion, gamma);
    } break;

    case SLIDING: {
      func_Project_sliding(index, ids, friction, cohesion, gamma);
    } break;

    case SPINNING: {
      func_Project_sliding(index, ids, friction, cohesion, gamma);
      func_Project_spinning(index, ids, friction, gamma);
    } break;
  }
}

void ChConstraintRigidRigid::Project(real* gamma) {
  const thrust::host_vector<int2> & bids = data_container->host_data.bids_rigid_rigid;
  const thrust::host_vector<real3> & friction = data_container->host_data.fric_rigid_rigid;
  const thrust::host_vector<real> & cohesion = data_container->host_data.coh_rigid_rigid;

  switch (data_container->settings.solver.local_solver_mode) {
    case NORMAL: {
#pragma omp parallel for
      for (int index = 0; index < data_container->num_contacts; index++) {
        func_Project_normal(index, bids.data(), cohesion.data(), gamma);
      }
    } break;

    case SLIDING: {
#pragma omp parallel for
      for (int index = 0; index < data_container->num_contacts; index++) {
        func_Project_sliding(index, bids.data(), friction.data(), cohesion.data(), gamma);
      }
    } break;

    case SPINNING: {
#pragma omp parallel for
      for (int index = 0; index < data_container->num_contacts; index++) {
        func_Project_sliding(index, bids.data(), friction.data(), cohesion.data(), gamma);
        func_Project_spinning(index, bids.data(), friction.data(), gamma);
      }
    } break;
  }
}
void ChConstraintRigidRigid::Project_Single(int index, real* gamma) {

  thrust::host_vector<int2>& bids = data_container->host_data.bids_rigid_rigid;
  thrust::host_vector<real3>& friction = data_container->host_data.fric_rigid_rigid;
  thrust::host_vector<real>& cohesion = data_container->host_data.coh_rigid_rigid;

  host_Project_single(index, bids.data(), friction.data(), cohesion.data(), gamma);
}

void chrono::Compute_Jacobian(const real4& quat, const real3& U, const real3& V, const real3& W, const real3& point, real3& T1, real3& T2, real3& T3) {
  real4 quaternion_conjugate = ~quat;
  real3 sbar = quatRotate(point, quaternion_conjugate);

  T1 = cross(quatRotate(U, quaternion_conjugate), sbar);
  T2 = cross(quatRotate(V, quaternion_conjugate), sbar);
  T3 = cross(quatRotate(W, quaternion_conjugate), sbar);
}
void chrono::Compute_Jacobian_Rolling(const real4& quat, const real3& U, const real3& V, const real3& W, real3& T1, real3& T2, real3& T3) {
  real4 quaternion_conjugate = ~quat;

  T1 = quatRotate(U, quaternion_conjugate);
  T2 = quatRotate(V, quaternion_conjugate);
  T3 = quatRotate(W, quaternion_conjugate);
}

void ChConstraintRigidRigid::Build_b()
{
  if (data_container->num_contacts <= 0) {
    return;
  }

#pragma omp parallel for
  for (int index = 0; index < data_container->num_contacts; index++) {
    real bi = 0;
    real depth = data_container->host_data.dpth_rigid_rigid[index];

    if (data_container->settings.solver.alpha > 0) {
      bi = inv_hpa * depth;
    } else if (data_container->settings.solver.contact_recovery_speed < 0) {
      bi = inv_h * depth;
    } else {
      bi = std::max(inv_h * depth, -data_container->settings.solver.contact_recovery_speed);
    }


    data_container->host_data.b[index * 1 + 0] = bi;
  }
}

void ChConstraintRigidRigid::Build_s() {
  if (data_container->num_contacts <= 0) {
    return;
  }

  if (data_container->settings.solver.solver_mode == NORMAL) {
    return;
  }

  int2* ids = data_container->host_data.bids_rigid_rigid.data();
  const CompressedMatrix<real>& D_t_T = data_container->host_data.D_t_T;
  DynamicVector<real> v_new;

  const DynamicVector<real>& M_invk = data_container->host_data.M_invk;
  const DynamicVector<real>& gamma = data_container->host_data.gamma;

  const CompressedMatrix<real>& M_invD_n = data_container->host_data.M_invD_n;
  const CompressedMatrix<real>& M_invD_t = data_container->host_data.M_invD_t;
  const CompressedMatrix<real>& M_invD_s = data_container->host_data.M_invD_s;
  const CompressedMatrix<real>& M_invD_b = data_container->host_data.M_invD_b;

  uint num_contacts = data_container->num_contacts;
    uint num_unilaterals = data_container->num_unilaterals;
    uint num_bilaterals = data_container->num_bilaterals;

  blaze::DenseSubvector<const DynamicVector<real> > gamma_b = blaze::subvector(gamma, num_unilaterals, num_bilaterals);
  blaze::DenseSubvector<const DynamicVector<real> > gamma_n = blaze::subvector(gamma, 0, num_contacts);

  // Compute new velocity based on the lagrange multipliers
  switch (data_container->settings.solver.solver_mode) {
    case NORMAL: {
      v_new = M_invk + M_invD_n * gamma_n + M_invD_b * gamma_b;
    } break;

    case SLIDING: {
      blaze::DenseSubvector<const DynamicVector<real> > gamma_t = blaze::subvector(gamma, num_contacts, num_contacts * 2);

      v_new = M_invk + M_invD_n * gamma_n + M_invD_t * gamma_t + M_invD_b * gamma_b;

    } break;

    case SPINNING: {
      blaze::DenseSubvector<const DynamicVector<real> > gamma_t = blaze::subvector(gamma, num_contacts, num_contacts * 2);
      blaze::DenseSubvector<const DynamicVector<real> > gamma_s = blaze::subvector(gamma, num_contacts * 3, num_contacts * 3);

      v_new = M_invk + M_invD_n * gamma_n + M_invD_t * gamma_t + M_invD_s * gamma_s + M_invD_b * gamma_b;

    } break;
  }

#pragma omp parallel for
  for (int index = 0; index < data_container->num_contacts; index++) {
    real fric = data_container->host_data.fric_rigid_rigid[index].x;
    int2 body_id = ids[index];

    real s_v = D_t_T(index * 2 + 0, body_id.x * 6 + 0) * +v_new[body_id.x * 6 + 0] + D_t_T(index * 2 + 0, body_id.x * 6 + 1) * +v_new[body_id.x * 6 + 1] +
               D_t_T(index * 2 + 0, body_id.x * 6 + 2) * +v_new[body_id.x * 6 + 2] + D_t_T(index * 2 + 0, body_id.x * 6 + 3) * +v_new[body_id.x * 6 + 3] +
               D_t_T(index * 2 + 0, body_id.x * 6 + 4) * +v_new[body_id.x * 6 + 4] + D_t_T(index * 2 + 0, body_id.x * 6 + 5) * +v_new[body_id.x * 6 + 5] +

               D_t_T(index * 2 + 0, body_id.y * 6 + 0) * +v_new[body_id.y * 6 + 0] + D_t_T(index * 2 + 0, body_id.y * 6 + 1) * +v_new[body_id.y * 6 + 1] +
               D_t_T(index * 2 + 0, body_id.y * 6 + 2) * +v_new[body_id.y * 6 + 2] + D_t_T(index * 2 + 0, body_id.y * 6 + 3) * +v_new[body_id.y * 6 + 3] +
               D_t_T(index * 2 + 0, body_id.y * 6 + 4) * +v_new[body_id.y * 6 + 4] + D_t_T(index * 2 + 0, body_id.y * 6 + 5) * +v_new[body_id.y * 6 + 5];

    real s_w = D_t_T(index * 2 + 1, body_id.x * 6 + 0) * +v_new[body_id.x * 6 + 0] + D_t_T(index * 2 + 1, body_id.x * 6 + 1) * +v_new[body_id.x * 6 + 1] +
               D_t_T(index * 2 + 1, body_id.x * 6 + 2) * +v_new[body_id.x * 6 + 2] + D_t_T(index * 2 + 1, body_id.x * 6 + 3) * +v_new[body_id.x * 6 + 3] +
               D_t_T(index * 2 + 1, body_id.x * 6 + 4) * +v_new[body_id.x * 6 + 4] + D_t_T(index * 2 + 1, body_id.x * 6 + 5) * +v_new[body_id.x * 6 + 5] +

               D_t_T(index * 2 + 1, body_id.y * 6 + 0) * +v_new[body_id.y * 6 + 0] + D_t_T(index * 2 + 1, body_id.y * 6 + 1) * +v_new[body_id.y * 6 + 1] +
               D_t_T(index * 2 + 1, body_id.y * 6 + 2) * +v_new[body_id.y * 6 + 2] + D_t_T(index * 2 + 1, body_id.y * 6 + 3) * +v_new[body_id.y * 6 + 3] +
               D_t_T(index * 2 + 1, body_id.y * 6 + 4) * +v_new[body_id.y * 6 + 4] + D_t_T(index * 2 + 1, body_id.y * 6 + 5) * +v_new[body_id.y * 6 + 5];

    data_container->host_data.s[index * 1 + 0] = sqrt(s_v * s_v + s_w * s_w) * fric;
  }
}

void ChConstraintRigidRigid::Build_E()
{
  if (data_container->num_contacts <= 0) {
    return;
  }
  SOLVERMODE solver_mode = data_container->settings.solver.solver_mode;
  DynamicVector<real>& E = data_container->host_data.E;
  uint num_contacts = data_container->num_contacts;

#pragma omp parallel for
  for (int index = 0; index < data_container->num_contacts; index++) {
    int2 body = data_container->host_data.bids_rigid_rigid[index];

    real4 cA = data_container->host_data.compliance_data[body.x];
    real4 cB = data_container->host_data.compliance_data[body.y];
    real4 compliance;

    compliance.x = (cA.x == 0 || cB.x == 0) ? 0 : (cA.x + cB.x) * .5;
    compliance.y = (cA.y == 0 || cB.y == 0) ? 0 : (cA.y + cB.y) * .5;
    compliance.z = (cA.z == 0 || cB.z == 0) ? 0 : (cA.z + cB.z) * .5;
    compliance.w = (cA.w == 0 || cB.w == 0) ? 0 : (cA.w + cB.w) * .5;

    E[index * 1 + 0] = compliance.x;    // normal
    if (solver_mode == SLIDING) {
      E[num_contacts + index * 2 + 0] = compliance.y;    // sliding
      E[num_contacts + index * 2 + 1] = compliance.y;    // sliding
    } else if (solver_mode == SPINNING) {
      E[3 * num_contacts + index * 3 + 0] = compliance.w;    // sliding
      E[3 * num_contacts + index * 3 + 1] = compliance.z;    // sliding
      E[3 * num_contacts + index * 3 + 2] = compliance.z;    // sliding
    }
  }
}

void ChConstraintRigidRigid::Build_D()
{
  real3* norm = data_container->host_data.norm_rigid_rigid.data();
  real3* ptA = data_container->host_data.cpta_rigid_rigid.data();
  real3* ptB = data_container->host_data.cptb_rigid_rigid.data();
  real3* pos_data = data_container->host_data.pos_data.data();
  int2* ids = data_container->host_data.bids_rigid_rigid.data();
  real4* rot = contact_rotation.data();

  CompressedMatrix<real>& D_n_T = data_container->host_data.D_n_T;
  CompressedMatrix<real>& D_t_T = data_container->host_data.D_t_T;
  CompressedMatrix<real>& D_s_T = data_container->host_data.D_s_T;

  SOLVERMODE solver_mode = data_container->settings.solver.solver_mode;

#pragma omp parallel for
  for (int index = 0; index < data_container->num_contacts; index++) {
    real3 U = norm[index], V, W;
    real3 T3, T4, T5, T6, T7, T8;
    real3 TA, TB, TC;
    real3 TD, TE, TF;

    Orthogonalize(U, V, W);

    int2 body_id = ids[index];

    int row = index;
    // The position is subtracted here now instead of performing it in the narrowphase
    Compute_Jacobian(rot[index], U, V, W, ptA[index] - pos_data[body_id.x], T3, T4, T5);
    Compute_Jacobian(rot[index + data_container->num_contacts], U, V, W, ptB[index] - pos_data[body_id.y], T6, T7, T8);

    D_n_T(row * 1 + 0, body_id.x * 6 + 0) = -U.x;
    D_n_T(row * 1 + 0, body_id.x * 6 + 1) = -U.y;
    D_n_T(row * 1 + 0, body_id.x * 6 + 2) = -U.z;

    D_n_T(row * 1 + 0, body_id.x * 6 + 3) = T3.x;
    D_n_T(row * 1 + 0, body_id.x * 6 + 4) = T3.y;
    D_n_T(row * 1 + 0, body_id.x * 6 + 5) = T3.z;

    D_n_T(row * 1 + 0, body_id.y * 6 + 0) = U.x;
    D_n_T(row * 1 + 0, body_id.y * 6 + 1) = U.y;
    D_n_T(row * 1 + 0, body_id.y * 6 + 2) = U.z;

    D_n_T(row * 1 + 0, body_id.y * 6 + 3) = -T6.x;
    D_n_T(row * 1 + 0, body_id.y * 6 + 4) = -T6.y;
    D_n_T(row * 1 + 0, body_id.y * 6 + 5) = -T6.z;

    if (solver_mode == SLIDING || solver_mode == SPINNING) {
      D_t_T(row * 2 + 0, body_id.x * 6 + 0) = -V.x;
      D_t_T(row * 2 + 0, body_id.x * 6 + 1) = -V.y;
      D_t_T(row * 2 + 0, body_id.x * 6 + 2) = -V.z;

      D_t_T(row * 2 + 0, body_id.x * 6 + 3) = T4.x;
      D_t_T(row * 2 + 0, body_id.x * 6 + 4) = T4.y;
      D_t_T(row * 2 + 0, body_id.x * 6 + 5) = T4.z;

      D_t_T(row * 2 + 1, body_id.x * 6 + 0) = -W.x;
      D_t_T(row * 2 + 1, body_id.x * 6 + 1) = -W.y;
      D_t_T(row * 2 + 1, body_id.x * 6 + 2) = -W.z;

      D_t_T(row * 2 + 1, body_id.x * 6 + 3) = T5.x;
      D_t_T(row * 2 + 1, body_id.x * 6 + 4) = T5.y;
      D_t_T(row * 2 + 1, body_id.x * 6 + 5) = T5.z;

      D_t_T(row * 2 + 0, body_id.y * 6 + 0) = V.x;
      D_t_T(row * 2 + 0, body_id.y * 6 + 1) = V.y;
      D_t_T(row * 2 + 0, body_id.y * 6 + 2) = V.z;

      D_t_T(row * 2 + 0, body_id.y * 6 + 3) = -T7.x;
      D_t_T(row * 2 + 0, body_id.y * 6 + 4) = -T7.y;
      D_t_T(row * 2 + 0, body_id.y * 6 + 5) = -T7.z;

      D_t_T(row * 2 + 1, body_id.y * 6 + 0) = W.x;
      D_t_T(row * 2 + 1, body_id.y * 6 + 1) = W.y;
      D_t_T(row * 2 + 1, body_id.y * 6 + 2) = W.z;

      D_t_T(row * 2 + 1, body_id.y * 6 + 3) = -T8.x;
      D_t_T(row * 2 + 1, body_id.y * 6 + 4) = -T8.y;
      D_t_T(row * 2 + 1, body_id.y * 6 + 5) = -T8.z;
    }

    if (solver_mode == SPINNING) {
      Compute_Jacobian_Rolling(rot[index], U, V, W, TA, TB, TC);
      Compute_Jacobian_Rolling(rot[index + data_container->num_contacts], U, V, W, TD, TE, TF);

      D_s_T(row * 3 + 0, body_id.x * 6 + 3) = -TA.x;
      D_s_T(row * 3 + 0, body_id.x * 6 + 4) = -TA.y;
      D_s_T(row * 3 + 0, body_id.x * 6 + 5) = -TA.z;

      D_s_T(row * 3 + 1, body_id.x * 6 + 3) = -TB.x;
      D_s_T(row * 3 + 1, body_id.x * 6 + 4) = -TB.y;
      D_s_T(row * 3 + 1, body_id.x * 6 + 5) = -TB.z;

      D_s_T(row * 3 + 2, body_id.x * 6 + 3) = -TC.x;
      D_s_T(row * 3 + 2, body_id.x * 6 + 4) = -TC.y;
      D_s_T(row * 3 + 2, body_id.x * 6 + 5) = -TC.z;

      D_s_T(row * 3 + 0, body_id.y * 6 + 3) = TD.x;
      D_s_T(row * 3 + 0, body_id.y * 6 + 4) = TD.y;
      D_s_T(row * 3 + 0, body_id.y * 6 + 5) = TD.z;

      D_s_T(row * 3 + 1, body_id.y * 6 + 3) = TE.x;
      D_s_T(row * 3 + 1, body_id.y * 6 + 4) = TE.y;
      D_s_T(row * 3 + 1, body_id.y * 6 + 5) = TE.z;

      D_s_T(row * 3 + 2, body_id.y * 6 + 3) = TF.x;
      D_s_T(row * 3 + 2, body_id.y * 6 + 4) = TF.y;
      D_s_T(row * 3 + 2, body_id.y * 6 + 5) = TF.z;
    }
  }
}

void ChConstraintRigidRigid::GenerateSparsity()
{
  SOLVERMODE solver_mode = data_container->settings.solver.solver_mode;

  CompressedMatrix<real>& D_n_T = data_container->host_data.D_n_T;
  CompressedMatrix<real>& D_t_T = data_container->host_data.D_t_T;
  CompressedMatrix<real>& D_s_T = data_container->host_data.D_s_T;

  const int2* ids = data_container->host_data.bids_rigid_rigid.data();

  for (int index = 0; index < data_container->num_contacts; index++) {
    int2 body_id = ids[index];
    int row = index;

    D_n_T.append(row * 1 + 0, body_id.x * 6 + 0, 1);
    D_n_T.append(row * 1 + 0, body_id.x * 6 + 1, 1);
    D_n_T.append(row * 1 + 0, body_id.x * 6 + 2, 1);

    D_n_T.append(row * 1 + 0, body_id.x * 6 + 3, 1);
    D_n_T.append(row * 1 + 0, body_id.x * 6 + 4, 1);
    D_n_T.append(row * 1 + 0, body_id.x * 6 + 5, 1);

    D_n_T.append(row * 1 + 0, body_id.y * 6 + 0, 1);
    D_n_T.append(row * 1 + 0, body_id.y * 6 + 1, 1);
    D_n_T.append(row * 1 + 0, body_id.y * 6 + 2, 1);

    D_n_T.append(row * 1 + 0, body_id.y * 6 + 3, 1);
    D_n_T.append(row * 1 + 0, body_id.y * 6 + 4, 1);
    D_n_T.append(row * 1 + 0, body_id.y * 6 + 5, 1);

    D_n_T.finalize(row * 1 + 0);

    if (solver_mode == SLIDING || solver_mode == SPINNING) {
      D_t_T.append(row * 2 + 0, body_id.x * 6 + 0, 1);
      D_t_T.append(row * 2 + 0, body_id.x * 6 + 1, 1);
      D_t_T.append(row * 2 + 0, body_id.x * 6 + 2, 1);

      D_t_T.append(row * 2 + 0, body_id.x * 6 + 3, 1);
      D_t_T.append(row * 2 + 0, body_id.x * 6 + 4, 1);
      D_t_T.append(row * 2 + 0, body_id.x * 6 + 5, 1);

      D_t_T.append(row * 2 + 0, body_id.y * 6 + 0, 1);
      D_t_T.append(row * 2 + 0, body_id.y * 6 + 1, 1);
      D_t_T.append(row * 2 + 0, body_id.y * 6 + 2, 1);

      D_t_T.append(row * 2 + 0, body_id.y * 6 + 3, 1);
      D_t_T.append(row * 2 + 0, body_id.y * 6 + 4, 1);
      D_t_T.append(row * 2 + 0, body_id.y * 6 + 5, 1);

      D_t_T.finalize(row * 2 + 0);

      D_t_T.append(row * 2 + 1, body_id.x * 6 + 0, 1);
      D_t_T.append(row * 2 + 1, body_id.x * 6 + 1, 1);
      D_t_T.append(row * 2 + 1, body_id.x * 6 + 2, 1);

      D_t_T.append(row * 2 + 1, body_id.x * 6 + 3, 1);
      D_t_T.append(row * 2 + 1, body_id.x * 6 + 4, 1);
      D_t_T.append(row * 2 + 1, body_id.x * 6 + 5, 1);

      D_t_T.append(row * 2 + 1, body_id.y * 6 + 0, 1);
      D_t_T.append(row * 2 + 1, body_id.y * 6 + 1, 1);
      D_t_T.append(row * 2 + 1, body_id.y * 6 + 2, 1);

      D_t_T.append(row * 2 + 1, body_id.y * 6 + 3, 1);
      D_t_T.append(row * 2 + 1, body_id.y * 6 + 4, 1);
      D_t_T.append(row * 2 + 1, body_id.y * 6 + 5, 1);

      D_t_T.finalize(row * 2 + 1);
    }

    if (solver_mode == SPINNING) {
      D_s_T.append(row * 3 + 0, body_id.x * 6 + 3, 1);
      D_s_T.append(row * 3 + 0, body_id.x * 6 + 4, 1);
      D_s_T.append(row * 3 + 0, body_id.x * 6 + 5, 1);

      D_s_T.append(row * 3 + 0, body_id.y * 6 + 3, 1);
      D_s_T.append(row * 3 + 0, body_id.y * 6 + 4, 1);
      D_s_T.append(row * 3 + 0, body_id.y * 6 + 5, 1);

      D_s_T.finalize(row * 3 + 0);

      D_s_T.append(row * 3 + 1, body_id.x * 6 + 3, 1);
      D_s_T.append(row * 3 + 1, body_id.x * 6 + 4, 1);
      D_s_T.append(row * 3 + 1, body_id.x * 6 + 5, 1);

      D_s_T.append(row * 3 + 1, body_id.y * 6 + 3, 1);
      D_s_T.append(row * 3 + 1, body_id.y * 6 + 4, 1);
      D_s_T.append(row * 3 + 1, body_id.y * 6 + 5, 1);

      D_s_T.finalize(row * 3 + 1);

      D_s_T.append(row * 3 + 2, body_id.x * 6 + 3, 1);
      D_s_T.append(row * 3 + 2, body_id.x * 6 + 4, 1);
      D_s_T.append(row * 3 + 2, body_id.x * 6 + 5, 1);

      D_s_T.append(row * 3 + 2, body_id.y * 6 + 3, 1);
      D_s_T.append(row * 3 + 2, body_id.y * 6 + 4, 1);
      D_s_T.append(row * 3 + 2, body_id.y * 6 + 5, 1);

      D_s_T.finalize(row * 3 + 2);
    }
  }
}

typedef blaze::CompressedMatrix<real>  SpMat;

using blaze::SparseSubmatrix;


void compute_roots(real* Poly, int& nbRealRacines, real* Racines) {
  real r[3][5];
  // Roots of poly p[0] x^4 + p[1] x^3...+p[4]=0
  // x=r[1][k] + i r[2][k]  k=1,...,4

  if (fabs(Poly[1] / Poly[0]) > 1e7) {
    Poly[0] = 0.0;
  }
  int degp1 = 5;
  if (Poly[0] != 0.0)
    BIQUADROOTS(Poly, r);
  else if (Poly[1] != 0.0) {
    CUBICROOTS(Poly + 1, r);
    degp1 = 4;
  } else if (Poly[2] != 0.0) {
    QUADROOTS(Poly + 2, r);
    degp1 = 3;
  } else if (Poly[3] != 0.0) {
    r[1][1] = -Poly[0] / Poly[3];
    r[2][1] = 0;
    degp1 = 2;
  } else {
    printf("Enumerative Contact: degree of polynomial is 0.");
    degp1 = 1;
  }
  (nbRealRacines) = 0;
  for (int k = 1; k < degp1; k++) {
    if (fabs(r[2][k]) < 1e-10) {
      Racines[nbRealRacines] = r[1][k];
      nbRealRacines++;
    }
  }
}

/*ax+by+c=0
  a1x+b1y+c1=0*/
void solve2x2(real& a, real& b, real& c, real& a1, real& b1, real& c1, real& x, real& y) {
  real delta = a * b1 - a1 * b;
  if (delta == 0) {
    x = std::numeric_limits<real>::quiet_NaN();
    y = std::numeric_limits<real>::quiet_NaN();
  } else {
    real invd = 1.0 / delta;
    x = (b * c1 - c * b1) * invd;
    y = -(a * c1 - a1 * c) * invd;
  }
}


void ChConstraintRigidRigid::SolveQuartic() {
  const int2* ids = data_container->host_data.bids_rigid_rigid.data();
  const CompressedMatrix<real>& D_n_T = data_container->host_data.D_n_T;
  const CompressedMatrix<real>& D_t_T = data_container->host_data.D_t_T;
  const CompressedMatrix<real>& D_s_T = data_container->host_data.D_s_T;
  const CompressedMatrix<real>& M_inv = data_container->host_data.M_inv;

  const DynamicVector<real>& R_full = data_container->host_data.R_full;
  const thrust::host_vector<real3>& friction = data_container->host_data.fric_rigid_rigid;

  for (int index = 0; index < data_container->num_contacts; index++) {
    CompressedMatrix<real> D_T(3, 12), Minv(12, 12), D;
    DynamicVector<real> R(3);    // rhs
    DynamicVector<real> g(3);    // lagrange multipliers

    int2 body_id = ids[index];
    real f_mu = friction[index].x;

    SparseSubmatrix<SpMat>(D_T, 0, 0, 1, 6) = SparseSubmatrix<const SpMat>(D_n_T, index * 1 + 0, body_id.x * 6, 1, 6);
    SparseSubmatrix<SpMat>(D_T, 0, 6, 1, 6) = SparseSubmatrix<const SpMat>(D_n_T, index * 1 + 0, body_id.y * 6, 1, 6);
    SparseSubmatrix<SpMat>(D_T, 1, 0, 2, 6) = SparseSubmatrix<const SpMat>(D_t_T, index * 1 + 0, body_id.x * 6, 2, 6);
    SparseSubmatrix<SpMat>(D_T, 1, 6, 2, 6) = SparseSubmatrix<const SpMat>(D_t_T, index * 1 + 0, body_id.y * 6, 2, 6);

    D = blaze::trans(D_T);

    SparseSubmatrix<SpMat>(Minv, 0, 0, 6, 6) = SparseSubmatrix<const SpMat>(M_inv, body_id.x * 6, body_id.x * 6, 6, 6);
    SparseSubmatrix<SpMat>(Minv, 6, 6, 6, 6) = SparseSubmatrix<const SpMat>(M_inv, body_id.y * 6, body_id.y * 6, 6, 6);

    CompressedMatrix<real> N = D_T * Minv * D;

    R[0] = R_full[index * 1 + 0];
    R[1] = R_full[data_container->num_contacts + index * 2 + 0];
    R[2] = R_full[data_container->num_contacts + index * 2 + 1];

    real cg[5];
    int nbRealRacines;
    real Racines[4];

    real W0 = N(0, 0);
    real invW0 = 1.0 / W0;
    real alpha = N(0, 1);
    real beta = N(0, 2);

    real q = R[0];
    real q1 = -(-alpha * q * invW0 + R[1]);
    real q2 = -(-beta * q * invW0 + R[2]);

    real lambda1 = N(1, 1) - alpha * alpha * invW0;
    real lambda2 = N(2, 2) - beta * beta * invW0;
    real lambda12 = N(1, 2) - alpha * beta * invW0;
    real mu = f_mu;

    printf("W0=%e\ninvW0=%e\n,alpha=%e\nbeta=%e\nlambda1=%e\nlambda2=%e\nlambda12=%e\nq=%e\nq1=%e\nq2=%e\nmu=%e", W0, invW0, alpha, beta, lambda1, lambda2, lambda12, q, q1, q2, mu);

    cg[0] = -mu * mu * q * q;
    cg[1] = -2 * mu * mu * q * alpha * q1 - 2 * mu * mu * q * beta * q2 - (2 * lambda1 + 2 * lambda2) * mu * mu * q * q;
    cg[2] = 2 * mu * mu * q * beta * q1 * lambda12 + W0 * W0 * q2 * q2 + 2 * mu * mu * q * q * lambda12 * lambda12 - 2 * mu * mu * beta * alpha * q1 * q2 + 2 * mu * mu * q * alpha * q2 * lambda12 -
            (lambda1 * lambda1 + 4 * lambda1 * lambda2 + lambda2 * lambda2) * mu * mu * q * q + W0 * W0 * q1 * q1 - beta * beta * mu * mu * q2 * q2 -
            2 * (lambda1 + 2 * lambda2) * alpha * mu * mu * q * q1 - mu * mu * alpha * alpha * q1 * q1 - 2 * (2 * lambda1 + lambda2) * beta * mu * mu * q * q2;
    cg[3] = 2 * beta * mu * mu * q * q2 * lambda12 * lambda12 + 2 * (lambda1 + lambda2) * mu * mu * q * q * lambda12 * lambda12 - 4 * W0 * W0 * q1 * q2 * lambda12 -
            2 * (lambda1 * lambda1 + 2 * lambda1 * lambda2) * beta * mu * mu * q * q2 + 2 * (lambda1 + lambda2) * alpha * mu * mu * q * q2 * lambda12 -
            (2 * lambda1 * lambda1 * lambda2 + 2 * lambda1 * lambda2 * lambda2) * mu * mu * q * q + 2 * (lambda1 + lambda2) * beta * mu * mu * q * q1 * lambda12 +
            2 * alpha * beta * mu * mu * q2 * q2 * lambda12 + 2 * lambda1 * W0 * W0 * q2 * q2 + 2 * beta * beta * mu * mu * q1 * q2 * lambda12 - 2 * lambda2 * alpha * alpha * mu * mu * q1 * q1 -
            2 * (lambda1 + lambda2) * alpha * beta * mu * mu * q1 * q2 - 2 * (2 * lambda1 * lambda2 + lambda2 * lambda2) * alpha * mu * mu * q * q1 + 2 * mu * mu * alpha * alpha * q1 * q2 * lambda12 +
            2 * mu * mu * beta * alpha * q1 * q1 * lambda12 + 2 * mu * mu * q * alpha * q1 * lambda12 * lambda12 + 2 * lambda2 * W0 * W0 * q1 * q1 - 2 * lambda1 * beta * beta * mu * mu * q2 * q2;
    cg[4] = -beta * beta * mu * mu * q1 * q1 * lambda12 * lambda12 - alpha * alpha * mu * mu * q2 * q2 * lambda12 * lambda12 - lambda1 * lambda1 * beta * beta * mu * mu * q2 * q2 -
            lambda1 * lambda1 * lambda2 * lambda2 * mu * mu * q * q - lambda2 * lambda2 * alpha * alpha * mu * mu * q1 * q1 + lambda2 * lambda2 * W0 * W0 * q1 * q1 +
            lambda1 * lambda1 * W0 * W0 * q2 * q2 + W0 * W0 * q1 * q1 * lambda12 * lambda12 - 2 * beta * mu * mu * q * q1 * pow((real)lambda12, (real)3) -
            2 * alpha * mu * mu * q * q2 * pow((real)lambda12, (real)3) + 2 * lambda1 * lambda2 * beta * mu * mu * q * q1 * lambda12 + 2 * lambda1 * lambda2 * alpha * mu * mu * q * q2 * lambda12 -
            2 * lambda1 * lambda2 * alpha * beta * mu * mu * q1 * q2 - 2 * alpha * beta * mu * mu * q1 * q2 * lambda12 * lambda12 + 2 * lambda2 * alpha * beta * mu * mu * q1 * q1 * lambda12 +
            2 * lambda2 * alpha * alpha * mu * mu * q1 * q2 * lambda12 + 2 * lambda1 * beta * mu * mu * q * q2 * lambda12 * lambda12 + 2 * lambda1 * beta * beta * mu * mu * q1 * q2 * lambda12 +
            2 * lambda1 * alpha * beta * mu * mu * q2 * q2 * lambda12 - 2 * lambda1 * lambda2 * lambda2 * alpha * mu * mu * q * q1 - 2 * lambda1 * lambda1 * lambda2 * beta * mu * mu * q * q2 +
            2 * lambda2 * alpha * mu * mu * q * q1 * lambda12 * lambda12 - mu * mu * q * q * pow((real)lambda12, (real)4) + W0 * W0 * q2 * q2 * lambda12 * lambda12 -
            2 * lambda2 * W0 * W0 * q1 * q2 * lambda12 - 2 * lambda1 * W0 * W0 * q1 * q2 * lambda12 + 2 * lambda1 * lambda2 * mu * mu * q * q * lambda12 * lambda12;

    compute_roots(cg, nbRealRacines, Racines);
    real nu = -1;
    for (int i = 0; i < nbRealRacines; i++) {
      if (nu < 0 && Racines[i] > 0)
        nu = Racines[i];
      else if (Racines[i] > 0 && Racines[i] < nu)
        nu = Racines[i];
    }
    if (nu > 0) {

      /*system Mt*Rt=qt*/
      real Mt00 = lambda1 + nu;
      real Mt01 = lambda12;
      real Mt10 = lambda12;
      real Mt11 = lambda2 + nu;
      real qt0 = -q1;
      real qt1 = -q2;
      solve2x2(Mt00, Mt01, qt0, Mt10, Mt11, qt1, g[1], g[2]);
      g[0] = sqrt((g[1] * g[1]) + (g[2] * g[2])) / mu;

      printf("gamma: %f %f %f \n", g[0],g[1],g[2]);

    } else {
      g[0] = 0;
      g[1] = 0;
      g[2] = 0;
      printf("Solve Fail \n");
    }
  }
}


void local_project(DynamicVector<real> & gamma, real coh, real mu) {
  gamma[0] += coh;
  if (mu == 0) {
    gamma[0] = gamma[0]< 0 ? 0 : gamma[0] - coh;
    gamma[1] = gamma[2] = 0;
    return;
  }
  if (Cone_generalized(gamma[0], gamma[1], gamma[2], mu)) {
  }
  gamma[0] = gamma[0] - coh;
}


#include <blaze/math/SparseRow.h>


void ChConstraintRigidRigid::SolveLocal() {
  const int2* ids = data_container->host_data.bids_rigid_rigid.data();
  const CompressedMatrix<real>& D_n_T = data_container->host_data.D_n_T;
  const CompressedMatrix<real>& D_t_T = data_container->host_data.D_t_T;
  const CompressedMatrix<real>& D_s_T = data_container->host_data.D_s_T;
  const CompressedMatrix<real>& M_inv = data_container->host_data.M_inv;

  const DynamicVector<real>& R_full = data_container->host_data.R_full;
  DynamicVector<real>& gamma = data_container->host_data.gamma;
  const thrust::host_vector<real3>& friction = data_container->host_data.fric_rigid_rigid;
  const thrust::host_vector<real>& cohesion = data_container->host_data.coh_rigid_rigid;
//#pragma omp parallel for
  for (int index = 0; index < data_container->num_contacts; index++) {
    //BLAZE_SERIAL_SECTION
    {

      CompressedMatrix<real> D_T(3, 12), Minv(12, 12), D;
      DynamicVector<real> mb(3, 0);    // rhs
      DynamicVector<real> ml(3, 0);    // lagrange multipliers
      DynamicVector<real> ml_old(3, 0), ml_k(3, 0);
      DynamicVector<real> resid(3, 0);
      int2 body_id = ids[index];
      real mu = friction[index].x;
      real c = cohesion[index];

      SparseSubmatrix<SpMat>(D_T, 0, 0, 1, 6) = SparseSubmatrix<const SpMat>(D_n_T, index * 1 + 0, body_id.x * 6, 1, 6);
      SparseSubmatrix<SpMat>(D_T, 0, 6, 1, 6) = SparseSubmatrix<const SpMat>(D_n_T, index * 1 + 0, body_id.y * 6, 1, 6);
      SparseSubmatrix<SpMat>(D_T, 1, 0, 2, 6) = SparseSubmatrix<const SpMat>(D_t_T, index * 1 + 0, body_id.x * 6, 2, 6);
      SparseSubmatrix<SpMat>(D_T, 1, 6, 2, 6) = SparseSubmatrix<const SpMat>(D_t_T, index * 1 + 0, body_id.y * 6, 2, 6);

      D = blaze::trans(D_T);

      SparseSubmatrix<SpMat>(Minv, 0, 0, 6, 6) = SparseSubmatrix<const SpMat>(M_inv, body_id.x * 6, body_id.x * 6, 6, 6);
      SparseSubmatrix<SpMat>(Minv, 6, 6, 6, 6) = SparseSubmatrix<const SpMat>(M_inv, body_id.y * 6, body_id.y * 6, 6, 6);

      CompressedMatrix<real> N = D_T * Minv * D;

      mb[0] = R_full[index * 1 + 0];
      mb[1] = R_full[data_container->num_contacts + index * 2 + 0];
      mb[2] = R_full[data_container->num_contacts + index * 2 + 1];

      real Dinv = 3.0 / (N(0, 0) + N(1, 1) + N(2, 2));
      real omega = 1.0;
      real lambda = .5;
      for (int i = 0; i < 20; i++) {

        ml[0] = ml[0] - omega * Dinv * ((row(N, 0), ml_old) - mb[0]);
        ml_old[0] = ml[0];

        ml[1] = ml[1] - omega * Dinv * ((row(N, 1), ml_old) - mb[1]);
        ml_old[1] = ml[1];

        ml[2] = ml[2] - omega * Dinv * ((row(N, 2), ml_old) - mb[2]);
        ml_old[2] = ml[2];

        //      ml = ml-omega*Dinv*(N*ml_old-mb);
        //      ml = lambda*ml+(1-lambda)*ml_old;

        local_project(ml, c, mu);

        resid = ml - ml_k;
        real residual = (resid, resid);
        if (residual < 1e-6) {
          break;
        }
        // printf("gamma: %f %f %f %d %f %f\n", ml[0],ml[1],ml[2], i, Dinv, residual);

        ml_k = ml;
      }

      gamma[index * 1 + 0] = ml[0];
      gamma[data_container->num_contacts + index * 2 + 0] = ml[1];
      gamma[data_container->num_contacts + index * 2 + 1] = ml[2];
    }
  }
}

void ChConstraintRigidRigid::SolveInverse() {
  const int2* ids = data_container->host_data.bids_rigid_rigid.data();
  const CompressedMatrix<real>& D_n_T = data_container->host_data.D_n_T;
  const CompressedMatrix<real>& D_t_T = data_container->host_data.D_t_T;
  const CompressedMatrix<real>& D_s_T = data_container->host_data.D_s_T;
  const CompressedMatrix<real>& M_inv = data_container->host_data.M_inv;

  const DynamicVector<real>& R_full = data_container->host_data.R_full;
  const thrust::host_vector<real3>& friction = data_container->host_data.fric_rigid_rigid;
  const thrust::host_vector<real>& cohesion = data_container->host_data.coh_rigid_rigid;

  for (int index = 0; index < data_container->num_contacts; index++) {
    CompressedMatrix<real> D_T(3, 12), Minv(12, 12), D;
    DynamicVector<real> mb(3);    // rhs
    DynamicVector<real> ml(3);    // lagrange multipliers

    int2 body_id = ids[index];
    real mu = friction[index].x;
    real c = cohesion[index];

    SparseSubmatrix<SpMat>(D_T, 0, 0, 1, 6) = SparseSubmatrix<const SpMat>(D_n_T, index * 1 + 0, body_id.x * 6, 1, 6);
    SparseSubmatrix<SpMat>(D_T, 0, 6, 1, 6) = SparseSubmatrix<const SpMat>(D_n_T, index * 1 + 0, body_id.y * 6, 1, 6);
    SparseSubmatrix<SpMat>(D_T, 1, 0, 2, 6) = SparseSubmatrix<const SpMat>(D_t_T, index * 1 + 0, body_id.x * 6, 2, 6);
    SparseSubmatrix<SpMat>(D_T, 1, 6, 2, 6) = SparseSubmatrix<const SpMat>(D_t_T, index * 1 + 0, body_id.y * 6, 2, 6);

    D = blaze::trans(D_T);

    SparseSubmatrix<SpMat>(Minv, 0, 0, 6, 6) = SparseSubmatrix<const SpMat>(M_inv, body_id.x * 6, body_id.x * 6, 6, 6);
    SparseSubmatrix<SpMat>(Minv, 6, 6, 6, 6) = SparseSubmatrix<const SpMat>(M_inv, body_id.y * 6, body_id.y * 6, 6, 6);

    CompressedMatrix<real> N = D_T * Minv * D;

    mb[0] = R_full[index * 1 + 0];
    mb[1] = R_full[data_container->num_contacts + index * 2 + 0];
    mb[2] = R_full[data_container->num_contacts + index * 2 + 1];


    real det = N(0, 0) * (N(1, 1) * N(2, 2) - N(2, 1) * N(1, 2)) - N(0, 1) * (N(1, 0) * N(2, 2) - N(1, 2) * N(2, 0)) + N(0, 2) * (N(1, 0) * N(2, 1) - N(1, 1) * N(2, 0));
    real invdet = 1.0 / det;
    CompressedMatrix<real> minv;

    minv(0, 0) = (N(1, 1) * N(2, 2) - N(2, 1) * N(1, 2)) * invdet;
    minv(0, 1) = (N(0, 2) * N(2, 1) - N(0, 1) * N(2, 2)) * invdet;
    minv(0, 2) = (N(0, 1) * N(1, 2) - N(0, 2) * N(1, 1)) * invdet;
    minv(1, 0) = (N(1, 2) * N(2, 0) - N(1, 0) * N(2, 2)) * invdet;
    minv(1, 1) = (N(0, 0) * N(2, 2) - N(0, 2) * N(2, 0)) * invdet;
    minv(1, 2) = (N(1, 0) * N(0, 2) - N(0, 0) * N(1, 2)) * invdet;
    minv(2, 0) = (N(1, 0) * N(2, 1) - N(2, 0) * N(1, 1)) * invdet;
    minv(2, 1) = (N(2, 0) * N(0, 1) - N(0, 0) * N(2, 1)) * invdet;
    minv(2, 2) = (N(0, 0) * N(1, 1) - N(1, 0) * N(0, 1)) * invdet;

    ml = minv*mb;


    printf("gamma: %f %f %f \n", ml[0],ml[1],ml[2]);
  }
}



