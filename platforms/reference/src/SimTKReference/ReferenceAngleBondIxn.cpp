
/* Portions copyright (c) 2006 Stanford University and Simbios.
 * Contributors: Pande Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <sstream>

#include "../SimTKUtilities/SimTKOpenMMCommon.h"
#include "../SimTKUtilities/SimTKOpenMMLog.h"
#include "../SimTKUtilities/SimTKOpenMMUtilities.h"
#include "ReferenceAngleBondIxn.h"
#include "ReferenceForce.h"

/**---------------------------------------------------------------------------------------

   ReferenceAngleBondIxn constructor

   --------------------------------------------------------------------------------------- */

ReferenceAngleBondIxn::ReferenceAngleBondIxn( ){

   // ---------------------------------------------------------------------------------------

   // static const char* methodName = "\nReferenceAngleBondIxn::ReferenceAngleBondIxn";

   // ---------------------------------------------------------------------------------------

}

/**---------------------------------------------------------------------------------------

   ReferenceAngleBondIxn destructor

   --------------------------------------------------------------------------------------- */

ReferenceAngleBondIxn::~ReferenceAngleBondIxn( ){

   // ---------------------------------------------------------------------------------------

   // static const char* methodName = "\nReferenceAngleBondIxn::~ReferenceAngleBondIxn";

   // ---------------------------------------------------------------------------------------

}

/**---------------------------------------------------------------------------------------

   Get dEdR and energy term for angle bond

   @param  cosine               cosine of angle
   @param  angleParameters      angleParameters: angleParameters[0] = angle in radians
                                                 angleParameters[1] = k (force constant)
   @param  dEdR                 output dEdR
   @param  energyTerm           output energyTerm

   @return ReferenceForce::DefaultReturn

   --------------------------------------------------------------------------------------- */

int ReferenceAngleBondIxn::getPrefactorsGivenAngleCosine( RealOpenMM cosine, RealOpenMM* angleParameters,
                                                          RealOpenMM* dEdR, RealOpenMM* energyTerm ) const {

   // ---------------------------------------------------------------------------------------

   // static const std::string methodName = "\nReferenceAngleBondIxn::getPrefactorsGivenAngleCosine";

   static const RealOpenMM zero        = 0.0;
   static const RealOpenMM one         = 1.0;
   static const RealOpenMM half        = 0.5;

   // ---------------------------------------------------------------------------------------

   RealOpenMM angle;
   if( cosine >= one ){
      angle = zero;
   } else if( cosine <= -one ){
      angle = PI_M;
   } else {
      angle = ACOS(cosine);
   }
   RealOpenMM deltaIdeal         = angle - angleParameters[0];
   RealOpenMM deltaIdeal2        = deltaIdeal*deltaIdeal;

  *dEdR                          = angleParameters[1]*deltaIdeal;
  *energyTerm                    = half*angleParameters[1]*deltaIdeal2;

   return ReferenceForce::DefaultReturn;

}

/**---------------------------------------------------------------------------------------

   Calculate Angle Bond ixn

   @param atomIndices      two bond indices
   @param atomCoordinates  atom coordinates
   @param parameters       parameters: parameters[0] = ideal bond length
                                       parameters[1] = bond k (includes factor of 2)
   @param forces           force array (forces added)
   @param energiesByBond   energies by bond: energiesByBond[bondIndex]
   @param energiesByAtom   energies by atom: energiesByAtom[atomIndex]

   @return ReferenceForce::DefaultReturn

   --------------------------------------------------------------------------------------- */

