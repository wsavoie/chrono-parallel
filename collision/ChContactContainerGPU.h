#ifndef CHCONTACTCONTAINERGPUSIMPLE_H
#define CHCONTACTCONTAINERGPUSIMPLE_H

///////////////////////////////////////////////////
//
//   ChContactContainerGPUsimple.h
//
//   Class for container of many contacts, as CPU
//   typical linked list of ChContactGPUsimple objects
//
//   HEADER file for CHRONO,
//   Multibody dynamics engine
//
// ------------------------------------------------
//   Copyright:Alessandro Tasora / DeltaKnowledge
//             www.deltaknowledge.com
// ------------------------------------------------
///////////////////////////////////////////////////
#include "physics/ChContactContainer.h"
#include <list>
#include "../ChApiGPU.h"
#include "ChDataManager.h"
namespace chrono {
/// Class representing a container of many contacts,
/// implemented as a typical linked list of ChContactGPUsimple
/// objects.
/// This contact container must be used for the preliminar CUDA solver
/// that was developed by Ale & Dan, but in future will be
/// replaced by ChContactContainerGPU, and advanced container
/// that does not use linked lists of cpu objects but rather
/// keeps all contact data as GPU buffers on the GPU device.

class ChApiGPU ChContactContainerGPU: public ChContactContainer {
CH_RTTI(ChContactContainerGPU, ChContactContainer)
	;

protected:

public:
	//
	// CONSTRUCTORS
	//

	ChContactContainerGPU();

	virtual ~ChContactContainerGPU();

	//
	// FUNCTIONS
	//

	ChGPUDataManager* data_container;
};

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
}// END_OF_NAMESPACE____

#endif
