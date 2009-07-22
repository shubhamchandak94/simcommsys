/*!
 \file

 \section svn Version Control
 - $Revision$
 - $Date$
 - $Author$
 */

#include "crypt.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>

namespace libcomm {

//////////////////////////////////////////////////////////////////////
// Const values
//////////////////////////////////////////////////////////////////////

const int crypt::BS = 64;
const int crypt::BS2 = 32;
const int crypt::KS = 48;
const int crypt::KS2 = 24;
const int crypt::IS = 56;
const int crypt::IS2 = 28;

const char crypt::PC1[] = {56, 48, 40, 32, 24, 16, 8, 0, 57, 49, 41, 33, 25,
      17, 9, 1, 58, 50, 42, 34, 26, 18, 10, 2, 59, 51, 43, 35, 62, 54, 46, 38,
      30, 22, 14, 6, 61, 53, 45, 37, 29, 21, 13, 5, 60, 52, 44, 36, 28, 20, 12,
      4, 27, 19, 11, 3};

const char crypt::PC2[] = {13, 16, 10, 23, 0, 4, 2, 27, 14, 5, 20, 9, 22, 18,
      11, 3, 25, 7, 15, 6, 26, 19, 12, 1, 40, 51, 30, 36, 46, 54, 29, 39, 50,
      44, 32, 47, 43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31};

const char crypt::IP[] = {57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19,
      11, 3, 61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7, 56,
      48, 40, 32, 24, 16, 8, 0, 58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36,
      28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6};

const char crypt::EP[] = {7, 39, 15, 47, 23, 55, 31, 63, 6, 38, 14, 46, 22, 54,
      30, 62, 5, 37, 13, 45, 21, 53, 29, 61, 4, 36, 12, 44, 20, 52, 28, 60, 3,
      35, 11, 43, 19, 51, 27, 59, 2, 34, 10, 42, 18, 50, 26, 58, 1, 33, 9, 41,
      17, 49, 25, 57, 0, 32, 8, 40, 16, 48, 24, 56};

const char crypt::E0[] = {31, 0, 1, 2, 3, 4, 3, 4, 5, 6, 7, 8, 7, 8, 9, 10, 11,
      12, 11, 12, 13, 14, 15, 16, 15, 16, 17, 18, 19, 20, 19, 20, 21, 22, 23,
      24, 23, 24, 25, 26, 27, 28, 27, 28, 29, 30, 31, 0};

const char crypt::PERM[] = {15, 6, 19, 20, 28, 11, 27, 16, 0, 14, 22, 25, 4,
      17, 30, 9, 1, 7, 23, 13, 31, 26, 2, 8, 18, 12, 29, 5, 21, 10, 3, 24};

const char crypt::S_BOX[][64] = {{14, 0, 4, 15, 13, 7, 1, 4, 2, 14, 15, 2, 11,
      13, 8, 1, 3, 10, 10, 6, 6, 12, 12, 11, 5, 9, 9, 5, 0, 3, 7, 8, 4, 15, 1,
      12, 14, 8, 8, 2, 13, 4, 6, 9, 2, 1, 11, 7, 15, 5, 12, 11, 9, 3, 7, 14, 3,
      10, 10, 0, 5, 6, 0, 13}, {15, 3, 1, 13, 8, 4, 14, 7, 6, 15, 11, 2, 3, 8,
      4, 14, 9, 12, 7, 0, 2, 1, 13, 10, 12, 6, 0, 9, 5, 11, 10, 5, 0, 13, 14,
      8, 7, 10, 11, 1, 10, 3, 4, 15, 13, 4, 1, 2, 5, 11, 8, 6, 12, 7, 6, 12, 9,
      0, 3, 5, 2, 14, 15, 9}, {10, 13, 0, 7, 9, 0, 14, 9, 6, 3, 3, 4, 15, 6, 5,
      10, 1, 2, 13, 8, 12, 5, 7, 14, 11, 12, 4, 11, 2, 15, 8, 1, 13, 1, 6, 10,
      4, 13, 9, 0, 8, 6, 15, 9, 3, 8, 0, 7, 11, 4, 1, 15, 2, 14, 12, 3, 5, 11,
      10, 5, 14, 2, 7, 12}, {7, 13, 13, 8, 14, 11, 3, 5, 0, 6, 6, 15, 9, 0, 10,
      3, 1, 4, 2, 7, 8, 2, 5, 12, 11, 1, 12, 10, 4, 14, 15, 9, 10, 3, 6, 15, 9,
      0, 0, 6, 12, 10, 11, 1, 7, 13, 13, 8, 15, 9, 1, 4, 3, 5, 14, 11, 5, 12,
      2, 7, 8, 2, 4, 14}, {2, 14, 12, 11, 4, 2, 1, 12, 7, 4, 10, 7, 11, 13, 6,
      1, 8, 5, 5, 0, 3, 15, 15, 10, 13, 3, 0, 9, 14, 8, 9, 6, 4, 11, 2, 8, 1,
      12, 11, 7, 10, 1, 13, 14, 7, 2, 8, 13, 15, 6, 9, 15, 12, 0, 5, 9, 6, 10,
      3, 4, 0, 5, 14, 3}, {12, 10, 1, 15, 10, 4, 15, 2, 9, 7, 2, 12, 6, 9, 8,
      5, 0, 6, 13, 1, 3, 13, 4, 14, 14, 0, 7, 11, 5, 3, 11, 8, 9, 4, 14, 3, 15,
      2, 5, 12, 2, 9, 8, 5, 12, 15, 3, 10, 7, 11, 0, 14, 4, 1, 10, 7, 1, 6, 13,
      0, 11, 8, 6, 13}, {4, 13, 11, 0, 2, 11, 14, 7, 15, 4, 0, 9, 8, 1, 13, 10,
      3, 14, 12, 3, 9, 5, 7, 12, 5, 2, 10, 15, 6, 8, 1, 6, 1, 6, 4, 11, 11, 13,
      13, 8, 12, 1, 3, 4, 7, 10, 14, 7, 10, 9, 15, 5, 6, 0, 8, 15, 0, 14, 5, 2,
      9, 3, 2, 12}, {13, 1, 2, 15, 8, 13, 4, 8, 6, 10, 15, 3, 11, 7, 1, 4, 10,
      12, 9, 5, 3, 6, 14, 11, 5, 0, 0, 14, 12, 9, 7, 2, 7, 2, 11, 1, 4, 14, 1,
      7, 9, 4, 12, 10, 14, 8, 2, 13, 0, 15, 6, 12, 10, 9, 13, 0, 15, 3, 3, 5,
      5, 6, 8, 11}};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

crypt::crypt()
   {
   }

crypt::~crypt()
   {
   }

//////////////////////////////////////////////////////////////////////
// Private functions
//////////////////////////////////////////////////////////////////////

void crypt::perm(char *a, const char *e, const char *pc, int n)
   {
   for (; n--; pc++, a++)
      *a = e[int(*pc)];
   }

void crypt::crypt_main(char *nachr_l, char *nachr_r, char *schl)
   {
   char tmp[KS];
   int sbval;
   char *tp = tmp;
   const char *e = E;
   int i, j;

   for (i = 0; i < 8; i++)
      {
      for (j = 0, sbval = 0; j < 6; j++)
         sbval = (sbval << 1) | (nachr_r[int(*e++)] ^ *schl++);
      sbval = S_BOX[i][sbval];
      for (tp += 4, j = 4; j--; sbval >>= 1)
         *--tp = sbval & 1;
      tp += 4;
      }

   e = PERM;
   for (i = 0; i < BS2; i++)
      *nachr_l++ ^= tmp[int(*e++)];
   }

//////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////

void crypt::encrypt(char *nachr, int decr)
   {
   char(*schl)[KS] = decr ? schluessel + 15 : schluessel;
   char tmp[BS];
   int i;

   perm(tmp, nachr, IP, BS);

   for (i = 8; i--;)
      {
      crypt_main(tmp, tmp + BS2, *schl);
      if (decr)
         schl--;
      else
         schl++;
      crypt_main(tmp + BS2, tmp, *schl);
      if (decr)
         schl--;
      else
         schl++;
      }

   perm(nachr, tmp, EP, BS);
   }

void crypt::setkey(char *schl)
   {
   char tmp1[IS];
   unsigned int ls = 0x7efc;
   int i, j, k;
   int shval = 0;
   char *akt_schl;

   memcpy(E, E0, KS);
   perm(tmp1, schl, PC1, IS);

   for (i = 0; i < 16; i++)
      {
      shval += 1 + (ls & 1);
      akt_schl = schluessel[i];
      for (j = 0; j < KS; j++)
         {
         if ((k = PC2[j]) >= IS2)
            {
            if ((k += shval) >= IS)
               k = (k - IS2) % IS2 + IS2;
            }
         else if ((k += shval) >= IS2)
            k %= IS2;
         *akt_schl++ = tmp1[k];
         }
      ls >>= 1;
      }
   }

char *crypt::crypttext(const char *wort, const char *salt)
   {
   // temporary variables
   char *k;
   int tmp, keybyte;
   int i, j;

   // declare and initialize key
   char key[BS + 2];
   memset(key, 0, BS + 2);

   for (k = key, i = 0; i < BS; i++)
      {
      // get next letter & quit if the end has been reached
      if (!(keybyte = *wort++))
         break;
      k += 7;
      for (j = 0; j < 7; j++, i++)
         {
         *--k = keybyte & 1;
         keybyte >>= 1;
         }
      k += 8;
      }

   setkey(key);
   memset(key, 0, BS + 2);

   static char retkey[14];
   for (k = E, i = 0; i < 2; i++)
      {
      retkey[i] = keybyte = *salt++;
      if (keybyte > 'Z')
         keybyte -= 'a' - 'Z' - 1;
      if (keybyte > '9')
         keybyte -= 'A' - '9' - 1;
      keybyte -= '.';

      for (j = 0; j < 6; j++, keybyte >>= 1, k++)
         {
         if (!(keybyte & 1))
            continue;
         tmp = *k;
         *k = k[24];
         k[24] = tmp;
         }
      }

   for (i = 0; i < 25; i++)
      encrypt(key, 0);

   for (k = key, i = 0; i < 11; i++)
      {
      for (j = keybyte = 0; j < 6; j++)
         {
         keybyte <<= 1;
         keybyte |= *k++;
         }

      keybyte += '.';
      if (keybyte > '9')
         keybyte += 'A' - '9' - 1;
      if (keybyte > 'Z')
         keybyte += 'a' - 'Z' - 1;
      retkey[i + 2] = keybyte;
      }

   retkey[i + 2] = 0;

   if (!retkey[1])
      retkey[1] = *retkey;

   return retkey;
   }

} // end namespace