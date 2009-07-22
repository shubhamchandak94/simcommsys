#ifndef __gnrcc_h
#define __gnrcc_h

#include "ccfsm.h"
#include "serializer.h"

namespace libcomm {

/*!
 \brief   Generalized Non-Recursive Convolutional Code.
 \author  Johann Briffa

 \section svn Version Control
 - $Revision$
 - $Date$
 - $Author$

 \version 1.00 (13 Dec 2007)
 - Initial version; implements NRCC where polynomial coefficients are elements
 of a finite field.
 - Derived from gnrcc 1.00 and nrcc 1.70
 - The finite field is specified as a template parameter.

 \version 1.01 (4 Jan 2008)
 - removed serialization functions, which were redundant
 - removed resetcircular(), which is now implemented in fsm()
 */

template <class G>
class gnrcc : public ccfsm<G> {
protected:
   /*! \name FSM helper operations */
   int determineinput(int input) const;
   libbase::vector<G> determinefeedin(int input) const;
   // @}
   /*! \name Constructors / Destructors */
   //! Default constructor
   gnrcc()
      {
      }
   // @}
public:
   /*! \name Constructors / Destructors */
   gnrcc(const libbase::matrix<libbase::vector<G> >& generator) :
      ccfsm<G> (generator)
      {
      }
   gnrcc(const gnrcc<G>& x) :
      ccfsm<G> (x)
      {
      }
   ~gnrcc()
      {
      }
   // @}

   // FSM state operations (getting and resetting)
   void resetcircular(int zerostate, int n);

   // Description
   std::string description() const;

   // Serialization Support
DECLARE_SERIALIZER(gnrcc);
};

} // end namespace

#endif
