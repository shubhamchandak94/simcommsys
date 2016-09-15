/*!
 * \file
 *
 * Copyright (c) 2010 Johann A. Briffa
 *
 * This file is part of SimCommSys.
 *
 * SimCommSys is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimCommSys is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimCommSys.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __masterslave_h
#define __masterslave_h

#include "config.h"
#include "vector.h"
#include "socket.h"
#include "walltimer.h"
#include "cputimer.h"
#include "functor.h"
#include <map>

#include <boost/shared_ptr.hpp>

namespace libbase {

/*!
 * \brief   Socket-based Master-Slave computation.
 * \author  Johann Briffa
 *
 * Class supporting socket-based master-slave relationship.
 * - supports dynamic slave list
 *
 * \note RPC calls are now done by passing a string reference, which is used
 * as a key in a map list. Two new functions support this:
 * - fregister() allows registering of functions by derived classes, and
 * - fcall() to actually call them
 * Since this class cannot know the exact type of the function pointers,
 * these are held by functors.
 *
 * \todo Serialize to network byte order always.
 *
 * \todo Consider modifying cmpi to support this class interface model, and
 * create a new abstract class to encapsulate both models.
 *
 * \todo Make setting priority effective on Windows
 *
 * \todo Split master and slave classes
 */

class masterslave {
private:
   // Internally-used constants (tags)
   typedef enum {
      GETNAME = 0xFA, GETCPUTIME, WORK = 0xFE, DIE
   } tag_t;

   // communication objects
public:
   // slave state
   typedef enum {
      NEW = 0, EVENT_PENDING, IDLE, WORKING
   } state_t;
   // operating mode - returned by enable()
   typedef enum {
      mode_local = 0, mode_master, mode_slave
   } mode_t;

   // items for use by everyone (?)
private:
   bool initialized;
   double cputimeused;
   walltimer twall;
   cputimer tcpu;
public:
   // global enable of cluster system
   mode_t enable(const std::string& endpoint, bool quiet = false, int priority =
         10);
   // informative functions
   bool isenabled() const
      {
      return initialized;
      }
   double getcputime() const
      {
      return initialized ? cputimeused : tcpu.elapsed();
      }
   double getwalltime() const
      {
      return twall.elapsed();
      }
   double getusage() const
      {
      return getcputime() / getwalltime();
      }

   // items for use by slaves
private:
   std::map<std::string, boost::shared_ptr<functor> > fmap;
   boost::shared_ptr<socket> master;
   // helper functions
   void close();
   void setpriority(const int priority);
   void connect(const std::string& hostname, const int16u port);
   std::string gethostname();
   int gettag();
   void sendname();
   void sendcputime();
   void dowork();
   void slaveprocess(const std::string& hostname, const int16u port,
         const int priority);
public:
   // RPC function calls
   //! Register a RPC function
   void fregister(const std::string& name, boost::shared_ptr<functor> f);
   //! Call a RPC function
   void fcall(const std::string& name);
   // slave -> master communication
   bool send(const void *buf, const size_t len);
   bool send(const int x)
      {
      return send(&x, sizeof(x));
      }
   bool send(const int64u x)
      {
      return send(&x, sizeof(x));
      }
   bool send(const double x)
      {
      return send(&x, sizeof(x));
      }
   bool send(const vector<double>& x);
   bool send(const std::string& x);
   bool receive(void *buf, const size_t len);
   bool receive(int& x)
      {
      return receive(&x, sizeof(x));
      }
   bool receive(int64u& x)
      {
      return receive(&x, sizeof(x));
      }
   bool receive(double& x)
      {
      return receive(&x, sizeof(x));
      }
   bool receive(std::string& x);

   // items for use by master
private:
   std::map<boost::shared_ptr<socket>, state_t> smap;
   // helper functions
   void close(boost::shared_ptr<socket> s);
public:
   // creation and destruction
   masterslave();
   ~masterslave();
   // disable process
   void disable();
   // slave-interface functions
   boost::shared_ptr<socket> find_new_slave();
   boost::shared_ptr<socket> find_idle_slave();
   boost::shared_ptr<socket> find_pending_slave();
   int count_workingslaves() const;
   bool anyoneworking() const;
   void waitforevent(const bool acceptnew = true, const double timeout = 0);
   void resetslave(boost::shared_ptr<socket> s);
   void resetslaves();
   // informative functions
   size_t getnumslaves() const
      {
      return smap.size();
      }
   // master -> slave communication
   bool send(boost::shared_ptr<socket> s, const void *buf, const size_t len);
   bool send(boost::shared_ptr<socket> s, const int x)
      {
      return send(s, &x, sizeof(x));
      }
   bool send(boost::shared_ptr<socket> s, const double x)
      {
      return send(s, &x, sizeof(x));
      }
   bool send(boost::shared_ptr<socket> s, const std::string& x);
   bool call(boost::shared_ptr<socket> s, const std::string& x)
      {
      return send(s, int(WORK)) && send(s, x);
      }
   //! Reset CPU usage accumulation
   void resetcputime()
      {
      cputimeused = 0;
      }
   bool updatecputime(boost::shared_ptr<socket> s);
   bool receive(boost::shared_ptr<socket> s, void *buf, const size_t len);
   bool receive(boost::shared_ptr<socket> s, int& x)
      {
      return receive(s, &x, sizeof(x));
      }
   bool receive(boost::shared_ptr<socket> s, libbase::int64u& x)
      {
      return receive(s, &x, sizeof(x));
      }
   bool receive(boost::shared_ptr<socket> s, double& x)
      {
      return receive(s, &x, sizeof(x));
      }
   bool receive(boost::shared_ptr<socket> s, vector<double>& x);
   bool receive(boost::shared_ptr<socket> s, std::string& x);
};

} // end namespace

#endif
