#include "chrono_parallel/physics/ChSystemParallel.h"
#include <omp.h>

using namespace chrono;

ChSystemParallelDVI::ChSystemParallelDVI(unsigned int max_objects)
      : ChSystemParallel(max_objects) {
   LCP_descriptor = new ChLcpSystemDescriptorParallelDVI();
   LCP_solver_speed = new ChLcpSolverParallelDVI();
   ((ChLcpSystemDescriptorParallelDVI*) LCP_descriptor)->data_container = data_manager;
   ((ChLcpSolverParallel*) LCP_solver_speed)->data_container = data_manager;

   //Set this so that the CD can check what type of system it is (needed for narrowphase)
   data_manager->settings.system_type = SYSTEM_DVI;

   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurA_normal");
   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurA_sliding");
   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurA_spinning");
   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurA_reduce");
   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurB_normal");
   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurB_sliding");
   data_manager->system_timer.AddTimer("ChConstraintRigidRigid_shurB_spinning");

   data_manager->system_timer.AddTimer("ChSolverParallel_solverA");
   data_manager->system_timer.AddTimer("ChSolverParallel_solverB");
   data_manager->system_timer.AddTimer("ChSolverParallel_solverC");
   data_manager->system_timer.AddTimer("ChSolverParallel_solverD");
   data_manager->system_timer.AddTimer("ChSolverParallel_solverE");
   data_manager->system_timer.AddTimer("ChSolverParallel_solverF");
   data_manager->system_timer.AddTimer("ChSolverParallel_solverG");
   data_manager->system_timer.AddTimer("ChSolverParallel_Project");
   data_manager->system_timer.AddTimer("ChSolverParallel_Solve");

}

void ChSystemParallelDVI::AddMaterialSurfaceData(ChSharedPtr<ChBody> newbody) {
   assert(typeid(*newbody.get_ptr()) == typeid(ChBody) || typeid(*newbody.get_ptr()) == typeid(ChBodyAuxRef));

   // Reserve space for material properties for the specified body. Not that the
   // actual data is set in UpdateMaterialProperties().
   data_manager->host_data.fric_data.push_back(R3(0));
   data_manager->host_data.cohesion_data.push_back(0);
   data_manager->host_data.compliance_data.push_back(R4(0));
}

void ChSystemParallelDVI::UpdateBodies() {
  custom_vector<bool>& active_pointer = data_manager->host_data.active_data;
  custom_vector<bool>& collide_pointer = data_manager->host_data.collide_data;
  custom_vector<real>& inv_mass_pointer = data_manager->host_data.inv_mass_data;
  custom_vector<real>& cohesion_pointer = data_manager->host_data.cohesion_data;
  custom_vector<real3>& fric_pointer = data_manager->host_data.fric_data;
  custom_vector<real3>& pos_pointer = data_manager->host_data.pos_data;
  custom_vector<real4>& rot_pointer = data_manager->host_data.rot_data;
  custom_vector<M33>& inr_pointer = data_manager->host_data.inr_data;
  custom_vector<real4>& compliance_pointer = data_manager->host_data.compliance_data;

#pragma omp parallel for
  for (int i = 0; i < bodylist.size(); i++) {
    bodylist[i]->UpdateTime(ChTime);
    // See if the body can fall asleep; if so, put it to sleeping
    // bodylist[i]->TrySleeping();
    // Apply limits (if in speed clamping mode) to speeds.
    bodylist[i]->ClampSpeed();
    // Set the gyroscopic momentum.
    bodylist[i]->ComputeGyro();
    bodylist[i]->UpdateForces(ChTime);
    bodylist[i]->VariablesFbLoadForces(GetStep());
    bodylist[i]->VariablesQbLoadSpeed();

    bodylist[i]->UpdateMarkers(ChTime);

    // Get the inverse inertia of the body
    ChMatrix33<> inertia = bodylist[i]->VariablesBody().GetBodyInvInertia();
    ChSharedPtr<ChMaterialSurface>& mat = bodylist[i]->GetMaterialSurface();

    data_manager->host_data.v[i * 6 + 0] = bodylist[i]->Variables().Get_qb().GetElementN(0);
    data_manager->host_data.v[i * 6 + 1] = bodylist[i]->Variables().Get_qb().GetElementN(1);
    data_manager->host_data.v[i * 6 + 2] = bodylist[i]->Variables().Get_qb().GetElementN(2);
    data_manager->host_data.v[i * 6 + 3] = bodylist[i]->Variables().Get_qb().GetElementN(3);
    data_manager->host_data.v[i * 6 + 4] = bodylist[i]->Variables().Get_qb().GetElementN(4);
    data_manager->host_data.v[i * 6 + 5] = bodylist[i]->Variables().Get_qb().GetElementN(5);

    data_manager->host_data.hf[i * 6 + 0] = bodylist[i]->Variables().Get_fb().ElementN(0);
    data_manager->host_data.hf[i * 6 + 1] = bodylist[i]->Variables().Get_fb().ElementN(1);
    data_manager->host_data.hf[i * 6 + 2] = bodylist[i]->Variables().Get_fb().ElementN(2);
    data_manager->host_data.hf[i * 6 + 3] = bodylist[i]->Variables().Get_fb().ElementN(3);
    data_manager->host_data.hf[i * 6 + 4] = bodylist[i]->Variables().Get_fb().ElementN(4);
    data_manager->host_data.hf[i * 6 + 5] = bodylist[i]->Variables().Get_fb().ElementN(5);

    pos_pointer[i] = (R3(bodylist[i]->GetPos().x, bodylist[i]->GetPos().y, bodylist[i]->GetPos().z));
    rot_pointer[i] = (R4(bodylist[i]->GetRot().e0, bodylist[i]->GetRot().e1, bodylist[i]->GetRot().e2, bodylist[i]->GetRot().e3));
    inr_pointer[i] = (M33(R3(inertia.GetElement(0, 0), inertia.GetElement(1, 0), inertia.GetElement(2, 0)),
                          R3(inertia.GetElement(0, 1), inertia.GetElement(1, 1), inertia.GetElement(2, 1)),
                          R3(inertia.GetElement(0, 2), inertia.GetElement(1, 2), inertia.GetElement(2, 2))));

    active_pointer[i] = bodylist[i]->IsActive();
    collide_pointer[i] = bodylist[i]->GetCollide();
    inv_mass_pointer[i] = 1.0f / bodylist[i]->VariablesBody().GetBodyMass();
    fric_pointer[i] = R3(mat->GetKfriction(), mat->GetRollingFriction(), mat->GetSpinningFriction());
    cohesion_pointer[i] = bodylist[i]->GetMaterialSurface()->GetCohesion();
    compliance_pointer[i] = R4(bodylist[i]->GetMaterialSurface()->GetCompliance(),
                               bodylist[i]->GetMaterialSurface()->GetComplianceT(),
                               bodylist[i]->GetMaterialSurface()->GetComplianceRolling(),
                               bodylist[i]->GetMaterialSurface()->GetComplianceSpinning());
    bodylist[i]->GetCollisionModel()->SyncPosition();
  }
}

