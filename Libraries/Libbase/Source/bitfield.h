#ifndef __bitfield_h
#define __bitfield_h

#include "config.h"

#include <iostream>
#include <string>

namespace libbase {

/*!
   \brief   Bitfield.
   \author  Johann Briffa

   \section svn Version Control
   - $Revision$
   - $Date$
   - $Author$

   \version 1.01 (27 Nov 2001)
  made setdefsize a static member (this should have been so anyway) and also
  check_fieldsize since this is called from it. Also made check_range a const member.

   \version 1.10 (28 Feb 2002)
  moved most functions to the implementation file rather than inline here. Also, made
  the extraction functions const, and added a stream input function.

   \version 1.11 (6 Mar 2002)
  changed vcs version variable from a global to a static class variable.
  also changed use of iostream from global to std namespace.
  also had to change the concatenation operator from ',' to '+' since this was
  conflicting with something in "iostream"

   \version 1.12 (13 Mar 2002)
  changed the stream input function to get the next word from the stream (as a string) and
  then use the same code as the constructor to change that into a bitfield. This code
  is now moved into a private init() function. Also, the init function has been changed
  to issue a warning if the string contains any invalid characters.

   \version 1.20 (23 Mar 2002)
  added operator to convert bitfield to a string. Also updated the stream output function
  to use this convertor to display a bitfield.

   \version 1.30 (26 Oct 2006)
   - defined class and associated data within "libbase" namespace.
   - removed use of "using namespace std", replacing by tighter "using" statements as needed.

   \version 1.31 (5 Dec 2007)
   - added constructor to directly convert an integer at a specified width
*/

class bitfield {
   static int   defsize;   // default size
   // member variables
   int32u field;           // bit field
   int    bits;            // number of bits
private:
   int32u mask() const;
   void check_range(int32u f) const;
   static void check_fieldsize(int b);
   void init(const char *s);
public:
   bitfield();
   bitfield(const char *s) { init(s); };
   bitfield(const int32u field, const int bits);

   // Type conversion to integer/string
   operator int32u() const { return field; };
   operator std::string() const;

   // Field size methods
   int size() const { return bits; };
   void resize(const int b);
   static void setdefsize(const int b);

   // Copy and Assignment
   bitfield& operator=(const bitfield& x);
   bitfield& operator=(const int32u x);

   // Partial extraction and indexed access
   bitfield extract(const int hi, const int lo) const;
   bitfield extract(const int b) const;
   bitfield operator[](const int b) const { return extract(b); };

   // Logical operators - OR, AND, XOR
   friend bitfield operator|(const bitfield& a, const bitfield& b);
   friend bitfield operator&(const bitfield& a, const bitfield& b);
   friend bitfield operator^(const bitfield& a, const bitfield& b);
   bitfield& operator|=(const bitfield& x);
   bitfield& operator&=(const bitfield& x);
   bitfield& operator^=(const bitfield& x);

   // Convolution operator
   friend bitfield operator*(const bitfield& a, const bitfield& b);
   // Concatenation operator
   friend bitfield operator+(const bitfield& a, const bitfield& b);
   // Shift-register operators - sequence shift-in
   friend bitfield operator<<(const bitfield& a, const bitfield& b);
   friend bitfield operator>>(const bitfield& a, const bitfield& b);
   // Shift-register operators - zero shift-in
   friend bitfield operator<<(const bitfield& a, const int b);
   friend bitfield operator>>(const bitfield& a, const int b);
   bitfield& operator<<=(const int x);
   bitfield& operator>>=(const int x);

   // Stream I/O
   friend std::ostream& operator<<(std::ostream& s, const bitfield& b);
   friend std::istream& operator>>(std::istream& s, bitfield& b);
};

}; // end namespace

#endif
