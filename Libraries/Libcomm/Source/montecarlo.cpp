/*!
   \file

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$
*/

#include "montecarlo.h"

#include "fsm.h"
#include "itfunc.h"
#include "secant.h"
#include "truerand.h"
#include <iostream>
#include <sstream>
#include <limits>

#ifdef min
#  undef min
#  undef max
#endif

namespace libcomm {

using std::cerr;
using libbase::trace;
using libbase::vector;

const int montecarlo::min_samples = 128;

// worker processes

void montecarlo::slave_getcode(void)
   {
   delete system;
   // Receive system as a string
   std::string systemstring;
   if(!receive(systemstring))
      exit(1);
   // Create system object from serialization
   std::istringstream is(systemstring);
   is >> system;
   // Compute its digest
   sysdigest.reset();
   is.seekg(0, std::ios::beg);
   sysdigest.process(is);
   // Tell the user what we've done
   cerr << "Date: " << libbase::timer::date() << "\n";
   cerr << system->description() << "\n";
   cerr << "Digest: " << sysdigest << "\n";
   }

void montecarlo::slave_getsnr(void)
   {
   libbase::truerand r;
   const libbase::int32u seed = r.ival(1<<16);
   system->seed(seed);
   double x;
   if(!receive(x))
      exit(1);
   system->set_parameter(x);

   cerr << "Date: " << libbase::timer::date() << "\n";
   cerr << "Seed: " << seed << "\n";
   cerr << "Simulating system at parameter = " << system->get_parameter() << "\n";
   }

void montecarlo::slave_work(void)
   {
   const int count = system->count();
   // Running values
   vector<double> sum(count);
   vector<double> sumsq(count);
   // Initialise running values
   samplecount = 0;
   sum = 0;
   sumsq = 0;

   // Iterate for 500ms, which is a good compromise between efficiency and usability
   libbase::timer t;
   while(t.elapsed() < 0.5)
      sampleandaccumulate(sum, sumsq);
   t.stop();   // to avoid expiry

   // Send system digest and current parameter back to master
   if(!send(sysdigest) || !send(system->get_parameter()))
      exit(1);
   // Send accumulated results back to master
   if(!send(sum) || !send(sumsq) || !send(samplecount))
      exit(1);

   // print something to inform the user of our progress
   vector<double> result(count);
   vector<double> tolerance(count);
   double acc = updateresults(result, tolerance, sum, sumsq);
   display(samplecount, (acc<1 ? 100*acc : 99), result.min());
   }

// helper functions

void montecarlo::createfunctors(void)
   {
   fgetcode = new libbase::specificfunctor<montecarlo>(this, &libcomm::montecarlo::slave_getcode);
   fgetsnr = new libbase::specificfunctor<montecarlo>(this, &libcomm::montecarlo::slave_getsnr);
   fwork = new libbase::specificfunctor<montecarlo>(this, &libcomm::montecarlo::slave_work);
   // register functions
   fregister("slave_getcode", fgetcode);
   fregister("slave_getsnr", fgetsnr);
   fregister("slave_work", fwork);
   }

void montecarlo::destroyfunctors(void)
   {
   delete fgetcode;
   delete fgetsnr;
   delete fwork;
   }

// overrideable user-interface functions

void montecarlo::display(const int pass, const double cur_accuracy, const double cur_mean)
   {
   static libbase::timer tupdate;
   if(tupdate.elapsed() > 0.5)
      {
      using std::clog;
      const int prec = clog.precision(3);
      clog << "Timer: " << t << ", ";
      if(isenabled())
         clog << getnumslaves() << " clients, " << getcputime()/t.elapsed() << "x speedup, ";
      else
         clog << "local, " << t.cputime()/t.elapsed() << "x usage, ";
      clog << "pass " << pass << ", "
         << "[" << cur_mean << " +/- " << cur_accuracy << "%] \r" << std::flush;
      clog.precision(prec);
      tupdate.start();
      }
   }

// constructor/destructor

montecarlo::montecarlo()
   {
   createfunctors();
   reset();
   // set default parameter settings
   set_confidence(0.95);
   set_accuracy(0.10);
   }

montecarlo::~montecarlo()
   {
   if(init)
      reset();
   delete system;
   destroyfunctors();
   }

// simulation initialization/finalization

void montecarlo::initialise(experiment *system)
   {
   init = true;
   // bind sub-components
   montecarlo::system = system;
   // reset the count of samples
   samplecount = 0;
   }

void montecarlo::reset()
   {
   init = false;
   system = NULL;
   }

// simulation parameters

void montecarlo::set_confidence(const double confidence)
   {
   assertalways(confidence > 0.5 && confidence < 1.0);
   trace << "DEBUG: setting confidence level of " << confidence << "\n";
   libbase::secant Qinv(libbase::Q);  // init Qinv as the inverse of Q(), using the secant method
   cfactor = Qinv((1.0-confidence)/2.0);
   }

void montecarlo::set_accuracy(const double accuracy)
   {
   assertalways(accuracy > 0 && accuracy < 1.0);
   trace << "DEBUG: setting accuracy level of " << accuracy << "\n";
   montecarlo::accuracy = accuracy;
   }

// main process

/*!
   \brief Compute a single sample and accumulate results
   \param[in,out] sum         Sum of results (to be updated)
   \param[in,out] sumsq       Sum of squares of results (to be updated)

   This function also updates the samplecount member variable.
*/
void montecarlo::sampleandaccumulate(vector<double>& sum, vector<double>& sumsq)
   {
   assert(sumsq.size() == sum.size());
   const int count = sum.size();
   vector<double> est(count);
   system->sample(est);
   samplecount++;
   accumulateresults(sum, sumsq, est);
   }

/*!
   \brief Accumulate running totals, given last sample
   \param[in,out] sum         Sum of results (to be updated)
   \param[in,out] sumsq       Sum of squares of results (to be updated)
   \param[in]     est         Last sample
*/
void montecarlo::accumulateresults(vector<double>& sum, vector<double>& sumsq, vector<double> est) const
   {
   assert(sum.size() == est.size());
   assert(sumsq.size() == est.size());
   // accumulate results
   sum += est;
   est.apply(square);
   sumsq += est;
   }

/*!
   \brief Update overall estimate, given last sample
   \param[in,out] result      Results to be updated
   \param[in,out] tolerance   Corresponding result accuracy to be updated
   \param[in]     sum         Sum of results
   \param[in]     sumsq       Sum of squares of results
   \return  Accuracy reached (worst accuracy over result set)
   
   \note If the accuracy cannot be computed yet (there has been no error event), then the
         accuracy reached takes the special largest-double value.
*/
double montecarlo::updateresults(vector<double>& result, vector<double>& tolerance, const vector<double>& sum, const vector<double>& sumsq) const
   {
   assert(result.size() > 0);
   assert(tolerance.size() == result.size());
   assert(sum.size() == result.size());
   assert(sumsq.size() == result.size());
   // for each result:
   double acc = 0;
   for(int i=0; i<result.size(); i++)
      {
      assert(samplecount > 0);
      // updated mean becomes the new result
      const double mean = sum(i)/double(samplecount);
      result(i) = mean;
      // compute tolerance only if this is meaningful
      if(mean > 0 && samplecount > 1)
         {
         const double sd = sqrt((sumsq(i)/double(samplecount) - mean*mean)/double(samplecount-1));
         tolerance(i) = cfactor*sd/mean;
         }
      else
         tolerance(i) = std::numeric_limits<double>::max();
      // track the worst tolerance reached
      if(tolerance(i) > acc)
         acc = tolerance(i);
      }
   return acc;
   }

/*!
   \brief Initialize given slave
   \param   s              Slave to be initialized
   \param   systemstring   Serialized system description

   Initialize given slave by sending the system being simulated and the
   current simulation parameter.
*/
void montecarlo::initslave(slave *s, std::string systemstring)
   {
   if(!call(s, "slave_getcode"))
      return;
   if(!send(s, systemstring))
      return;
   if(!call(s, "slave_getsnr"))
      return;
   if(!send(s, system->get_parameter()))
      return;
   trace << "DEBUG (estimate): Slave (" << s << ") initialized ok.\n";
   }

/*!
   \brief Initialize any new slaves
   \param   systemstring   Serialized system description

   If there are any slaves in the NEW state, initialize them by sending the system
   being simulated and the current simulation parameters.
*/
void montecarlo::initnewslaves(std::string systemstring)
   {
   while(slave *s = newslave())
      {
      trace << "DEBUG (estimate): New slave found (" << s << "), initializing.\n";
      initslave(s, systemstring);
      }
   }

/*!
   \brief Get idle slaves to work if we're not yet done
   \param   converged  True if results have already converged

   If there are any slaves in the IDLE state, ask them to start working. We ask *all* IDLE
   slaves to work, as long as the results have not yet converged. Therefore, this happens
   when the target accuracy is not yet reached or if the number of samples gathered is not
   yet enough. This necessarily causes extraneous results to be computed; these will then
   be discarded during the next turn. This method avoids the master hanging up waiting for
   results from slaves that will never come (happens if the machine is locked up but the
   TCP/IP stack is still running).
*/
void montecarlo::workidleslaves(bool converged)
   {
   for(slave *s; (!converged) && (s = idleslave()); )
      {
      trace << "DEBUG (estimate): Idle slave found (" << s << "), assigning work.\n";
      if(!call(s, "slave_work"))
         continue;
      trace << "DEBUG (estimate): Slave (" << s << ") work assigned ok.\n";
      }
   }

/*!
   \brief Read and accumulate results from any pending slaves
   \param[in,out] sum         Sum of results (to be updated)
   \param[in,out] sumsq       Sum of squares of results (to be updated)
   \return  True if any new results have been added, false otherwise

   If there are any slaves in the EVENT_PENDING state, read their results. Values
   returned are accumulated into the running totals.

   If any slave returns a result that does not correspond to the same system
   or parameter that are now being simulated, this is discarded and the slave
   is marked as 'new'.
*/
bool montecarlo::readpendingslaves(vector<double>& sum, vector<double>& sumsq)
   {
   assert(sumsq.size() == sum.size());
   const int count = sum.size();
   bool results_available = false;
   while(slave *s = pendingslave())
      {
      trace << "DEBUG (estimate): Pending event from slave (" << s << "), trying to read.\n";
      // get digest and parameter for simulated system
      std::string simdigest;
      double simparameter;
      if(!receive(s, simdigest) || !receive(s, simparameter))
         continue;
      // set up space for results that need to be returned
      vector<double> estsum(count);
      vector<double> estsumsq(count);
      int estsamplecount;
      // get results
      if(!receive(s, estsum) || !receive(s, estsumsq) || !receive(s, estsamplecount))
         continue;
      // check that results correspond to system under simulation
      if(sysdigest != sha(simdigest) || simparameter != system->get_parameter())
         {
         trace << "DEBUG (estimate): Slave returned invalid results (" << s << "), re-initializing.\n";
         resetslave(s);
         continue;
         }
      // accumulate
      samplecount += estsamplecount;
      sum += estsum;
      sumsq += estsumsq;
      // update usage information and return flag
      updatecputime(s);
      results_available = true;
      trace << "DEBUG (estimate): Read from slave (" << s << ") succeeded.\n";
      }
   return results_available;
   }

/*!
   \brief Simulate the system until convergence to given accuracy & confidence,
          and return estimated results
   \param[out] result      Vector of results
   \param[out] tolerance   Vector of corresponding result accuracy (at given confidence level)
*/
void montecarlo::estimate(vector<double>& result, vector<double>& tolerance)
   {
   t.start();

   // Initialise space for results
   const int count = system->count();
   assert(count > 0);
   result.init(count);
   tolerance.init(count);

   // Running values
   vector<double> sum(count);
   vector<double> sumsq(count);
   bool converged = false;
   // Initialise running values
   samplecount = 0;
   sum = 0;
   sumsq = 0;

   // Seed the experiment
   std::string systemstring;
   if(isenabled())
      {
      // create string representation of system
      std::ostringstream os;
      os << system;
      systemstring = os.str();
      // compute its digest
      std::istringstream is(systemstring);
      sysdigest.reset();
      sysdigest.process(is);
      // reset timer and all slaves to 'new' state
      resetslaves();
      resetcputime();
      }
   else
      system->seed(0);

   // Repeat the experiment until all the following are true:
   // 1) We have the accuracy we need
   // 2) We have enough samples for the accuracy to be meaningful
   // An interrupt from the user overrides everything...
   while(!converged)
      {
      bool results_available = false;
      // repeat the experiment
      if(isenabled())
         {
         // first initialize any new slaves
         initnewslaves(systemstring);
         // get idle slaves to work if we're not yet done
         workidleslaves(converged);
         // wait for results, but not indefinitely - this allows user to break
         trace << "DEBUG (estimate): Waiting for event.\n";
         waitforevent(true, 0.5);
         // accumulate results from any pending slaves
         results_available = readpendingslaves(sum, sumsq);
         }
      else
         {
         sampleandaccumulate(sum, sumsq);
         results_available = true;
         }
      // if we did get any results, update the statistics
      if(results_available)
         {
         double acc = updateresults(result, tolerance, sum, sumsq);
         // check if we have reached the required accuracy
         if(acc <= accuracy && samplecount >= min_samples)
            converged = true;
         // print something to inform the user of our progress
         display(samplecount, (acc<1 ? 100*acc : 99), result.min());
         }
      // consider our work done if the user has interrupted the processing
      // (note: this overrides everything)
      if(interrupt())
         break;
      }

   t.stop();
   }

}; // end namespace
