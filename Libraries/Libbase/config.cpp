/*!
 * \file
 * 
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 */

#include "config.h"

#ifdef WIN32
#  include <afx.h>
#  include <conio.h>
#else
#  include <unistd.h>
#  include <termios.h>
#  include <sys/ioctl.h>
#  include <errno.h>
#  include <string.h>
#endif
#include <signal.h>
#include <stdlib.h>
#include <sstream>

namespace libbase {

// Debugging tools

void reporterrorandfail(const char *expression, const char *file, int line)
   {
   std::cerr << "ERROR (" << file << " line " << line << "): assertion "
         << expression << " failed.\n";
   exit(1);
   }

class tracestreambuf : public std::streambuf {
protected:
   std::string buffer;
public:
   tracestreambuf()
      {
      buffer = "";
      }
   virtual ~tracestreambuf()
      {
      }
   int underflow()
      {
      return EOF;
      }
   int overflow(int c = EOF);
};

inline int tracestreambuf::overflow(int c)
   {
#ifndef NDEBUG
   if (c == '\r' || c == '\n')
      {
      if (!buffer.empty())
         {
#ifdef WIN32
         TRACE("%s\n", buffer.c_str());
#else
         std::clog << buffer.c_str() << std::endl << std::flush;
#endif
         buffer = "";
         }
      }
#ifdef WIN32
   // handle TRACE limit in Windows (512 chars including NULL)

   else if(buffer.length() == 511)
      {
      TRACE("%s", buffer.c_str());
      buffer = c;
      }
#endif
   else
      buffer += c;
#endif
   return 1;
   }
;

tracestreambuf g_tracebuf;
std::ostream trace(&g_tracebuf);

// Constants

const double PI = 3.14159265358979323846;

#ifdef WIN32
const char DIR_SEPARATOR = '\\';
#else
const char DIR_SEPARATOR = '/';
#endif

// Checks if a key has been pressed and returns true if this has happened.
int keypressed(void)
   {
#ifdef WIN32
   return _kbhit();
#else
   int count = 0;
   int error;
   struct timespec tv;
   struct termios otty, ntty;

   tcgetattr(STDIN_FILENO, &otty);
   ntty = otty;
   ntty.c_lflag &= ~ICANON; /* raw mode */

   if (0 == (error = tcsetattr(STDIN_FILENO, TCSANOW, &ntty)))
      {
      error += ioctl(STDIN_FILENO, FIONREAD, &count);
      error += tcsetattr(STDIN_FILENO, TCSANOW, &otty);
      // minimal delay gives up cpu time slice, allows use in a tight loop
      tv.tv_sec = 0;
      tv.tv_nsec = 10;
      nanosleep(&tv, NULL);
      }

   return error == 0 ? count : -1;
#endif
   }

// Waits for the user to hit a key and returns its value.
// The user's response is not shown on screen.
int readkey(void)
   {
#ifdef WIN32
   return _getch();
#else
   unsigned char ch;
   int error;
   struct termios otty, ntty;

   fflush(stdout);
   tcgetattr(STDIN_FILENO, &otty);
   ntty = otty;

   ntty.c_lflag &= ~ICANON; /* line settings   */

   /* disable echoing the char as it is typed */
   ntty.c_lflag &= ~ECHO; /* disable echo        */

   ntty.c_cc[VMIN] = 1; /* block for input  */
   ntty.c_cc[VTIME] = 0; /* timer is ignored */

   // flush the input buffer before blocking for new input
   //#define FLAG TCSAFLUSH
   // return a char from the current input buffer, or block if no input is waiting.
#define FLAG TCSANOW

   if (0 == (error = tcsetattr(STDIN_FILENO, FLAG, &ntty)))
      {
      /* get a single character from stdin */
      error = read(STDIN_FILENO, &ch, 1);
      /* restore old settings */
      error += tcsetattr(STDIN_FILENO, FLAG, &otty);
      }

   return (error == 1 ? (int) ch : -1);
#endif
   }

// Interrupt-signal handling function

static bool interrupt_caught = false;

static void catch_signal(int sig_num)
   {
   trace << "DEBUG (catch_signal): caught signal " << sig_num << "\n.";
   // re-set the signal handler again for next time
   signal(sig_num, catch_signal);
   // update variables accordingly
   if (sig_num == SIGINT)
      {
      std::cerr << "Caught interrupt...\n";
      interrupt_caught = true;
      }
   }

bool interrupted(void)
   {
   // set the signal handler
   signal(SIGINT, catch_signal);
   // return pre-set variable
   return interrupt_caught;
   }

// System error message reporting

std::string getlasterror()
   {
   std::ostringstream sout;
#ifdef WIN32
   TCHAR buf[80];
   DWORD code = GetLastError();
   FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, NULL, buf, 80, NULL);
   sout << buf << " (" << std::hex << code << std::dec << ")";
#else
   sout << strerror(errno) << " (" << std::hex << errno << std::dec << ")";
#endif
   return sout.str();
   }

//! Function to skip over whitespace

std::istream& eatwhite(std::istream& is)
   {
   char c;
   while (is.get(c))
      {
      if (!isspace(c))
         {
         is.putback(c);
         break;
         }
      }
   return is;
   }

//! Function to skip over comments (and preceding whitespace)

std::istream& eatcomments(std::istream& is)
   {
   char c;
   while (is.get(c))
      {
      if (c == '#')
         {
         std::string s;
         getline(is, s);
         }
      else if (!isspace(c))
         {
         is.putback(c);
         break;
         }
      }
   return is;
   }

/*!
 * \brief Check for a failure during the last stream input.
 * \return True if the last stream input did not succeed.
 * 
 * If the last stream input did not succeed, an error message is also shown,
 * detailing the stream position where this occurred.
 */
bool isfailedload(std::istream &is)
   {
   if (is.fail())
      {
      std::ios::iostate state = is.rdstate();
      is.clear();
      std::cerr << "ERROR: Failure loading object at position " << is.tellg()
            << ".\n";
      is.clear(state);
      return true;
      }
   return false;
   }

/*!
 * \brief Check for a unloaded data on the stream.
 * \return True if there is still data left on the stream.
 * 
 * If there is still data left on the stream an error message is also shown,
 * detailing the stream position where this occurred. All data left from this
 * position onwards is also printed. Note that comments are excluded.
 */
bool isincompleteload(std::istream &is)
   {
   libbase::eatcomments(is);
   if (!is.eof())
      {
      std::cerr << "ERROR: Incomplete loading, stopped at position "
            << is.tellg() << ".\n";
      std::string s;
      while (getline(is, s))
         std::cerr << s << "\n";
      return true;
      }
   return false;
   }

/*!
 * \brief Verify that all stream data was read without error.
 * 
 * If the last stream input failed, or if there is still data left on the
 * stream, an error message is shown, and the program is stopped.
 */
void verifycompleteload(std::istream& is)
   {
   if (isfailedload(is) || isincompleteload(is))
      exit(1);
   }

} // end namespace
