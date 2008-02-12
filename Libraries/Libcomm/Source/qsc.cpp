/*!
   \file

   \par Version Control:
   - $Revision: 561 $
   - $Date: 2008-01-28 10:54:08 +0000 (Mon, 28 Jan 2008) $
   - $Author: jabriffa $
*/

#include "qsc.h"
#include "gf.h"
#include <sstream>

namespace libcomm {

// Channel parameter handling

template <class G> void qsc<G>::set_parameter(const double Ps)
   {
   assert(Ps >=0 && Ps < 1);
   qsc::Ps = Ps;
   }

// Channel function overrides

/*!
   \copydoc channel::corrupt()

   The channel model implemented is described by the following state diagram:
   \dot
   digraph bsidstates {
      // Make figure left-to-right
      rankdir = LR;
      // state definitions
      this [ shape=circle, color=gray, style=filled, label="t(i)" ];
      next [ shape=circle, color=gray, style=filled, label="t(i+1)" ];
      // path definitions
      this -> next [ label="1-Ps" ];
      this -> Substitute [ label="Ps" ];
      Substitute -> next;
   }
   \enddot

   For symbols that are substituted, any of the remaining symbols are equally
   likely.
*/
template <class G> G qsc<G>::corrupt(const G& s)
   {
   const double p = r.fval();
   if(p < Ps)
      return s + (r.ival(G::elements()-1) + 1);
   return s;
   }

// description output

template <class G> std::string qsc<G>::description() const
   {
   std::ostringstream sout;
   sout << G::elements() << "-ary Symmetric channel";
   return sout.str();
   }

// Explicit Realizations

template class qsc< libbase::gf<1,0x3> >;
template <> const libbase::serializer qsc< libbase::gf<1,0x3> >::shelper("channel", "qsc<gf<1,0x3>>", qsc< libbase::gf<1,0x3> >::create);
template class qsc< libbase::gf<2,0x7> >;
template <> const libbase::serializer qsc< libbase::gf<2,0x7> >::shelper("channel", "qsc<gf<2,0x7>>", qsc< libbase::gf<2,0x7> >::create);
template class qsc< libbase::gf<3,0xB> >;
template <> const libbase::serializer qsc< libbase::gf<3,0xB> >::shelper("channel", "qsc<gf<3,0xB>>", qsc< libbase::gf<3,0xB> >::create);
template class qsc< libbase::gf<4,0x13> >;
template <> const libbase::serializer qsc< libbase::gf<4,0x13> >::shelper("channel", "qsc<gf<4,0x13>>", qsc< libbase::gf<4,0x13> >::create);

}; // end namespace
