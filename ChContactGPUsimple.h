#ifndef CHCONTACTGPUSIMPLE_H
#define CHCONTACTGPUSIMPLE_H

///////////////////////////////////////////////////
//
//   ChContactGPUsimple.h
//
//   Classes for enforcing constraints (contacts)
//   created by collision detection.
//
//   HEADER file for CHRONO,
//	 Multibody dynamics engine
//
// ------------------------------------------------
// 	 Copyright:Alessandro Tasora / DeltaKnowledge
//             www.deltaknowledge.com
// ------------------------------------------------
///////////////////////////////////////////////////
#include "core/ChFrame.h"
#include "ChLcpConstraintTwoGPUcontN.h"
#include "lcp/ChLcpSystemDescriptor.h"
#include "lcp/ChLcpVariablesBody.h"
#include "collision/ChCCollisionModel.h"
#include "ChApiGPU.h"
namespace chrono
{



	/// Class representing an unilateral contact constraint. 
	/// Such objects are automatically generated by the
	/// collision detection engine. 
	/// This is a 'basic' implementation of contact for the 
	/// original GPU solver by Dan & Ale. This is used in
	/// the ChContactContainerGPUsimple container. (In future, 
	/// the GPU solver will access directly a more performant
	/// ChContactContainerGPUsimple that uses GPU buffers for storing
	/// contacts, instead of vectors of structures like this one.)

	class ChApiGPU ChContactGPUsimple {

	protected:
		//
		// DATA
		//
		collision::ChCollisionModel* modA;	///< model A
		collision::ChCollisionModel* modB;  ///< model B

		// The three scalar constraints, to be feed into the 'basic' GPU
		// system solver. They contain jacobians data and special functions.
		ChLcpConstraintTwoGPUcontN Nx;
		ChLcpConstraintTwoGPUcontT Tu;
		ChLcpConstraintTwoGPUcontT Tv; 

		ChVector<> react_force;

	public:
		//
		// CONSTRUCTORS
		//

		ChContactGPUsimple ();

		ChContactGPUsimple (collision::ChCollisionModel* mmodA,	///< model A
			collision::ChCollisionModel* mmodB,	///< model B
			const ChLcpVariablesBody* varA, ///< pass A vars
			const ChLcpVariablesBody* varB, ///< pass B vars
			const ChFrame<>* frameA,		///< pass A frame
			const ChFrame<>* frameB,		///< pass B frame
			const ChVector<>& vpA,			///< pass coll.point on A
			const ChVector<>& vpB,			///< pass coll.point on B
			const ChVector<>& vN, 			///< pass coll.normal, respect to A
			double mdistance,				///< pass the distance (negative for penetration)
			float* mreaction_cache,			///< pass the pointer to array of N,U,V reactions: a cache in contact manifold. If not available=0.
			float  mfriction				///< friction coeff.
			);

		virtual ~ChContactGPUsimple ();


		//
		// FUNCTIONS
		//

		/// Initialize again this constraint.
		virtual void Reset(	collision::ChCollisionModel* mmodA,	///< model A
			collision::ChCollisionModel* mmodB,	///< model B
			const ChLcpVariablesBody* varA, ///< pass A vars
			const ChLcpVariablesBody* varB, ///< pass B vars
			const ChFrame<>* frameA,		///< pass A frame
			const ChFrame<>* frameB,		///< pass B frame
			const ChVector<>& vpA,			///< pass coll.point on A
			const ChVector<>& vpB,			///< pass coll.point on B
			const ChVector<>& vN, 			///< pass coll.normal, respect to A
			double mdistance,				///< pass the distance (negative for penetration)
			float* mreaction_cache,			///< pass the pointer to array of N,U,V reactions: a cache in contact manifold. If not available=0.
			float mfriction					///< friction coeff.
			);

		/// Get the contact coordinate system, expressed in absolute frame.
		/// This represents the 'main' reference of the link: reaction forces 
		/// are expressed in this coordinate system. Its origin is point P2.
		/// (It is the coordinate system of the contact plane and normal)
		virtual ChCoordsys<> GetContactCoords();

		/// Returns the pointer to a contained 3x3 matrix representing the UV and normal
		/// directions of the contact. In detail, the X versor (the 1s column of the 
		/// matrix) represents the direction of the contact normal.
		//ChMatrix33<float>* GetContactPlane() {return &contact_plane;};


		/// Get the contact point 1, in absolute coordinates
		virtual ChVector<> GetContactP1() {return Nx.GetP1(); };

		/// Get the contact point 2, in absolute coordinates
		virtual ChVector<> GetContactP2() {return Nx.GetP2(); };

		/// Get the contact normal, in absolute coordinates
		virtual ChVector<float> GetContactNormal()  {return Nx.GetNormal(); };

		/// Get the contact distance
		virtual double	   GetContactDistance()  {return Vdot(Nx.GetNormal(), Nx.GetP2()-Nx.GetP1());; };

		/// Get the contact force, if computed, in contact coordinate system
		virtual ChVector<> GetContactForce() {return react_force; };

		/// Get the contact friction coefficient
		virtual float GetFriction() {return (float)Nx.GetFrictionCoefficient(); };
		/// Set the contact friction coefficient
		virtual void SetFriction(float mf) { Nx.SetFrictionCoefficient(mf); };

		/// Get the collision model A, with point P1
		virtual collision::ChCollisionModel* GetModelA() {return this->modA;}
		/// Get the collision model B, with point P2
		virtual collision::ChCollisionModel* GetModelB() {return this->modB;}

		//
		// UPDATING FUNCTIONS
		//


		void  InjectConstraints(ChLcpSystemDescriptor& mdescriptor);

		void  ConstraintsBiReset();

		void  ConstraintsBiLoad_C(double factor=1., double recovery_clamp=0.1, bool do_clamp=false);

		void  ConstraintsFetch_react(double factor);

		void  ConstraintsLiLoadSuggestedSpeedSolution();

		void  ConstraintsLiLoadSuggestedPositionSolution();

		void  ConstraintsLiFetchSuggestedSpeedSolution();

		void  ConstraintsLiFetchSuggestedPositionSolution();

	};




	//////////////////////////////////////////////////////
	//////////////////////////////////////////////////////


} // END_OF_NAMESPACE____

#endif
