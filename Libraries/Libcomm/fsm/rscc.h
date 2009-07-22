#ifndef __rscc_h
#define __rscc_h

#include "ccbfsm.h"
#include "serializer.h"

namespace libcomm {

/*!
 \brief   Recursive Systematic Convolutional Coder.
 \author  Johann Briffa

 \section svn Version Control
 - $Revision$
 - $Date$
 - $Author$

 \version 1.01 (4 Nov 2001)
 added a function which outputs details on the finite state machine (in accordance
 with fsm 1.10)

 \version 1.10 (28 Feb 2002)
 added serialization facility (in accordance with fsm 1.20)

 \version 1.11 (6 Mar 2002)
 changed vcs version variable from a global to a static class variable.
 also changed use of iostream from global to std namespace.
 also modified system to use '+' instead of ',' for concatenating bitfields, to
 conform with bitfield 1.11.

 \version 1.12 (11 Mar 2002)
 changed the definition of the cloning operation to be a const member, to conform with
 fsm 1.22. Also, made that function inline.

 \version 1.20 (11 Mar 2002)
 updated the system to conform with the completed serialization protocol (in conformance
 with fsm 1.30), by adding the necessary name() function, and also by removing the class
 name reading/writing in serialize(); this is now done only in the stream << and >>
 functions. serialize() assumes that the correct class is being read/written. We also
 add a static serializer member and initialize it with this class's name and the static
 constructor (adding that too, together with the necessary private empty constructor).
 Also made the fsm object a public base class, rather than a virtual public one, since
 this was affecting the transfer of virtual functions within the class (causing access
 violations). Also changed the access to the init() function to protected, which should
 make deriving from this class a bit easier.

 \version 1.21 (14 Mar 2002)
 fixed a bug in the copy constructor - 'm' was not copied.

 \version 1.22 (23 Mar 2002)
 changed creation and init functions; number of inputs and outputs are no longer
 specified directly, but are taken from the generator matrix size. Also changed the
 serialization functions; now these do not read/write k & n but only the generator.

 \version 1.30 (27 Mar 2002)
 changed descriptive output function to conform with fsm 1.40.

 \version 1.31 (13 Apr 2002)
 modified init() and constructor to take generator matrix by reference rather than
 directly.

 \version 1.40 (8 Jan 2006)
 modified class to conform with fsm 1.50.

 \version 1.50 (30 Oct 2006)
 - defined class and associated data within "libcomm" namespace.
 - removed use of "using namespace std", replacing by tighter "using" statements as needed.

 \version 1.51 (29 Oct 2007)
 - updated clone() to return this object's type, rather than its base class type. [cf. Stroustrup 15.6.2]

 \version 1.60 (3-5 Dec 2007)
 - updated output() as per fsm 1.70
 - changed register set from array to vector
 - implemented advance() and output(), and changed step() to use them
 - removed implementation of step() in favor of the default provided by fsm
 - renamed constraint length from K to nu, in favor of convention in Lin & Costello, 2004

 \version 1.70 (5 Dec 2007)
 - derived class from the newly-formed ccbfsm
 - extracted determinefeedin()
 - created determineinput() and modified advance() & output() to use it
 - refactored output()
 - extracted advance() and output() to ccbfsm
 - made determineinput() and determinefeedin() protected, like the virtual members they implement
 - cleaned up order of members and documentation

 \version 1.71 (4 Jan 2008)
 - removed serialization functions, which were redundant
 - removed resetcircular(), which is now implemented in fsm()
 */

class rscc : public ccbfsm {
protected:
   libbase::bitfield determineinput(const int input) const;
   libbase::bitfield determinefeedin(const int input) const;
   rscc()
      {
      }
public:
   /*! \name Constructors / Destructors */
   rscc(libbase::matrix<libbase::bitfield> const &generator) :
      ccbfsm(generator)
      {
      }
   rscc(rscc const &x) :
      ccbfsm(x)
      {
      }
   ~rscc()
      {
      }
   // @}

   // FSM state operations (getting and resetting)
   void resetcircular(int zerostate, int n);

   // Description
   std::string description() const;

   // Serialization Support
DECLARE_SERIALIZER(rscc);
};

} // end namespace

#endif
