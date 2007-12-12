/*!
   \file

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$
*/

#include "ccfsm.h"
#include <iostream>
#include <sstream>

namespace libcomm {

using libbase::vector;

// Internal functions

/*! \brief Initialization
*/
template <class G> void ccfsm<G>::init(const libbase::matrix<vector<G>>& generator)
   {
   // copy automatically what we can
   gen = generator;
   k = gen.xsize();
   n = gen.ysize();
   // set default value to the rest
   m = 0;
   // check that the generator matrix is valid (correct sizes) and create shift registers
   reg.init(k);
   nu = 0;
   for(int i=0; i<k; i++)
      {
      // assume with of register of input 'i' from its generator sequence for first output
      int m = gen(i,0).size() - 1;
      reg(i).init(m);
      nu += m;
      // check that the gen. seq. for all outputs are the same length
      for(int j=1; j<n; j++)
         if(gen(i,j).size() != m+1)
            {
            std::cerr << "FATAL ERROR (ccfsm): Generator sequence must have constant width for each input.\n";
            exit(1);
            }
      // update memory order
      if(m > ccfsm::m)
         ccfsm::m = m;
      }
   }


// Constructors / Destructors

/*! \brief Principal constructor
*/
template <class G> ccfsm<G>::ccfsm(const libbase::matrix<vector<G>>& generator)
   {
   init(generator);
   }

/*! \brief Copy constructor
*/
template <class G> ccfsm<G>::ccfsm(const ccfsm& x)
   {
   // copy automatically what we can
   k = x.k;
   n = x.n;
   nu = x.nu;
   m = x.m;
   gen = x.gen;
   reg = x.reg;
   }


// FSM state operations (getting and resetting)

/*! \brief The current state
    \return A unique integer representation of the current state

    \note Lower-order inputs get lower-order positions within the state representation.

    \note Left-most register positions (ie. those closest to the input junction) get
          higher-order positions within the state representation.

    \invariant The state value should always be between 0 and num_states()-1
*/
template <class G> int ccfsm<G>::state() const
   {
   int newstate = 0;
   for(int i=k-1; i>=0; i--)
      for(int j=reg(i).size()-1; j>=0; j--)
         {
         newstate *= G::elements();
         newstate += reg(i)(j);
         }
   assert(newstate>=0 && newstate<num_states());
   return newstate;
   }

/*! \brief Reset to a specified state
*/
template <class G> void ccfsm<G>::reset(int state)
   {
   vector<G> newstate;
   newstate.resize(nu);
   newstate = state;
   for(int i=0; i<k; i++)
      {
      int size = reg(i).size();
      // check for case where no memory is associated with the input bit
      if(size > 0)
         {
         reg(i) = newstate.extract(size-1, 0);
         newstate >>= size;
         }
      }
   }


// FSM operations (advance/output/step)

/*! \brief Feeds the specified input and advances the state
*/
template <class G> void ccfsm<G>::advance(int& input)
   {
   vector<G> ip = determineinput(input);
   vector<G> sin = determinefeedin(ip);
   // Update input
   input = ip;
   // Compute next state
   for(int i=0; i<k; i++)
      reg(i) = sin[i] >> reg(i);
   }

/*! \brief Computes the output for the given input and the present state
*/
template <class G> int ccfsm<G>::output(const int& input) const
   {
   vector<G> ip = determineinput(input);
   vector<G> sin = determinefeedin(ip);
   // Compute output
   vector<G> op(0,0);
   for(int j=0; j<n; j++)
      {
      vector<G> thisop(0,1);
      for(int i=0; i<k; i++)
         thisop ^= (sin[i] + reg(i)) * gen(i,j);
      op = thisop + op;
      }
   return op;
   }


// Description & Serialization

/*! \brief Description output - common part only, must be preceded by specific name
*/
template <class G> std::string ccfsm<G>::description() const
   {
   std::ostringstream sout;
   sout << "(nu=" << nu << ", rate " << k << "/" << n << ", G=[";
   for(int i=0; i<k; i++)
      for(int j=0; j<n; j++)
         sout << gen(i,j) << (j==n-1 ? (i==k-1 ? "])" : "; ") : ", ");
   return sout.str();
   }

/*! \brief Serialization output
*/
template <class G> std::ostream& ccfsm<G>::serialize(std::ostream& sout) const
   {
   sout << gen;
   return sout;
   }

/*! \brief Serialization input
*/
template <class G> std::istream& ccfsm<G>::serialize(std::istream& sin)
   {
   sin >> gen;
   init(gen);
   return sin;
   }

}; // end namespace