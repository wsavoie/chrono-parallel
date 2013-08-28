#ifndef CHCONSTRAINT_FLUIDFLUID_H
#define CHCONSTRAINT_FLUIDFLUID_H

#include "ChBaseGPU.h"

namespace chrono {
class ChApiGPU ChConstraintFluidFluid: public ChBaseGPU {
	public:
		ChConstraintFluidFluid(ChGPUDataManager *data_container_) {
			data_container = data_container_;
			Initialize();
		}
		~ChConstraintFluidFluid() {
		}
		void host_Project(int2 *ids, real *gamma);
		void Project(custom_vector<real> & gamma);

		void host_RHS(int2 *ids, real3 *pos, real3 *vel, real3 *JXYZA, real3 *JXYZB, real* compliance, real *rhs);
		void ComputeRHS();
		void host_Jacobians(int2* ids, real3* pos, real3* JXYZA, real3* JXYZB);
		void ComputeJacobians();

		void host_shurA(int2 *ids, real *inv_mass, real3 *JXYZA, real3 *JXYZB, real *gamma, real3* QXYZ);
		void host_shurB(int2 *ids, real *inv_mass, real * gamma,real* compliance, real3 *JXYZA, real3 *JXYZB, real3 *QXYZ, real *AX);

		void ShurA(custom_vector<real> &x);
		void ShurB(custom_vector<real> &x, custom_vector<real> & output);
		protected:

		custom_vector<real3> device_JXYZA_fluid_fluid;
		custom_vector<real3> device_JXYZB_fluid_fluid;
		custom_vector<real> device_comp_fluid_fluid;
		custom_vector<real> rhs, gamma, ax;

	}
	;}

#endif