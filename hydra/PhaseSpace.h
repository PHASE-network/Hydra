/*----------------------------------------------------------------------------
 *
 *   Copyright (C) 2016 Antonio Augusto Alves Junior
 *
 *   This file is part of Hydra Data Analysis Framework.
 *
 *   Hydra is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Hydra is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Hydra.  If not, see <http://www.gnu.org/licenses/>.
 *
 *---------------------------------------------------------------------------*/

/*-
 * PhaseSpace.h
 *
 * Created on : Feb 25, 2016
 *      Author: Antonio Augusto Alves Junior
 */


/*!\file PhaseSpace.h
 * Implements the struct Events and the class PhaseSpace
 */

#ifndef PHASESPACE_H_
#define PHASESPACE_H_

#include <array>
#include <vector>
#include <string>
#include <map>
#include <omp.h>
#include <iostream>
#include <ostream>
#include <algorithm>
#include <time.h>
#include <stdio.h>
//#include <math.h>

#include <hydra/detail/Config.h>
#include <hydra/Types.h>
#include <hydra/Containers.h>
#include <hydra/Vector3R.h>
#include <hydra/Vector4R.h>
#include <hydra/Events.h>
#include <hydra/detail/functors/DecayMother.h>
#include <hydra/detail/functors/DecayMothers.h>
#include <hydra/detail/functors/FlagAcceptReject.h>
#include <hydra/detail/functors/IsAccepted.h>
#include <hydra/detail/utility/Generic.h>
#include <hydra/strided_iterator.h>
#include <hydra/detail/launch_decayers.inl>

#include <thrust/iterator/zip_iterator.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/tuple.h>
#include <thrust/extrema.h>
#include <thrust/random.h>
#include <thrust/distance.h>


#include <thrust/system/omp/execution_policy.h>


using namespace std;

namespace hydra {


template <size_t N, typename GRND=thrust::random::default_random_engine>
class PhaseSpace {

public:

	/**
	 * PhaseSpace ctor. Constructor of the phase-space generator takes as input parameters:
	 * - _MotherMass: the mass of the mother particle in Gev/c*c
	 * - _Masses: STL vector with the mass of the daughter particles.
	 */
	PhaseSpace(GReal_t _MotherMass, vector<GReal_t> _Masses) :
		fNDaughters(_Masses.size()),
		fSeed(1)

{

		fMassesD.resize(_Masses.size());
		thrust::copy(_Masses.begin(), _Masses.end(), fMassesD.begin());

		fMassesH.resize(_Masses.size());
		thrust::copy(_Masses.begin(), _Masses.end(), fMassesH.begin());

		GReal_t fTeCmTm = 0.0;

		fTeCmTm = _MotherMass; // total energy in C.M. minus the sum of the masses

		for (size_t n = 0; n < fNDaughters; n++) {
			fTeCmTm -= _Masses[n];
		}
		if (fTeCmTm < 0.0) {
			cout << "Not enough energy for this decay. Exit." << endl;
			exit(1);
		}

}    //decay


	~PhaseSpace() {}

	template<unsigned int BACKEND>
	void Generate(Vector4R const& mother, Events<N, BACKEND>& events)
	{
		/**
			 * Run the generator and calculate the maximum weight. It takes as input the fourvector of the mother particle
			 * in any system of reference. The daughters will be generated in this system.
			 */

#if(THRUST_DEVICE_SYSTEM==THRUST_DEVICE_BACKEND_CUDA && (BACKEND==device))
	cudaDeviceSetCacheConfig(cudaFuncCachePreferL1);
#endif


			DecayMother<N, BACKEND,GRND> decayer(mother,
					detail::ObjSelector<BACKEND==host>::select(fMassesH,fMassesD), fNDaughters, fSeed);
			detail::launch_decayer(decayer, events );

	}

	template<unsigned int BACKEND>
	void Generate(typename Events<N, BACKEND>::vector_particles_iterator mothers_begin,
			typename Events<N, BACKEND>::vector_particles_iterator mothers_end, Events<N, BACKEND>& events)
	{
		/**
		 * Run the generator and calculate the maximum weight. It takes as input the device vector with the four-vectors of the mother particle
		 * in any system of reference. The daughters will be generated in this system.
		 */

#if(THRUST_DEVICE_SYSTEM==THRUST_DEVICE_BACKEND_CUDA && (BACKEND==device))
	cudaDeviceSetCacheConfig(cudaFuncCachePreferL1);
#endif

		size_t n_mothers= thrust::distance(mothers_end,mothers_begin);


		if ( events.GetNEvents()  != n_mothers){
			cout << "NEvents != NMothers" << endl;
			exit(1);
		}

		DecayMothers<N, BACKEND,GRND> decayer(detail::ObjSelector<BACKEND==host>::select(fMassesH,fMassesD), fNDaughters, fSeed);
		detail::launch_decayer(decayer,mothers_begin, events );



	}

	inline GInt_t GetSeed() const	{
			return fSeed;
		}

	inline void SetSeed(GInt_t _seed) 	{
				fSeed=_seed;
			}


	/**
	 * PDK function
	 */
	inline GReal_t PDK(const GReal_t a, const GReal_t b, const GReal_t c) const {
		//the PDK function
		GReal_t x = (a - b - c) * (a + b + c) * (a - b + c) * (a + b - c);
		x = sqrt(x) / (2 * a);
		return x;
	}

private:


	GInt_t  fNDaughters;///< Number of daughters.
	GInt_t  fSeed;///< seed.
	mc_device_vector<GReal_t> fMassesD;
	mc_host_vector<GReal_t> fMassesH;



};

}//namespace hydra
#include <hydra/detail/PhaseSpace.inl>

#endif /* PHASESPACE_H_ */
