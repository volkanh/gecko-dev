/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_ipc_DaemonSocketPDUHelpers_h
#define mozilla_ipc_DaemonSocketPDUHelpers_h

#include <stdint.h>
#include "mozilla/ipc/DaemonSocketPDU.h"
#include "nsString.h"

namespace mozilla {
namespace ipc {

struct DaemonSocketPDUHeader {
  DaemonSocketPDUHeader()
    : mService(0x00)
    , mOpcode(0x00)
    , mLength(0x00)
  { }

  DaemonSocketPDUHeader(uint8_t aService, uint8_t aOpcode, uint16_t aLength)
    : mService(aService)
    , mOpcode(aOpcode)
    , mLength(aLength)
  { }

  uint8_t mService;
  uint8_t mOpcode;
  uint16_t mLength;
};

namespace DaemonSocketPDUHelpers {

//
// Conversion
//
// PDUs can only store primitive data types, such as integers or
// byte arrays. Gecko often uses more complex data types, such as
// enumators or stuctures. Conversion functions convert between
// primitive data and internal Gecko's data types during a PDU's
// packing and unpacking.
//

nsresult
Convert(bool aIn, uint8_t& aOut);

nsresult
Convert(bool aIn, int32_t& aOut);

nsresult
Convert(int aIn, uint8_t& aOut);

nsresult
Convert(int aIn, int16_t& aOut);

nsresult
Convert(int aIn, int32_t& aOut);

nsresult
Convert(uint8_t aIn, bool& aOut);

nsresult
Convert(uint8_t aIn, char& aOut);

nsresult
Convert(uint8_t aIn, int& aOut);

nsresult
Convert(uint8_t aIn, unsigned long& aOut);

nsresult
Convert(uint32_t aIn, int& aOut);

nsresult
Convert(uint32_t aIn, uint8_t& aOut);

nsresult
Convert(size_t aIn, uint16_t& aOut);

//
// Packing
//

// introduce link errors on non-handled data types
template <typename T>
nsresult
PackPDU(T aIn, DaemonSocketPDU& aPDU);

inline nsresult
PackPDU(uint8_t aIn, DaemonSocketPDU& aPDU)
{
  return aPDU.Write(aIn);
}

inline nsresult
PackPDU(uint16_t aIn, DaemonSocketPDU& aPDU)
{
  return aPDU.Write(aIn);
}

inline nsresult
PackPDU(int32_t aIn, DaemonSocketPDU& aPDU)
{
  return aPDU.Write(aIn);
}

inline nsresult
PackPDU(uint32_t aIn, DaemonSocketPDU& aPDU)
{
  return aPDU.Write(aIn);
}

nsresult
PackPDU(const DaemonSocketPDUHeader& aIn, DaemonSocketPDU& aPDU);

//
// Unpacking
//

// introduce link errors on non-handled data types
template <typename T>
nsresult
UnpackPDU(DaemonSocketPDU& aPDU, T& aOut);

inline nsresult
UnpackPDU(DaemonSocketPDU& aPDU, int8_t& aOut)
{
  return aPDU.Read(aOut);
}

inline nsresult
UnpackPDU(DaemonSocketPDU& aPDU, uint8_t& aOut)
{
  return aPDU.Read(aOut);
}

inline nsresult
UnpackPDU(DaemonSocketPDU& aPDU, uint16_t& aOut)
{
  return aPDU.Read(aOut);
}

inline nsresult
UnpackPDU(DaemonSocketPDU& aPDU, int32_t& aOut)
{
  return aPDU.Read(aOut);
}

inline nsresult
UnpackPDU(DaemonSocketPDU& aPDU, uint32_t& aOut)
{
  return aPDU.Read(aOut);
}

inline nsresult
UnpackPDU(DaemonSocketPDU& aPDU, DaemonSocketPDUHeader& aOut)
{
  nsresult rv = UnpackPDU(aPDU, aOut.mService);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = UnpackPDU(aPDU, aOut.mOpcode);
  if (NS_FAILED(rv)) {
    return rv;
  }
  return UnpackPDU(aPDU, aOut.mLength);
}

nsresult
UnpackPDU(DaemonSocketPDU& aPDU, nsDependentCString& aOut);

/* |UnpackCString0| is a helper for unpacking 0-terminated C string,
 * including the \0 character. Pass an instance of this structure
 * as the first argument to |UnpackPDU| to unpack a string.
 */
struct UnpackCString0
{
  UnpackCString0(nsCString& aString)
    : mString(&aString)
  { }

  nsCString* mString; // non-null by construction
};

/* This implementation of |UnpackPDU| unpacks a 0-terminated C string.
 */
nsresult
UnpackPDU(DaemonSocketPDU& aPDU, const UnpackCString0& aOut);

/* |UnpackString0| is a helper for unpacking 0-terminated C string,
 * including the \0 character. Pass an instance of this structure
 * as the first argument to |UnpackPDU| to unpack a C string and convert
 * it to wide-character encoding.
 */
struct UnpackString0
{
  UnpackString0(nsString& aString)
    : mString(&aString)
  { }

  nsString* mString; // non-null by construction
};

/* This implementation of |UnpackPDU| unpacks a 0-terminated C string
 * and converts it to wide-character encoding.
 */
nsresult
UnpackPDU(DaemonSocketPDU& aPDU, const UnpackString0& aOut);

//
// Init operators
//

//
// Below are general-purpose init operators for Bluetooth. The classes
// of type |ConstantInitOp[1..3]| initialize results or notifications
// with constant values.
//

template <typename T1>
class ConstantInitOp1 final
{
public:
  ConstantInitOp1(const T1& aArg1)
    : mArg1(aArg1)
  { }

  nsresult operator () (T1& aArg1) const
  {
    aArg1 = mArg1;

    return NS_OK;
  }

private:
  const T1& mArg1;
};

template <typename T1, typename T2>
class ConstantInitOp2 final
{
public:
  ConstantInitOp2(const T1& aArg1, const T2& aArg2)
    : mArg1(aArg1)
    , mArg2(aArg2)
  { }

  nsresult operator () (T1& aArg1, T2& aArg2) const
  {
    aArg1 = mArg1;
    aArg2 = mArg2;

    return NS_OK;
  }

private:
  const T1& mArg1;
  const T2& mArg2;
};

template <typename T1, typename T2, typename T3>
class ConstantInitOp3 final
{
public:
  ConstantInitOp3(const T1& aArg1, const T2& aArg2, const T3& aArg3)
    : mArg1(aArg1)
    , mArg2(aArg2)
    , mArg3(aArg3)
  { }

  nsresult operator () (T1& aArg1, T2& aArg2, T3& aArg3) const
  {
    aArg1 = mArg1;
    aArg2 = mArg2;
    aArg3 = mArg3;

    return NS_OK;
  }

private:
  const T1& mArg1;
  const T2& mArg2;
  const T3& mArg3;
};

// |PDUInitOP| provides functionality for init operators that unpack PDUs.
class PDUInitOp
{
protected:
  PDUInitOp(DaemonSocketPDU& aPDU)
    : mPDU(&aPDU)
  { }

  DaemonSocketPDU& GetPDU() const
  {
    return *mPDU; // cannot be nullptr
  }

  void WarnAboutTrailingData() const;

private:
  DaemonSocketPDU* mPDU; // Hold pointer to allow for constant instances
};

} // namespace DaemonSocketPDUHelpers

} // namespace ipc
} // namespace mozilla

#endif // mozilla_ipc_DaemonSocketPDUHelpers_h
