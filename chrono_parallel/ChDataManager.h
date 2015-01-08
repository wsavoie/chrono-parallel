// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Hammad Mazhar
// =============================================================================
//
// Description: This class contains manages all data associated with a parallel
// System. Rather than passing in individual data parameters to different parts
// of the code like the collision detection and the solver, passing a pointer to
// a data manager is more convenient from a development perspective.
// =============================================================================

#ifndef CH_DATAMANAGER_H
#define CH_DATAMANAGER_H

#include "chrono_parallel/ChParallelDefines.h"
#include "chrono_parallel/math/ChParallelMath.h"
#include "chrono_parallel/ChTimerParallel.h"
#include "chrono_parallel/ChSettings.h"
#include <blaze/math/CompressedMatrix.h>

using blaze::CompressedMatrix;
using blaze::DynamicVector;
namespace chrono {

struct host_container {
   // Collision data
   thrust::host_vector<real3> ObA_rigid;
   thrust::host_vector<real3> ObB_rigid;
   thrust::host_vector<real3> ObC_rigid;
   thrust::host_vector<real4> ObR_rigid;
   thrust::host_vector<short2> fam_rigid;
   thrust::host_vector<int> typ_rigid;
   thrust::host_vector<uint> id_rigid;
   thrust::host_vector<real3> aabb_rigid;

   // Contact data
   thrust::host_vector<real3> norm_rigid_rigid;
   thrust::host_vector<real3> cpta_rigid_rigid;
   thrust::host_vector<real3> cptb_rigid_rigid;
   thrust::host_vector<real> dpth_rigid_rigid;
   thrust::host_vector<real> erad_rigid_rigid;
   thrust::host_vector<int2> bids_rigid_rigid;
   thrust::host_vector<long long> pair_rigid_rigid;

   // This vector holds the friction information as a triplet
   // x - Sliding friction
   // y - Rolling friction
   // z - Spinning Friction
   // This is precomputed at every timestep for all contacts in parallel
   // Improves performance and reduces conditionals later on
   thrust::host_vector<real3> fric_rigid_rigid;
   // Holds the cohesion value for each contact, similar to friction this is
   // precomputed for all contacts in parallel
   thrust::host_vector<real> coh_rigid_rigid;

   // Object data
   thrust::host_vector<real3> pos_data, pos_new_data;
   thrust::host_vector<real4> rot_data, rot_new_data;
   thrust::host_vector<real3> inr_data;
   thrust::host_vector<bool> active_data;
   thrust::host_vector<bool> collide_data;
   thrust::host_vector<real> inv_mass_data;

   // keeps track of active bilateral constraints
   thrust::host_vector<int> bilateral_mapping;

   // Shaft data
   thrust::host_vector<real> shaft_rot;    // shaft rotation angles
   thrust::host_vector<real> shaft_omg;    // shaft angular velocities
   thrust::host_vector<real> shaft_trq;    // shaft torques
   thrust::host_vector<real> shaft_inr;    // shaft inverse inertias
   thrust::host_vector<bool> shaft_active; // shaft active (not sleeping nor fixed) flags

   // Material properties (DVI)
   thrust::host_vector<real3> fric_data;
   thrust::host_vector<real> cohesion_data;
   thrust::host_vector<real4> compliance_data;

   // Material properties (DEM)
   thrust::host_vector<real2> elastic_moduli;  // Young's modulus and Poisson ratio
   thrust::host_vector<real> mu;               // Coefficient of friction
   thrust::host_vector<real> alpha;            // Dissipation factor (Hunt-Crossley)
   thrust::host_vector<real> cr;               // Coefficient of restitution

   //This matrix, if used will hold D^TxM^-1xD in sparse form
   CompressedMatrix<real> Nshur;
   //The D Matrix hold the Jacobian for the entire system
   CompressedMatrix<real> D;
   //D_T is the transpose of the D matrix, note that D_T is actually computed
   //first and D is taken as the transpose. This is due to the way that blaze
   //handles sparse matrix allocation, it is easier to do it on a per row basis
   CompressedMatrix<real> D_T;
   //M_inv is the inver mass matrix, This matrix, if holding the full inertia
   //tensor is block diagonal
   CompressedMatrix<real> M_inv;
   //Minv_D holds M_inv multiplied by D, this is done as a preprocessing step
   //so that later, when the full matrix vector product is needed it can be
   //performed in two steps, first R = Minv_D*x, and then D_T*R where R is just
   //a temporary variable used here for illustrative purposes. In reality the
   //entire operation happens inline without a temp variable.
   CompressedMatrix<real> M_invD;

   DynamicVector<real> R; //The right hand side of the system
   DynamicVector<real> b; //Correction terms
   DynamicVector<real> M_invk; //result of M_inv multiplied by vector of forces
   DynamicVector<real> gamma; //THe unknowns we are solving for
   DynamicVector<real> v; //This vector holds the velocities for all objects
   DynamicVector<real> hf;//This vector holds h*forces, h is time step
   DynamicVector<real> rhs_bilateral;
   //While E is the compliance matrix, in reality it is completely diagonal
   //therefore it is stored in a vector for performance reasons
   DynamicVector<real> E;
   //Vector that stores the gamma values for just the bilaterals
   DynamicVector<real> gamma_bilateral;

};

class CH_PARALLEL_API ChParallelDataManager {
 public:
   ChParallelDataManager();
   ~ChParallelDataManager();

   host_container host_data;

   //Indexing variables
   uint num_bodies;			//The number of objects in a system
   uint num_models;			//The number of collision models in a system
   uint num_contacts;		//The number of contacts in a system
   uint old_num_contacts;	//The number of contacts during the previous step
   uint num_unilaterals;	//The number of contact constraints
   uint num_bilaterals;		//The number of bilateral constraints
   uint num_constraints;	//Total number of constraints
   uint num_shafts;			//The number of shafts in teh system

   // Flag indicating whether or not effective contact radius is calculated
   bool erad_is_set;
   //This object hold all of the timers for the system
   ChTimerParallel system_timer;
   //Structure that contains all settings for the system, collision detection
   //and the solver
   settings_container settings;

   //Output a vector (one dimensional matrix) from blaze to a file
   int OutputBlazeVector(blaze::DynamicVector<real> src, std::string filename);
   //Output a sparse blaze matrix to a file
   int OutputBlazeMatrix(blaze::CompressedMatrix<real> src,std::string filename);
   //Convenience funtion that outputs all of the data associted for a system
   //This is useful when debugging
   int ExportCurrentSystem(std::string output_dir);
};
}

#endif