static inline chrono::ChVector<real> ToChVector(const real3 &a) {
   return chrono::ChVector<real>(a.x, a.y, a.z);
}

void ChSystemParallelDVI::SolveSystem() {
   data_manager->system_timer.Reset();
   data_manager->system_timer.start("step");
   data_manager->system_timer.start("update");
   Setup();
   Update();
   data_manager->system_timer.stop("update");
   data_manager->system_timer.start("collision");
   collision_system->Run();
   collision_system->ReportContacts(this->contact_container);
   data_manager->system_timer.stop("collision");
   data_manager->system_timer.start("lcp");
   ((ChLcpSolverParallel *) (LCP_solver_speed))->RunTimeStep(GetStep());
   data_manager->system_timer.stop("lcp");
   data_manager->system_timer.stop("step");
   timer_update = data_manager->system_timer.GetTime("update");
   timer_collision = data_manager->system_timer.GetTime("collision");
   timer_lcp = data_manager->system_timer.GetTime("lcp");
   timer_step = data_manager->system_timer.GetTime("step");
}
void ChSystemParallelDVI::AssembleSystem() {
   Setup();

   collision_system->Run();
   collision_system->ReportContacts(this->contact_container);
   ChSystem::Update();
   this->contact_container->BeginAddContact();
   chrono::collision::ChCollisionInfo icontact;
   for (int i = 0; i < data_manager->num_contacts; i++) {
      int2 cd_pair = data_manager->host_data.bids_rigid_rigid[i];
      icontact.modelA = bodylist[cd_pair.x]->GetCollisionModel();
      icontact.modelB = bodylist[cd_pair.y]->GetCollisionModel();
      icontact.vN = ToChVector(data_manager->host_data.norm_rigid_rigid[i]);
      icontact.vpA = ToChVector(data_manager->host_data.cpta_rigid_rigid[i] + data_manager->host_data.pos_data[cd_pair.x]);
      icontact.vpB = ToChVector(data_manager->host_data.cptb_rigid_rigid[i] + data_manager->host_data.pos_data[cd_pair.y]);
      icontact.distance = data_manager->host_data.dpth_rigid_rigid[i];
      this->contact_container->AddContact(icontact);
   }
   this->contact_container->EndAddContact();

   {
      std::vector<ChLink*>::iterator iterlink = linklist.begin();
      while (iterlink != linklist.end()) {
         (*iterlink)->ConstraintsBiReset();
         iterlink++;
      }
      std::vector<ChBody*>::iterator ibody = bodylist.begin();
      while (ibody != bodylist.end()) {
         (*ibody)->VariablesFbReset();
         ibody++;
      }
      this->contact_container->ConstraintsBiReset();
   }

   LCPprepare_load(true,                  // Cq,
         true,                            // adds [M]*v_old to the known vector
         step,                            // f*dt
         step * step,                     // dt^2*K  (nb only non-Schur based solvers support K matrix blocks)
         step,                            // dt*R   (nb only non-Schur based solvers support R matrix blocks)
         1.0,                             // M (for FEM with non-lumped masses, add their mass-matrixes)
         1.0,                             // Ct   (needed, for rheonomic motors)
         1.0 / step,                      // C/dt
         max_penetration_recovery_speed,  // vlim, max penetrations recovery speed (positive for exiting)
         true                             // do above max. clamping on -C/dt
         );

   this->LCP_descriptor->BeginInsertion();
   for (int i = 0; i < bodylist.size(); i++) {
      bodylist[i]->InjectVariables(*this->LCP_descriptor);
   }
   std::vector<ChLink *>::iterator it;
   for (it = linklist.begin(); it != linklist.end(); it++) {
      (*it)->InjectConstraints(*this->LCP_descriptor);
   }
   this->contact_container->InjectConstraints(*this->LCP_descriptor);
   this->LCP_descriptor->EndInsertion();

}

