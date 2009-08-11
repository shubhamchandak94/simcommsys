/*
 * ldpc.cpp
 *
 *  Created on: 9 Jul 2009
 *      Author: swesemeyer
 */

#include "ldpc.h"
#include "linear_code_utils.h"
#include "randgen.h"
#include "sumprodalg/spa_factory.h"
#include <math.h>
#include <sstream>

namespace libcomm {

// Determine debug level:
// 1 - Normal debug output only
// 2 - Show intermediate decoding output
#ifndef NDEBUG
#  undef DEBUG
#  define DEBUG 1
#endif

template <class GF_q, class real> void ldpc<GF_q, real>::init()
   {

   //compute the generator matrix for the code

   libbase::linear_code_utils<GF_q>::compute_dual_code(this->pchk_matrix,
         this->gen_matrix, this->perm_to_systematic);

   //as we define the LDPC code by its parity check matrix,H , the generator matrix
   //will be of the form (P|I) provided H was in systematic form when reduced to REF
   //if H wasn't then perm_to_systematic contains the permutation that transformed
   //H into systematic form which gives us the information we need to extract the
   //positions of the info symbols in G. In fact the last k values of perm_to_systematic
   //are those positions.

   this->dim_k = this->gen_matrix.size().rows();
   this->info_symb_pos.init(this->dim_k);
   for (int loop = 0; loop < this->dim_k; loop++)
      {
      this ->info_symb_pos(loop) = this->perm_to_systematic((this->length_n
            - this->dim_k) + loop);
      }
   }

template <class GF_q, class real> void ldpc<GF_q, real>::setreceiver(
      const array1vd_t& ptable)
   {

   this->current_iteration = 0;
   this->decodingSuccess = false;

#if DEBUG>=2
   libbase::trace << "\nThe received likelihoods are:\n";
   ptable.serialize(libbase::trace, ' ');
#endif

   this->received_probs.init(this->length_n);

   //initialise the computed probabilities by normalising the received likelihoods,
   //eg in the binary case compute
   //P(x_n=0)=P(y_n|x_n=0)/(P(y_n|x_n=0)+P(y|x_n=1))
   //P(x_n=1)=P(y_n|x_n=1)/(P(y_n|x_n=0)+P(y|x_n=1))
   //generalise this formula to all elements in GF_q
   //The result of this is that the computed probs for each entry adds to 1
   //and the most likely symbol has the highest probability
   real alpha = 0.0;

   for (int loop_n = 0; loop_n < this->length_n; loop_n++)
      {
      alpha = ptable(loop_n).sum(); //sum all the likelihoods in ptable(loop_n)
      this->received_probs(loop_n) = ptable(loop_n) / alpha; //normalise them
      }

#if DEBUG>=2
   libbase::trace << "\nThe normalised likelihoods are given by:\n";
   this->computed_probs.serialize(libbase::trace, ' ');
#endif

   //determine the most likely symbol
   libbase::linear_code_utils<GF_q, real>::get_most_likely_received_word(
         this->received_probs, this->received_word_sd, this->received_word_hd);

#if DEBUG>=2
   libbase::trace << "\nCurrently, the most likely received word is:\n";
   this->received_word_hd.serialize(libbase::trace, ' ');
   libbase::trace << "\n the symbol probabilities are given by:\n";
   this->received_word_sd.serialize(libbase::trace, ' ');
#endif
   //do we have a solution already?
   this->isCodeword();
   if (this->decodingSuccess)
      {
      this->computed_solution = this->received_probs;
      }

   //only do the rest if we don't have a codeword already
   if (!this->decodingSuccess)
      {
      this->spa_alg->spa_init(this->received_probs);
      }
   }
template <class GF_q, class real> void ldpc<GF_q, real>::isCodeword()
   {

   bool dec_success = true;
   int num_of_entries = 0;
   int pos_n = 0;

   GF_q tmp_val = GF_q(0);
   int rows = 0;
   while (dec_success && rows < this->dim_pchk)
      {
      tmp_val = GF_q(0);
      num_of_entries = this->N_m(rows).size();
      for (int loop = 0; loop < num_of_entries; loop++)
         {
         pos_n = this->N_m(rows)(loop) - 1;//we count from zero
         tmp_val += this->pchk_matrix(rows, pos_n) * received_word_hd(pos_n);
         }
      if (tmp_val != GF_q(0))
         {
         //the syndrome is non-zero
         dec_success = false;
         }
      rows++;
      }
   this->decodingSuccess = dec_success;
#if DEBUG>=2
   if (dec_success)
      {
      libbase::trace << "We have a solution\n";
      }
#endif

   }
template <class GF_q, class real> void ldpc<GF_q, real>::encode(
      const libbase::vector<int>& source, libbase::vector<int>& encoded)
   {
   libbase::linear_code_utils<GF_q>::encode_cw(this->gen_matrix, source,
         encoded);

#if DEBUG>=2
   this->received_word_hd = encoded;
   this->isCodeword();
   assertalways(this->decodingSuccess);
   //extract the info symbols from the codeword word and compare them to the original
   for (int loop_i = 0; loop_i < this->dim_k; loop_i++)
      {
      assertalways(source(loop_i) == encoded(this->info_symb_pos(loop_i)));
      }
#endif
#if DEBUG>=2
   libbase::trace << "The encoded word is:\n";
   encoded.serialize(libbase::trace, ' ');
   libbase::trace << "\n";
#endif
   }

template <class GF_q, class real> void ldpc<GF_q, real>::softdecode(
      array1vd_t& ri, libbase::vector<array1d_t>& ro)
   {
   //update the iteration counter
   this->current_iteration++;
   //init the received sd information vector;
   ri.init(this->dim_k);

   //Only continue if we haven't already computed a solution in a previous iteration
   if (this->decodingSuccess)
      {
      //initialise the output vector to the previously computed solution
      ro = this->computed_solution;
      }
   else
      {
      this->spa_alg->spa_iteration(ro);

#if DEBUG>=3
      libbase::trace << "\nThis is iteration: " << this->current_iteration << "\n";
      libbase::trace
      << "The newly computed normalised probabilities are given by:\n";
      ro.serialize(libbase::trace, ' ');
#endif

      //determine the most likely symbol
      libbase::linear_code_utils<GF_q, real>::get_most_likely_received_word(ro,
            this->received_word_sd, this->received_word_hd);

      //do we have a solution?
      this->isCodeword();
      if (this->decodingSuccess)
         {
         //store the solution for the next iteration
         this->computed_solution = ro;
         }

#if DEBUG>=2
      libbase::trace << "\nThis is iteration: " << this->current_iteration << "\n";
      libbase::trace << "The most likely received word is now given by:\n";
      this->received_word_hd.serialize(libbase::trace, ' ');
      libbase::trace << "\nIts symbol probabilities are given by:\n";
      this->received_word_sd.serialize(libbase::trace, ' ');
#endif
      //finished decoding
      }

   //extract the info symbols from the received word
   for (int loop_i = 0; loop_i < this->dim_k; loop_i++)
      {
      ri(loop_i) = ro(this->info_symb_pos(loop_i));
      }
#if DEBUG>=3
   libbase::trace << "\nThis is iteration: " << this->current_iteration << "\n";
   libbase::trace << "\nThe info symbol probabilities are given by:\n";
   ri.serialize(libbase::trace, ' ');
#endif

   }

template <class GF_q, class real> std::string ldpc<GF_q, real>::description() const
   {
   std::ostringstream sout;
   sout << "LDPC(n=" << this->length_n << ", m=" << this->dim_pchk << ", k="
         << this->dim_k << ", spa=" << this->spa_alg->spa_type() << ", iter="
         << this->max_iter << ") ";
#if DEBUG>=2
   this->serialize(libbase::trace);
   libbase::trace << "\n";
#endif

#if DEBUG>=2
   libbase::trace << "Its parity check matrix is given by:\n";
   this->pchk_matrix.serialize(libbase::trace, '\n');

   libbase::trace << "Its generator matrix is given by:\n";
   this->gen_matrix.serialize(libbase::trace, '\n');
   libbase::trace << "The information symbols are located in columns:\n";
   for (int loop = 0; loop < this->dim_k; loop++)
      {
      libbase::trace << this->info_symb_pos(loop) + 1 << " ";
      }
   libbase::trace << "\n";
#endif
   return sout.str();
   }

// object serialization - writing

/*
 *  This method write out the following format
 *
 * ldpc<gf<m,n>>
 * version
 * spa_type
 * max_iter
 * n m
 * max_n max_m
 * rand_prov_vals
 * seed (only present if rand_only if rand_prov_vals=random)
 * list of the number of non-zero entries for each column
 * list of the number of non-zero entries for each row
 * pos of each non-zero entry per col
 * vals of each non-zero entry per col - optional
 *
 * where
 * - ldpc<gf<n,m>> is actually written by the serialisation code and not this method
 * - version is the file format version used
 * - spa_type is the impl of the SPA used
 * - max_iter is the maximum number of iterations used by the decoder
 * - n is the length of the code
 * - m is the dimension of the parity check matrix
 * - max_n is the maxiumum number of non-zero entries per column
 * - max_m is the maximum number of non-zero entries per row
 * - rand_prov_vals is either ones, random, provided and indicates how
 *   the non-zero values are to be determined:
 *      - ones sets all entries to 1
 *      - provided reads them from the file
 *      - random generates non-zero entries randomly using the provided seed
 * - seed is an unsigned 32bit integer which contains the seed for the random number generator
 *   It is only present when rand_prov_vals has been set to random
 *
 *
 * Note that in the binary case the values are left out as they will be 1
 * anyway.
 *
 * given the matrix (5,3) over GF(4):
 *
 * 2 1 0 0 1
 * 0 2 2 3 3
 * 3 0 2 1 0
 *
 * An example file would be
 * ldpc<gf<2,0x7>>
 * #version
 * 2
 * #SPA
 * trad
 * #iter
 * 10
 * # length dim
 * 5 3
 * # max col/row weight
 * 2 2
 * #non-zero vals
 * provided
 * # col weights
 * 5
 * 2 2 2 2 2
 * # row weights
 * 3
 * 3 4 3
 * #non-zero pos in cols
 * 2
 * 1 3
 * 2
 * 1 2
 * 2
 * 2 3
 * 2
 * 2 3
 * 2
 * 1 2
 * #non-zero vals in cols
 * 2
 * 2 3
 * 2
 * 1 2
 * 2
 * 2 2
 * 2
 * 3 1
 * 2
 * 1 3
 *
 */
template <class GF_q, class real> std::ostream& ldpc<GF_q, real>::serialize(
      std::ostream& sout) const
   {

   assertalways(sout.good());
   sout << "#version of this file format\n";
   sout << 2 << "\n";
   sout << "#SPA type\n";
   sout << this->spa_alg->spa_type() << "\n";
   sout << "# number of iterations\n";
   sout << this->max_iter << "\n";
   sout << "# length n and dimension m\n";
   sout << this->length_n << " " << this->dim_pchk << "\n";
   sout << "#max col weight and max row weight\n";
   sout << this->max_col_weight << " " << this->max_row_weight << "\n";

   sout << "#non-zero values:ones/random/provided\n";
   sout << "#if random is chosen then it must be followed\n";
   sout << "#by a positive integer which acts as the seed for the\n";
   sout << "#random number generator\n";
   sout << this->rand_prov_values << "\n";
   if ("random" == this->rand_prov_values)
      {
      sout << "#seed value\n";
      sout << this->seed << "\n";
      }

   sout << "#the column weight vector\n";
   sout << this->col_weight;
   sout << "#the row weight vector\n";
   sout << this->row_weight;

   sout << "#the non zero pos per col\n";
   int num_of_non_zeros;
   int gf_val_int;
   int tmp_pos;
   for (int loop1 = 0; loop1 < this->length_n; loop1++)
      {
      sout << this->M_n(loop1);
      }
   // only output non-zero entries if needed

   if ("provided" == this->rand_prov_values)
      {
      libbase::vector<GF_q> non_zero_vals_in_col;
      sout << "#the non zero vals per col\n";
      for (int loop1 = 0; loop1 < this->length_n; loop1++)
         {
         num_of_non_zeros = this->M_n(loop1).size();
         non_zero_vals_in_col.init(num_of_non_zeros);
         for (int loop2 = 0; loop2 < num_of_non_zeros; loop2++)
            {
            tmp_pos = this->M_n(loop1)(loop2) - 1;
            gf_val_int = this->pchk_matrix(tmp_pos, loop1);
            non_zero_vals_in_col(loop2) = gf_val_int;
            }
         sout << non_zero_vals_in_col;
         }
      }
   return sout;
   }

// object serialization - loading

/* loading of the serialized codec information
 * This method expects the following format
 *
 * ldpc<gf<m,n>>
 * version
 * spa_type
 * max_iter
 * n m
 * max_n max_m
 * rand_prov_vals
 * seed (only present if rand_only if rand_prov_vals=random)
 * list of the number of non-zero entries for each column
 * list of the number of non-zero entries for each row
 * pos of each non-zero entry per col
 * vals of each non-zero entry per col - optional
 *
 * where
 * - ldpc<gf<n,m>> is actually written by the serialisation code and not this method
 * - version is the file format version used
 * - spa_type is the impl of the SPA used
 * - max_iter is the maximum number of iterations used by the decoder
 * - n is the length of the code
 * - m is the dimension of the parity check matrix
 * - max_n is the maxiumum number of non-zero entries per column
 * - max_m is the maximum number of non-zero entries per row
 * - rand_prov_vals is either ones, random, provided and indicates how
 *   the non-zero values are to be determined:
 *      - ones sets all entries to 1
 *      - provided reads them from the file
 *      - random generates non-zero entries randomly using the provided seed
 * - seed is an unsigned 32bit integer which contains the seed for the random number generator
 *   It is only present when rand_prov_vals has been set to random
 *
 * Note that in the binary case the values are left out as they will be 1
 * anyway.
 *
 *
 * given the matrix (5,3) over GF(4):
 *
 * 2 1 0 0 1
 * 0 2 2 3 3
 * 3 0 2 1 0
 *
 * An example file would be
 * ldpc<gf<2,0x7>>
 * #version
 * 2
 * #SPA
 * trad
 * #iter
 * 10
 * # length dim
 * 5 3
 * # max col/row weight
 * 2 2
 * #non-zero vals
 * provided
 * # col weights
 * 5
 * 2 2 2 2 2
 * # row weights
 * 3
 * 3 4 3
 * #non-zero pos in cols
 * 2
 * 1 3
 * 2
 * 1 2
 * 2
 * 2 3
 * 2
 * 2 3
 * 2
 * 1 2
 * #non-zero vals in cols
 * 2
 * 2 3
 * 2
 * 1 2
 * 2
 * 2 2
 * 2
 * 3 1
 * 2
 * 1 3
 *
 */

template <class GF_q, class real> std::istream& ldpc<GF_q, real>::serialize(
      std::istream& sin)
   {
   assertalways(sin.good());
   int version;
   sin >> libbase::eatcomments >> version;
   assertalways(version==2);
   string spa_type;
   sin >> libbase::eatcomments >> spa_type;

   sin >> libbase::eatcomments >> this->max_iter;
   assertalways(this->max_iter>=1);

   sin >> libbase::eatcomments >> this->length_n;
   sin >> libbase::eatcomments >> this->dim_pchk;

   sin >> libbase::eatcomments >> this->max_col_weight;
   sin >> libbase::eatcomments >> this->max_row_weight;

   libbase::randgen rng;
   //are the non-zero values provided or do we randomly generate them?
   sin >> libbase::eatcomments >> this->rand_prov_values;
   assertalways(("ones"==this->rand_prov_values) ||
         ("random"==this->rand_prov_values) ||
         ("provided"==this->rand_prov_values));
   if ("random" == this->rand_prov_values)
      {
      //read the seed value;
      sin >> libbase::eatcomments >> this-> seed;
      assertalways(seed>=0);
      rng.seed(seed);
      }
   //read the col weights and ensure they are sensible
   this->col_weight.init(this->length_n);
   sin >> libbase::eatcomments;
   sin >> this->col_weight;

   assertalways((1<=this->col_weight.min())&&(this->col_weight.max()<=this->max_col_weight));

   //read the row weights and ensure they are sensible
   this->row_weight.init(this->dim_pchk);
   sin >> libbase::eatcomments;
   sin >> this->row_weight;
   assertalways((0<this->row_weight.min())&&(this->row_weight.max()<=this->max_row_weight));

   this->M_n.init(this->length_n);

   //read the non-zero entries pos per col
   for (int loop1 = 0; loop1 < this->length_n; loop1++)
      {
      this->M_n(loop1).init(this->col_weight(loop1));
      sin >> libbase::eatcomments;
      sin >> this ->M_n(loop1);
      //ensure that the number of non-zero pos matches the previously read value
      assertalways(this->M_n(loop1).size()==this->col_weight(loop1));
      }

   //init the parity check matrix and read in the non-zero entries
   this->pchk_matrix.init(this->dim_pchk, this->length_n);
   this->pchk_matrix = GF_q(0);
   int tmp_entries;
   int tmp_pos;
   libbase::vector<GF_q> non_zero_vals;
   int num_of_non_zero_elements = GF_q::elements() - 1;
   for (int loop1 = 0; loop1 < this->length_n; loop1++)
      {
      tmp_entries = this->M_n(loop1).size();
      non_zero_vals.init(tmp_entries);
      if ("ones" == this->rand_prov_values)
         {
         //in the binary case the non-zero values are 1
         non_zero_vals = GF_q(1);
         }
      else if ("random" == this->rand_prov_values)
         {
         for (int loop_e = 0; loop_e < tmp_entries; loop_e++)
            {
            non_zero_vals(loop_e) = GF_q(1 + int(rng.ival(
                  num_of_non_zero_elements)));
            }
         assertalways(non_zero_vals.min()!=GF_q(0));
         }
      else
         {
         sin >> libbase::eatcomments;
         sin >> non_zero_vals;
         assertalways(non_zero_vals.min()!=GF_q(0));
         }
      for (int loop2 = 0; loop2 < tmp_entries; loop2++)
         {
         tmp_pos = this->M_n(loop1)(loop2) - 1;//we count from 0
         this->pchk_matrix(tmp_pos, loop1) = non_zero_vals(loop2);
         }
      }

   //derive the non-zero position per row from the parity check matrix
   this->N_m.init(this->dim_pchk);
   for (int loop1 = 0; loop1 < this->dim_pchk; loop1++)
      {
      tmp_entries = this->row_weight(loop1);
      this->N_m(loop1).init(tmp_entries);
      tmp_pos = 0;
      for (int loop2 = 0; loop2 < this->length_n; loop2++)
         {
         if (GF_q(0) != this->pchk_matrix(loop1, loop2))
            {
            assertalways(tmp_pos<tmp_entries);
            this->N_m(loop1)(tmp_pos) = loop2 + 1;//we count from 0;
            tmp_pos++;
            }
         }
      //tmp_pos should now correspond to the given row weight
      assertalways(tmp_pos==this->row_weight(loop1));
      }
   this->spa_alg = libcomm::spa_factory<GF_q, real>::get_spa(spa_type,
         this->length_n, this->dim_pchk, this->M_n, this->N_m,
         this->pchk_matrix);
   this->init();
   return sin;
   }

/*
 * This method outputs the alist format of this code as
 * described by MacKay @
 * http://www.inference.phy.cam.ac.uk/mackay/codes/alist.html
 *
 * n m q
 * max_n max_m
 * list of the number of non-zero entries for each column
 * list of the number of non-zero entries for each row
 * pos of each non-zero entry per col followed by their values (for GF(p>2))
 * pos of each non-zero entry per row followed by their values (for GF(p>2))
 *
 * where
 * - n is the length of the code
 * - m is the dimension of the parity check matrix
 * - q is only provided in non-binary cases where q=|GF(q)|
 * - max_n is the maxiumum number of non-zero entries per column
 * - max_m is the maximum number of non-zero entries per row
 * Note that the last set of positions and values are only used to
 * verify the information provided by the first set.
 *
 * Note that in the binary case the values are left out as they will be 1
 * anyway. An example row with 4 non-zero entries would look like this (assuming gf<3,0xB>
 * 1 1 3 3 9 7 10 2
 * ie the non-zero entries at pos 1,3,9 and 10 are 1,3,7 and 2 respectively
 *
 * Similarly a column with 3 non-zero entries over gf<3,0xB> would look like:
 * 3 4 6 3 12 6
 * ie the non-zero entries at pos 3,6 and 12 are 4, 3 and 6 respectively
 *
 * in the binary case the above row and column would simply be given by
 * 3 6 12
 * 1 3 9 10
 *
 * Also note that the alist format expects cols/rows with weight less than
 * the max col/row weight to be padded with extra 0s, eg
 * if a code has max col weight of 5 and a given col only has weight 3 then
 * it would look like
 * 1 4 5 0 0 (each entry immediately followed by the non-zero values in case of non-binary code)
 * These additional 0s need to be ignored
 *
 */

template <class GF_q, class real> std::ostream& ldpc<GF_q, real>::write_alist(
      std::ostream& sout) const
   {

   assertalways(sout.good());
   int numOfElements = GF_q::elements();
   bool nonbinary = (numOfElements > 2);

   // alist format version
   sout << this->length_n << " " << this->dim_pchk;
   if (nonbinary)
      {
      sout << " " << numOfElements;
      }
   sout << "\n";
   sout << this->max_col_weight << " " << this->max_row_weight << "\n";
   this->col_weight.serialize(sout, ' ');
   this->row_weight.serialize(sout, ' ');
   int num_of_non_zeros;
   int gf_val_int;
   int tmp_pos;

   //positions per column (and the non-zero values associated with them in the non-binary case)
   for (int loop1 = 0; loop1 < this->length_n; loop1++)
      {
      num_of_non_zeros = this->M_n(loop1).size().length();
      for (int loop2 = 0; loop2 < num_of_non_zeros; loop2++)
         {
         sout << this->M_n(loop1)(loop2) << " ";
         tmp_pos = this->M_n(loop1)(loop2) - 1;
         if (nonbinary)
            {
            gf_val_int = this->pchk_matrix(tmp_pos, loop1);
            sout << gf_val_int << " ";
            }
         }
      //add 0 zeros if necessary
      for (int loop2 = 0; loop2 < (this->max_col_weight - num_of_non_zeros); loop2++)
         {
         sout << "0 ";
         if (nonbinary)
            {
            sout << "0 ";
            }
         }
      sout << "\n";
      }

   //positions per row (and the non-zero values associated with them in the non-binary case)
   for (int loop1 = 0; loop1 < this->dim_pchk; loop1++)
      {
      num_of_non_zeros = this->N_m(loop1).size().length();
      for (int loop2 = 0; loop2 < num_of_non_zeros; loop2++)
         {
         sout << this->N_m(loop1)(loop2) << " ";
         tmp_pos = this->N_m(loop1)(loop2) - 1;
         if (nonbinary)
            {
            gf_val_int = this->pchk_matrix(loop1, tmp_pos);
            sout << gf_val_int << " ";
            }
         }
      //add 0 zeros if necessary
      for (int loop2 = 0; loop2 < (this->max_row_weight - num_of_non_zeros); loop2++)
         {
         sout << "0 ";
         if (nonbinary)
            {
            sout << "0 ";
            }
         }
      sout << "\n";
      }

   return sout;
   }

/* loading of the  alist format of an LDPC code
 * This method expects the following format
 *
 * n m q
 * max_n max_m
 * list of the number of non-zero entries for each column
 * list of the number of non-zero entries for each row
 * pos of each non-zero entry per col followed by their values
 * pos of each non-zero entry per row followed by their values
 *
 * where
 * - n is the length of the code
 * - m is the dimension of the parity check matrix
 * - q is only provided in non-binary cases where q=|GF(q)|
 * - max_n is the maxiumum number of non-zero entries per column
 * - max_m is the maximum number of non-zero entries per row
 * Note that the last set of positions and values are only used to
 * verify the information provided by the first set.
 *
 * Note that in the binary case the values are left out as they will be 1
 * anyway. An example row with 4 non-zero entries would look like this (assuming gf<3,0xB>
 * 1 1 3 3 9 7 10 2
 * ie the non-zero entries at pos 1,3,9 and 10 are 1,3,7 and 2 respectively
 *
 * Similarly a column with 3 non-zero entries over gf<3,0xB> would look like:
 * 3 4 6 3 12 6
 * ie the non-zero entries at pos 3,6 and 12 are 4, 3 and 6 respectively
 *
 * in the binary case the above row and column would simply be given by
 * 3 6 12
 * 1 3 9 10
 *
 * Also note that the alist format expects cols/rows with weight less than
 * the max col/row weight to be padded with extra 0s, eg
 * if a code has max col weight of 5 and a given col only has weight 3 then
 * it would look like
 * 1 4 5 0 0 (each entry immediately followed by the non-zero values in case of non-binary code)
 * These additional 0s need to be ignored
 *
 */

template <class GF_q, class real> std::istream& ldpc<GF_q, real>::read_alist(
      std::istream& sin)
   {
   assertalways(sin.good());
   int numOfElements = GF_q::elements();
   bool nonbinary = (numOfElements > 2);

   sin >> libbase::eatcomments >> this->length_n;
   sin >> libbase::eatcomments >> this->dim_pchk;
   if (nonbinary)
      {
      int q;
      sin >> libbase::eatcomments >> q;
      assertalways(numOfElements==q);
      }

   sin >> libbase::eatcomments >> this->max_col_weight;
   sin >> libbase::eatcomments >> this->max_row_weight;

   //read the col weights and ensure they are sensible
   int tmp_col_weight;
   this->col_weight.init(this->length_n);
   for (int loop1 = 0; loop1 < this->length_n; loop1++)
      {
      sin >> libbase::eatcomments >> tmp_col_weight;
      //is it between 1 and max_col_weight?
      assertalways((1<=tmp_col_weight)&&(tmp_col_weight<=this->max_col_weight));
      this ->col_weight(loop1) = tmp_col_weight;
      }

   //read the row weights and ensure they are sensible
   int tmp_row_weight;
   this->row_weight.init(this->dim_pchk);
   for (int loop1 = 0; loop1 < this->dim_pchk; loop1++)
      {
      sin >> libbase::eatcomments >> tmp_row_weight;
      //is it between 1 and max_row_weight?
      assertalways((1<=tmp_row_weight)&&(tmp_row_weight<=this->max_row_weight));
      this ->row_weight(loop1) = tmp_row_weight;
      }

   this->pchk_matrix.init(this->dim_pchk, this->length_n);
   this->M_n.init(this->length_n);

   //read the non-zero entries of the parity check matrix col by col
   //and ensure they make sense
   int tmp_entries;
   int tmp_pos;
   int tmp_val = 1; //this is the default value for the binary case
   for (int loop1 = 0; loop1 < this->length_n; loop1++)
      {
      //read in the non-zero row entries
      tmp_entries = this->col_weight(loop1);
      this->M_n(loop1).init(tmp_entries);
      for (int loop2 = 0; loop2 < tmp_entries; loop2++)
         {
         sin >> libbase::eatcomments >> tmp_pos;
         this->M_n(loop1)(loop2) = tmp_pos;
         tmp_pos--;//we start counting at 0 internally
         assertalways((0<=tmp_pos)&&(tmp_pos<this->dim_pchk));
         // read the non-zero element in the non-binary case
         if (nonbinary)
            {
            sin >> libbase::eatcomments >> tmp_val;
            assertalways((0<=tmp_val)&&(tmp_val<numOfElements));
            }
         this->pchk_matrix(tmp_pos, loop1) = GF_q(tmp_val);
         }
      //discard any padded 0 zeros if necessary
      for (int loop2 = 0; loop2 < (this->max_col_weight - tmp_entries); loop2++)
         {
         sin >> libbase::eatcomments >> tmp_pos;
         assertalways(0==tmp_pos);
         if (nonbinary)
            {
            sin >> libbase::eatcomments >> tmp_val;
            assertalways((0==tmp_val));
            }
         }
      }

   //read the non-zero entries of the parity check matrix row by row
   this->N_m.init(this->dim_pchk);
   for (int loop1 = 0; loop1 < this->dim_pchk; loop1++)
      {
      tmp_entries = this->row_weight(loop1);
      this->N_m(loop1).init(tmp_entries);
      for (int loop2 = 0; loop2 < tmp_entries; loop2++)
         {
         sin >> libbase::eatcomments >> tmp_pos;
         this->N_m(loop1)(loop2) = tmp_pos;
         tmp_pos--;//we start counting at 0 internally
         assertalways((0<=tmp_pos)&&(tmp_pos<this->length_n));
         // read the non-zero element in the non-binary case
         if (nonbinary)
            {
            sin >> libbase::eatcomments >> tmp_val;
            assertalways((0<=tmp_val)&&(tmp_val<numOfElements));
            }
         assertalways(GF_q(tmp_val)==this->pchk_matrix(loop1,tmp_pos));
         }
      //discard any padded 0 zeros if necessary
      for (int loop2 = 0; loop2 < (this->max_row_weight - tmp_entries); loop2++)
         {
         sin >> libbase::eatcomments >> tmp_pos;
         assertalways(0==tmp_pos);
         if (nonbinary)
            {
            sin >> libbase::eatcomments >> tmp_val;
            assertalways((0==tmp_val));
            }
         }
      }
   //set some default values
   this->max_iter = 50;
   if (GF_q::dimension() == 1)
      {
      this->rand_prov_values = "ones";
      }
   else
      {
      this->rand_prov_values = "provided";
      }
   this->spa_alg = libcomm::spa_factory<GF_q, real>::get_spa("gdl",
         this->length_n, this->dim_pchk, this->M_n, this->N_m,
         this->pchk_matrix);
   this->init();
   return sin;
   }

}//end namespace