int ReferenceAngleBondIxn::calculateBondIxn( int* atomIndices,
                                             RealOpenMM** atomCoordinates,
                                             RealOpenMM* parameters,
                                             RealOpenMM** forces,
                                             RealOpenMM* energiesByBond,
                                             RealOpenMM* energiesByAtom ) const {

   // ---------------------------------------------------------------------------------------

   // static const char* methodName = "\nReferenceAngleBondIxn::calculateBondIxn";

   // ---------------------------------------------------------------------------------------

   static const std::string methodName = "\nReferenceAngleBondIxn::calculateBondIxn";

   // constants -- reduce Visual Studio warnings regarding conversions between float & double

   static const RealOpenMM zero        =  0.0;
   static const RealOpenMM one         =  1.0;
   static const RealOpenMM two         =  2.0;
   static const RealOpenMM three       =  3.0;
   static const RealOpenMM oneM        = -1.0;

   static const int threeI             = 3;

   // debug flag

   static const int debug              = 0;

   static const int LastAtomIndex      = 3;

   RealOpenMM deltaR[2][ReferenceForce::LastDeltaRIndex];

   // ---------------------------------------------------------------------------------------

   // get deltaR, R2, and R between 2 atoms

   int atomAIndex = atomIndices[0];
   int atomBIndex = atomIndices[1];
   int atomCIndex = atomIndices[2];
   ReferenceForce::getDeltaR( atomCoordinates[atomAIndex], atomCoordinates[atomBIndex], deltaR[0] );  
   ReferenceForce::getDeltaR( atomCoordinates[atomCIndex], atomCoordinates[atomBIndex], deltaR[1] );  

   RealOpenMM pVector[threeI];
   SimTKOpenMMUtilities::crossProductVector3( deltaR[0], deltaR[1], pVector );
   RealOpenMM rp              = DOT3( pVector, pVector );
   rp                         = SQRT( rp );
   if( rp < 1.0e-06 ){
      rp = (RealOpenMM) 1.0e-06;
   }   
   RealOpenMM dot             = DOT3( deltaR[0], deltaR[1] );
   RealOpenMM cosine          = dot/SQRT( (deltaR[0][ReferenceForce::R2Index]*deltaR[1][ReferenceForce::R2Index]) );

   RealOpenMM dEdR;
   RealOpenMM energy;
   getPrefactorsGivenAngleCosine( cosine, parameters, &dEdR, &energy );

   RealOpenMM termA           =  dEdR/(deltaR[0][ReferenceForce::R2Index]*rp);
   RealOpenMM termC           = -dEdR/(deltaR[1][ReferenceForce::R2Index]*rp);

   RealOpenMM deltaCrossP[LastAtomIndex][threeI];
   SimTKOpenMMUtilities::crossProductVector3( deltaR[0], pVector, deltaCrossP[0] );
   SimTKOpenMMUtilities::crossProductVector3( deltaR[1], pVector, deltaCrossP[2] );

   for( int ii = 0; ii < threeI; ii++ ){
      deltaCrossP[0][ii] *= termA;
      deltaCrossP[2][ii] *= termC;
      deltaCrossP[1][ii]  = oneM*(deltaCrossP[0][ii] + deltaCrossP[2][ii]);
   }   

   // accumulate forces
 
   for( int jj = 0; jj < LastAtomIndex; jj++ ){
      for( int ii = 0; ii < threeI; ii++ ){
         forces[atomIndices[jj]][ii] += deltaCrossP[jj][ii];
      }   
   }   

   // accumulate energies

   updateEnergy( energy, energiesByBond, LastAtomIndex, atomIndices, energiesByAtom );

   // debug 

   if( debug ){
      static bool printHeader = false;
      std::stringstream message;
      message << methodName;
      message << std::endl;
      if( !printHeader  ){  
         printHeader = true;
         message << std::endl;
         message << methodName.c_str() << " a0 k [c q p s] r1 r2  angle dt rp p[] dot cosine angle dEdR*r F[]" << std::endl;
      }   

      message << std::endl;
      for( int ii = 0; ii < 3; ii++ ){
         message << " Atm " << atomIndices[ii] << " [" << atomCoordinates[ii][0] << " " << atomCoordinates[ii][1] << " " << atomCoordinates[ii][2] << "] ";
      }
      message << std::endl << " Delta:";
      for( int ii = 0; ii < 2; ii++ ){
         message << " [";
         for( int jj = 0; jj < ReferenceForce::LastDeltaRIndex; jj++ ){
            message << deltaR[ii][jj] << " ";
         }
         message << "]";
      }
      message << std::endl;

      message << " a0="     << parameters[0];
      message << " k="      << parameters[1];
      message << " rab2="  << deltaR[0][ReferenceForce::R2Index];
      message << " rcb2="  << deltaR[1][ReferenceForce::R2Index];
      message << std::endl << "            ";
      message << " rp="    << rp; 
      message << " p[";
      SimTKOpenMMUtilities::formatRealStringStream( message, pVector );
      message << "] dot="   << dot;
      message << " cos="   << cosine;

      message << std::endl << "            ";
      message << " dEdr=" << dEdR;
      message << " trmA=" << termA;
      message << " trmC=" << termC;

      message << " E=" << energy << " F=compute force; f=cumulative force";

      message << std::endl << "            ";
      for( int ii = 0; ii < 3; ii++ ){
         message << " F" << (ii+1) << "[";
         SimTKOpenMMUtilities::formatRealStringStream( message, deltaCrossP[ii], threeI );
         message << "]";
      }   
      message << std::endl << "            ";

      for( int ii = 0; ii < LastAtomIndex; ii++ ){
         message << " f" << (ii+1) << "[";
         SimTKOpenMMUtilities::formatRealStringStream( message, forces[atomIndices[ii]], threeI );
         message << "]";
      }

      SimTKOpenMMLog::printMessage( message );
   }   

   return ReferenceForce::DefaultReturn;
}
