#include "ChSolverAPGDBlaze.h"

using namespace chrono;

void ChSolverAPGDBlaze::SetAPGDParams(real theta_k,
                                      real shrink,
                                      real grow) {
   init_theta_k = theta_k;
   step_shrink = shrink;
   step_grow = grow;

}

real ChSolverAPGDBlaze::Res4(const int SIZE,
                             blaze::DynamicVector<real> & mg_tmp2,
                             blaze::DynamicVector<real> &x,
                             blaze::DynamicVector<real> & mb_tmp) {
   real gdiff = 1e-6;
   real sum = 0;
   mb_tmp = -gdiff * mg_tmp2 + x;
   Project(mb_tmp.data());
   mb_tmp = (-1.0f / (gdiff)) * (mb_tmp - x);
   sum = (mb_tmp, trans(mb_tmp));
   return sqrt(sum);

}

uint ChSolverAPGDBlaze::SolveAPGDBlaze(const uint max_iter,
                                       const uint size,
                                       custom_vector<real> &b,
                                       custom_vector<real> &x) {
   ms.resize(size);
   mg_tmp2.resize(size);
   mb_tmp.resize(size);
   mg_tmp.resize(size);
   mg_tmp1.resize(size);
   mg.resize(size);
   ml.resize(size);
   mx.resize(size);
   my.resize(size);
   mb.resize(size);
   mso.resize(size);
   blaze::DynamicVector<real> one(size, 1.0);
   data_container->system_timer.start("ChSolverParallel_solverA");
   real lastgoodres = 10e30;
   real theta_k = init_theta_k;
   real theta_k1 = theta_k;
   real beta_k1 = 0.0;
   real L_k, t_k;
   real mb_tmp_norm = 0, mg_tmp_norm = 0;
   real obj1 = 0.0, obj2 = 0.0;
   real dot_mg_ms = 0, norm_ms = 0;
   real delta_obj = 1e8;
   real maxdeltalambda = 0;
#pragma omp parallel for
   for (int i = 0; i < size; i++) {
      ml[i] = x[i];
      mb[i] = b[i];
   }

   custom_vector<real3> vel_data, omg_data;

   Project(ml.data());
   ml_candidate = ml;
   //ShurProduct(ml, mg);  //mg is never used, only re-written
   mg = data_container->host_data.Nshur * ml;
   mg = mg - mb;
   mb_tmp = ml - one;
   mg_tmp = data_container->host_data.Nshur * mb_tmp;

   mb_tmp_norm = (mb_tmp, trans(mb_tmp));
   mb_tmp_norm = sqrt(mb_tmp_norm);

   mg_tmp_norm = (mg_tmp, trans(mg_tmp));
   mg_tmp_norm = sqrt(mg_tmp_norm);

   if (mb_tmp_norm == 0) {
      L_k = 1;
   } else {
      L_k = mg_tmp_norm / mb_tmp_norm;
   }

   t_k = 1.0 / L_k;
   my = ml;
   mx = ml;
   obj1 = obj2 = 0.0;
   dot_mg_ms = 0;
   norm_ms = 0;
   data_container->system_timer.stop("ChSolverParallel_solverA");

   for (current_iteration = 0; current_iteration < max_iter; current_iteration++) {

      data_container->system_timer.start("ChSolverParallel_solverB");

      mg_tmp1 = data_container->host_data.Nshur * my;
      data_container->system_timer.stop("ChSolverParallel_solverB");
      data_container->system_timer.start("ChSolverParallel_solverC");

      mg = mg_tmp1 - mb;
      mx = -t_k * mg + my;

      Project(mx.data());
      mg_tmp = data_container->host_data.Nshur * mx;
      data_container->system_timer.stop("ChSolverParallel_solverC");
      data_container->system_timer.start("ChSolverParallel_solverD");

      //mg_tmp2 = mg_tmp - mb;
      mso = .5 * mg_tmp - mb;
      obj1 = (mx, trans(mso));
      ms = .5 * mg_tmp1 - mb;
      obj2 = (my, trans(ms));
      ms = mx - my;
      dot_mg_ms = (mg, trans(ms));
      norm_ms = (ms, trans(ms));

      data_container->system_timer.stop("ChSolverParallel_solverD");
      while (obj1 > obj2 + dot_mg_ms + 0.5 * L_k * norm_ms) {
         data_container->system_timer.start("ChSolverParallel_solverE");
         L_k = 2.0 * L_k;
         t_k = 1.0 / L_k;
         mx = -t_k * mg + my;
         Project(mx.data());

         mg_tmp = data_container->host_data.Nshur * mx;

         mso = .5 * mg_tmp - mb;
         obj1 = (mx, trans(mso));
         ms = mx - my;
         dot_mg_ms = (mg, trans(ms));
         norm_ms = (ms, trans(ms));

         data_container->system_timer.stop("ChSolverParallel_solverE");
      }
      data_container->system_timer.start("ChSolverParallel_solverF");

      theta_k1 = (-pow(theta_k, 2) + theta_k * sqrt(pow(theta_k, 2) + 4)) / 2.0;
      beta_k1 = theta_k * (1.0 - theta_k) / (pow(theta_k, 2) + theta_k1);
      real dot_mg_ms = 0;

      ms = mx - ml;
      my = beta_k1 * ms + mx;
      dot_mg_ms = (mg, trans(ms));

      if (dot_mg_ms > 0) {
         my = mx;
         theta_k1 = 1.0;
      }
      L_k = 0.9 * L_k;
      t_k = 1.0 / L_k;
      ml = mx;
      step_grow = 2.0;
      theta_k = theta_k1;
      //if (current_iteration % 2 == 0) {
      mg_tmp2 = mg_tmp - mb;
      real g_proj_norm = Res4(num_unilaterals, mg_tmp2, ml, mb_tmp);
      //real objective = 100;
//      if(current_iteration==0){
//         old_objective = GetObjective(ml,b);
//      }else{
//         objective = GetObjective(ml,b);
//         g_proj_norm = fabs(objective - old_objective);
//         old_objective = objective;
//      }

      if (num_bilaterals > 0) {
         real resid_bilat = -1;
         for (int i = num_unilaterals; i < x.size(); i++) {
            resid_bilat = std::max(resid_bilat, std::abs(mg_tmp2[i]));
         }
         g_proj_norm = std::max(g_proj_norm, resid_bilat);
      }

      bool update = false;
      if (g_proj_norm < lastgoodres) {
         lastgoodres = g_proj_norm;
         ml_candidate = ml;
         maxdeltalambda = (ml_candidate, mso);
         //maxdeltalambda = GetObjectiveBlaze(ml_candidate, mb);
         update = true;

      }

      residual = lastgoodres;
      //CompRes(b,num_contacts);     //NormInf(ms);
      //if (update_rhs) {
      //ComputeSRhs(ml_candidate, rhs, vel_data, omg_data, b);
      //}
//      if (collision_inside) {
//         //UpdatePosition(ml_candidate);
//         //UpdateContacts();
//      }
      //}

      AtIterationEnd(residual, maxdeltalambda, iter_hist.size());
      //cout << "delta_obj " << delta_obj<<" "<< residual<<" "<<current_iteration<< endl;
      if (tol_objective) {
         if (maxdeltalambda <= tolerance) {
            break;
         }
      } else {
         if (residual < tolerance) {
            break;
         }
      }
      data_container->system_timer.stop("ChSolverParallel_solverF");
   }
#pragma omp parallel for
   for (int i = 0; i < size; i++) {
      x[i] = ml_candidate[i];
   }
   return current_iteration;
}