//Explicit realisations
namespace libcomm {
using libbase::serializer;

template class ldpc<gf<1, 0x3> , double> ;
template <>
const serializer ldpc<gf<1, 0x3> , double>::shelper = serializer("codec",
      "ldpc<gf<1,0x3>>", ldpc<gf<1, 0x3> , double>::create);

template class ldpc<gf<2, 0x7> > ;
template <>
const serializer ldpc<gf<2, 0x7> >::shelper = serializer("codec",
      "ldpc<gf<2,0x7>>", ldpc<gf<2, 0x7> >::create);

template class ldpc<gf<3, 0xB> > ;
template <>
const serializer ldpc<gf<3, 0xB> >::shelper = serializer("codec",
      "ldpc<gf<3,0xB>>", ldpc<gf<3, 0xB> >::create);

template class ldpc<gf<4, 0x13> > ;
template <>
const serializer ldpc<gf<4, 0x13> >::shelper = serializer("codec",
      "ldpc<gf<4,0x13>>", ldpc<gf<4, 0x13> >::create);

template class ldpc<gf<6, 0x43> > ;
template <>
const serializer ldpc<gf<6, 0x43> >::shelper = serializer("codec",
      "ldpc<gf<6,0x43>>", ldpc<gf<6, 0x43> >::create);

template class ldpc<gf<7, 0x89> > ;
template <>
const serializer ldpc<gf<7, 0x89> >::shelper = serializer("codec",
      "ldpc<gf<7,0x89>>", ldpc<gf<7, 0x89> >::create);

template class ldpc<gf<8, 0x11D> > ;
template <>
const serializer ldpc<gf<8, 0x11D> >::shelper = serializer("codec",
      "ldpc<gf<8,0x11D>>", ldpc<gf<8, 0x11D> >::create);

template class ldpc<gf<9, 0x211> > ;
template <>
const serializer ldpc<gf<9, 0x211> >::shelper = serializer("codec",
      "ldpc<gf<9,0x211>>", ldpc<gf<9, 0x211> >::create);

template class ldpc<gf<10, 0x409> > ;
template <>
const serializer ldpc<gf<10, 0x409> >::shelper = serializer("codec",
      "ldpc<gf<10,0x409>>", ldpc<gf<10, 0x409> >::create);

}
