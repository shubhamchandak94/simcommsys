#ifndef __turbo_h
#define __turbo_h

#include "config.h"
#include "serializer.h"

#include "codec_softout.h"
#include "fsm.h"
#include "interleaver.h"
#include "bcjr.h"
#include "itfunc.h"

#include <stdlib.h>
#include <math.h>

namespace libcomm {

/*!
   \brief   Turbo decoding algorithm.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   All internal metrics are held as type 'real', which is user-defined. This
   allows internal working at any required level of accuracy. This is necessary
   because the internal matrics have a very wide dynamic range, which increases
   exponentially with block size 'tau'. Actually, the required range is within
   [1,0), but very large exponents are required. (For BCJR sub-component)

   \note Memory is allocaed only on first call to demodulate/decode. This
         reduces memory requirements in cases where classes are instantiated
         but not actually used for decoding (e.g. in master node on a
         distributed Monte Carlo simulation)

   \note Since puncturing is not handled within the codec, for the moment, the
         modification for stipple puncturing with simile interleavers is not
         performed.

   \note The template class 'dbl', which defaults to 'double', defines the
         numerical representation for inter-iteration statistics. This became
         necessary for the parallel decoding structure, where the range of
         extrinsic information is much larger than for serial decoding;
         furthermore, this range increases with the number of iterations
         performed.

   \note Serialization is versioned; for compatibility, earlier versions are
         interpreted as v.0; a flat interleaver is automatically used for the
         first encoder in these cases.
   
   \note Debug output is only enabled if pre-processor define DEBUG is set at
         a value of 2 or above.

   \todo Remove tau from user parameters, as this can be derived from
         interleavers (requires a change to interleaver interface)

   \todo Fix terminated sequence encoding (implicitly assume a flat first
         interleaver)

   \todo Standardize encoding/decoding of multiple symbols within a larger
         symbol space; this parallels what was done in ccfsm.
*/

template <class real, class dbl=double>
class turbo : public codec_softout, private bcjr<real,dbl> {
private:
   /*! \name User-defined parameters */
   //!< Set of interleavers, one per parity sequence (including first set)
   libbase::vector<interleaver *> inter;
   fsm      *encoder;      //!< Encoder object (same for all parity sequences)
   int      tau;           //!< Length of interleavers (information sequence + tail)
   int      iter;          //!< Number of iterations to perform
   bool     endatzero;     //!< Flag to indicate that trellises are terminated
   bool     parallel;      //!< Flag to enable parallel decoding algorithm (rather than serial)
   bool     circular;      //!< Flag to indicate trellis tailbiting
   // @}
   /*! \name Internal object representation */
   bool initialised;       //!< Initially false, becomes true when memory is initialised
   libbase::matrix<dbl> rp;   //!< A priori intrinsic source statistics (natural)
   libbase::matrix<dbl> ri;   //!< A posteriori source statistics (natural)
   libbase::vector< libbase::matrix<dbl> > R;   //!< A priori intrinsic encoder-output statistics (interleaved)
   libbase::vector< libbase::matrix<dbl> > ra;  //!< A priori extrinsic source statistics
   libbase::vector< libbase::vector<dbl> > ss;  //!< Holder for start-state probabilities (used with circular trellises)
   libbase::vector< libbase::vector<dbl> > se;  //!< Holder for end-state probabilities (used with circular trellises)
   // @}
   /*! \name Temporary variables */
   libbase::matrix<dbl> rai;  //!< Temporary statistics (interleaved version of ra)
   libbase::matrix<dbl> rii;  //!< Temporary statistics (interleaved version of ri)
   // @}
   /*! \name Internal functions */
   //! Memory allocator (for internal use only)
   void allocate();
   // wrapping functions
   static void work_extrinsic(const libbase::matrix<dbl>& ra, const libbase::matrix<dbl>& ri, const libbase::matrix<dbl>& r, libbase::matrix<dbl>& re);
   void bcjr_wrap(const int set, const libbase::matrix<dbl>& ra, libbase::matrix<dbl>& ri, libbase::matrix<dbl>& re);
   void decode_serial(libbase::matrix<dbl>& ri);
   void decode_parallel(libbase::matrix<dbl>& ri);
   // @}
protected:
   /*! \name Internal functions */
   /*! \brief Read access for a-posteriori statistics
      This access is available only for derived classes, and was initially
      added for use by the diffturbo class.
   */
   double aposteriori(const int t, const int i) const { return ri(t,i); };
   void init();
   void free();
   void reset();
   // @}
   /*! \name Constructors / Destructors */
   //! Default constructor
   turbo();
   // @}
public:
   /*! \name Constructors / Destructors */
   turbo(const fsm& encoder, const int tau, const libbase::vector<interleaver *>& inter, \
      const int iter, const bool endatzero, const bool parallel=false, const bool circular=false);
   ~turbo() { free(); };
   // @}

   // Codec operations
   void seedfrom(libbase::random& r);
   void encode(libbase::vector<int>& source, libbase::vector<int>& encoded);
   void translate(const libbase::matrix<double>& ptable);
   void decode(libbase::vector<int>& decoded);
   void decode(libbase::matrix<double>& ri);
   void decode(libbase::matrix<double>& ri, libbase::matrix<double>& ro);

   // Codec information functions - fundamental
   int block_size() const { return tau; };
   int num_inputs() const { return encoder->num_inputs(); };
   int num_outputs() const { return int(num_inputs()*pow(enc_parity(),num_sets())); };
   int output_alphabet() const { return libbase::gcd(num_inputs(),enc_parity()); };
   int tail_length() const { return endatzero ? encoder->mem_order() : 0; };
   int num_iter() const { return iter; };

   /*! \name Codec information functions - internal */
   int num_sets() const { return inter.size(); };
   int enc_states() const { return encoder->num_states(); };
   int enc_outputs() const { return encoder->num_outputs(); };
   int enc_parity() const { return enc_outputs()/num_inputs(); };
   // @}

   // Description
   std::string description() const;

   // Serialization Support
   DECLARE_SERIALIZER(turbo)
};

}; // end namespace

#endif
