#ifndef __commsys_hist_symerr_h
#define __commsys_hist_symerr_h

#include "config.h"
#include "commsys.h"

namespace libcomm {

/*!
   \brief   Communication System Profiler.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   \version 1.10 (7 Jun 1999)
   modified the system to comply with codec 1.10.

   \version 1.20 (2 Sep 1999)
   added a hook for clients to know the number of frames simulated in a particular run.

   \version 1.21 (1 Mar 2002)
   edited the classes to be compileable with Microsoft extensions enabled - in practice,
   the major change is in for() loops, where MS defines scope differently from ANSI.
   Rather than taking the loop variables into function scope, we chose to avoid having
   more than one loop per function, by defining private helper functions (or doing away
   with them if there are better ways of doing the same operation).

   \version 1.22 (6 Mar 2002)
   changed vcs version variable from a global to a static class variable.
   also changed use of iostream from global to std namespace.

   \version 1.30 (19 Mar 2002)
   changed constructor to take also the modem and an optional puncturing system, besides
   the already present random source (for generating the source stream), the channel
   model, and the codec. This change was necessitated by the definition of codec 1.41.
   Also changed the sample loop to bail out after 0.5s rather than after at least 1000
   modulation symbols have been transmitted.

   \version 1.40 (19 Mar 2002)
   changed system to use commsys 1.41 as its base class, overriding cycleonce().

   \version 1.50 (30 Oct 2006)
   - defined class and associated data within "libcomm" namespace.
   - removed use of "using namespace std", replacing by tighter "using" statements as needed.

   \version 1.51 (24 Jan 2008)
   - Changed reference from channel to channel<sigspace>

   \version 1.52 (25 Jan 2008)
   - Updated to conform with commsys 2.00

   \version 1.53 (28 Jan 2008)
   - Changed reference from modulator to modulator<sigspace>

   \version 2.00 (19 Feb 2008)
   - Renamed to commsys_hist_symerr, indicating its function in computing histogram of
     symbol error count for each block simulated
   - Modified to count the number of symbol errors rather than the number of bit errors
*/

class commsys_hist_symerr : public commsys<sigspace> {
protected:
   void updateresults(libbase::vector<double>& result, const int i, const libbase::vector<int>& source, const libbase::vector<int>& decoded) const;
public:
   commsys_hist_symerr(libbase::randgen *src, codec *cdc, modulator<sigspace> *modem, puncture *punc, channel<sigspace> *chan);
   ~commsys_hist_symerr();
   //! For each iteration, we count the frequency of each possible symbol-error count, including zero
   int count() const { return ((tau-m)+1)*iter; };
};

}; // end namespace

#endif
