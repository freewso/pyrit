/*
#
#    Copyright 2008, Lukas Lueg, knabberknusperhaus@yahoo.de
#    Copyright 2009, Benedikt Heinz, Zn000h@googlemail.com
#
#    This file is part of Pyrit.
#
#    Pyrit is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Pyrit is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Pyrit.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "brook/cpyrit.h"
#include "cpyrit.h"

extern "C" PyObject *
cpyrit_stream (PyObject * self, PyObject * args)
{
  char *essid_pre;
  char essid[33 + 4];
  unsigned char temp[32], pad[64];
  PyObject *listObj;
  int numLines, line, slen, i;
  SHA_CTX ctx_pad;
  uint *dbuf, dim;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure ();

  if (!PyArg_ParseTuple (args, "sO!", &essid_pre, &PyList_Type, &listObj))
    return NULL;
  numLines = PyList_Size (listObj);
  if ((numLines < 0) || (numLines > 8192))
    return NULL;

  dbuf = (uint *) malloc (8192 * 2 * 4 * (5 * 3));
  if (dbuf == NULL)
    {
      PyGILState_Release (gstate);
      return PyErr_NoMemory ();
    }

  memset (essid, 0, sizeof (essid));
  slen = strlen (essid_pre);
  slen = slen <= 32 ? slen : 32;
  memcpy (essid, essid_pre, slen);
  slen = strlen (essid) + 4;

  for (line = 0; line < numLines; line++)
    {
      char *key = PyString_AsString (PyList_GetItem (listObj, line));

      strncpy ((char *) pad, key, sizeof (pad));
      for (i = 0; i < sizeof (pad); i++)
	pad[i] ^= 0x36;
      SHA1_Init (&ctx_pad);
      SHA1_Update (&ctx_pad, pad, sizeof (pad));

      dbuf[(8192 * 2 * 0) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 0) + (line * 2)] = ctx_pad.h0;
      dbuf[(8192 * 2 * 1) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 1) + (line * 2)] = ctx_pad.h1;
      dbuf[(8192 * 2 * 2) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 2) + (line * 2)] = ctx_pad.h2;
      dbuf[(8192 * 2 * 3) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 3) + (line * 2)] = ctx_pad.h3;
      dbuf[(8192 * 2 * 4) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 4) + (line * 2)] = ctx_pad.h4;

      for (i = 0; i < sizeof (pad); i++)
	pad[i] ^= (0x36 ^ 0x5c);
      SHA1_Init (&ctx_pad);
      SHA1_Update (&ctx_pad, pad, sizeof (pad));

      dbuf[(8192 * 2 * 5) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 5) + (line * 2)] = ctx_pad.h0;
      dbuf[(8192 * 2 * 6) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 6) + (line * 2)] = ctx_pad.h1;
      dbuf[(8192 * 2 * 7) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 7) + (line * 2)] = ctx_pad.h2;
      dbuf[(8192 * 2 * 8) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 8) + (line * 2)] = ctx_pad.h3;
      dbuf[(8192 * 2 * 9) + (line * 2) + 1] =
	dbuf[(8192 * 2 * 9) + (line * 2)] = ctx_pad.h4;

      essid[slen - 1] = '\1';
      HMAC (EVP_sha1 (), (unsigned char *) key, strlen (key),
	    (unsigned char *) essid, slen, (unsigned char *) &ctx_pad, NULL);
      dbuf[(8192 * 2 * 10) + (line * 2)] = ctx_pad.h0;
      dbuf[(8192 * 2 * 11) + (line * 2)] = ctx_pad.h1;
      dbuf[(8192 * 2 * 12) + (line * 2)] = ctx_pad.h2;
      dbuf[(8192 * 2 * 13) + (line * 2)] = ctx_pad.h3;
      dbuf[(8192 * 2 * 14) + (line * 2)] = ctx_pad.h4;

      essid[slen - 1] = '\2';
      HMAC (EVP_sha1 (), (unsigned char *) key, strlen (key),
	    (unsigned char *) essid, slen, (unsigned char *) &ctx_pad, NULL);
      dbuf[(8192 * 2 * 10) + (line * 2) + 1] = ctx_pad.h0;
      dbuf[(8192 * 2 * 11) + (line * 2) + 1] = ctx_pad.h1;
      dbuf[(8192 * 2 * 12) + (line * 2) + 1] = ctx_pad.h2;
      dbuf[(8192 * 2 * 13) + (line * 2) + 1] = ctx_pad.h3;
      dbuf[(8192 * 2 * 14) + (line * 2) + 1] = ctx_pad.h4;
    }

  // We promise not to touch python objects beyond this point 
  Py_BEGIN_ALLOW_THREADS;

  dim = numLines;

  ::brook::Stream < uint2 > ipad_A (1, &dim);
  ::brook::Stream < uint2 > ipad_B (1, &dim);
  ::brook::Stream < uint2 > ipad_C (1, &dim);
  ::brook::Stream < uint2 > ipad_D (1, &dim);
  ::brook::Stream < uint2 > ipad_E (1, &dim);

  ::brook::Stream < uint2 > opad_A (1, &dim);
  ::brook::Stream < uint2 > opad_B (1, &dim);
  ::brook::Stream < uint2 > opad_C (1, &dim);
  ::brook::Stream < uint2 > opad_D (1, &dim);
  ::brook::Stream < uint2 > opad_E (1, &dim);

  ::brook::Stream < uint2 > pmk_in0 (1, &dim);
  ::brook::Stream < uint2 > pmk_in1 (1, &dim);
  ::brook::Stream < uint2 > pmk_in2 (1, &dim);
  ::brook::Stream < uint2 > pmk_in3 (1, &dim);
  ::brook::Stream < uint2 > pmk_in4 (1, &dim);

  ::brook::Stream < uint2 > pmk_out0 (1, &dim);
  ::brook::Stream < uint2 > pmk_out1 (1, &dim);
  ::brook::Stream < uint2 > pmk_out2 (1, &dim);
  ::brook::Stream < uint2 > pmk_out3 (1, &dim);
  ::brook::Stream < uint2 > pmk_out4 (1, &dim);

  ipad_A.read (dbuf + (8192 * 2 * 0));
  ipad_B.read (dbuf + (8192 * 2 * 1));
  ipad_C.read (dbuf + (8192 * 2 * 2));
  ipad_D.read (dbuf + (8192 * 2 * 3));
  ipad_E.read (dbuf + (8192 * 2 * 4));

  opad_A.read (dbuf + (8192 * 2 * 5));
  opad_B.read (dbuf + (8192 * 2 * 6));
  opad_C.read (dbuf + (8192 * 2 * 7));
  opad_D.read (dbuf + (8192 * 2 * 8));
  opad_E.read (dbuf + (8192 * 2 * 9));

  pmk_in0.read (dbuf + (8192 * 2 * 10));
  pmk_in1.read (dbuf + (8192 * 2 * 11));
  pmk_in2.read (dbuf + (8192 * 2 * 12));
  pmk_in3.read (dbuf + (8192 * 2 * 13));
  pmk_in4.read (dbuf + (8192 * 2 * 14));

  sha1_rounds (ipad_A, ipad_B, ipad_C, ipad_D, ipad_E, opad_A, opad_B, opad_C,
	       opad_D, opad_E, pmk_in0, pmk_in1, pmk_in2, pmk_in3, pmk_in4,
	       uint2 (0x80000000, 0x80000000), pmk_out0, pmk_out1, pmk_out2,
	       pmk_out3, pmk_out4);

  pmk_out0.write (dbuf + (8192 * 2 * 0));
  pmk_out1.write (dbuf + (8192 * 2 * 1));
  pmk_out2.write (dbuf + (8192 * 2 * 2));
  pmk_out3.write (dbuf + (8192 * 2 * 3));
  pmk_out4.write (dbuf + (8192 * 2 * 4));

  i = 0;
  if ((pmk_out0.error () !=::brook::BR_NO_ERROR)
      || (pmk_out1.error () !=::brook::BR_NO_ERROR)
      || (pmk_out2.error () !=::brook::BR_NO_ERROR)
      || (pmk_out3.error () !=::brook::BR_NO_ERROR)
      || (pmk_out4.error () !=::brook::BR_NO_ERROR))
    i = -1;

  Py_END_ALLOW_THREADS;

  if (i)
    return NULL;

  PyObject *destlist = PyList_New (numLines);
  for (i = 0; i < numLines * 2; i++)
    {
      unsigned int *ptr = (unsigned int *) temp;
      ptr[0] = dbuf[i];
      ptr[1] = dbuf[(8192 * 2) + i];
      ptr[2] = dbuf[(2 * 8192 * 2) + i];
      ptr[3] = dbuf[(3 * 8192 * 2) + i];
      ptr[4] = dbuf[(4 * 8192 * 2) + i];
      i++;
      ptr[5] = dbuf[i];
      ptr[6] = dbuf[(8192 * 2) + i];
      ptr[7] = dbuf[(2 * 8192 * 2) + i];
      PyList_SetItem (destlist, i / 2,
		      Py_BuildValue ("(s,s#)",
				     PyString_AsString (PyList_GetItem
							(listObj, i / 2)),
				     temp, 32));
    }

  free (dbuf);

  PyGILState_Release (gstate);
  return destlist;
}
