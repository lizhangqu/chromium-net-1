// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/ip_address.h"

#include "net/base/ip_address_number.h"
#include "url/gurl.h"
#include "url/url_canon_ip.h"

namespace net {

const size_t IPAddress::kIPv4AddressSize = 4;
const size_t IPAddress::kIPv6AddressSize = 16;

IPAddress::IPAddress() {}

IPAddress::IPAddress(const IPAddressNumber& address) : ip_address_(address) {}

IPAddress::IPAddress(const uint8_t* address, size_t address_len)
    : ip_address_(address, address + address_len) {}

IPAddress::~IPAddress() {}

bool IPAddress::IsIPv4() const {
  return ip_address_.size() == kIPv4AddressSize;
}

bool IPAddress::IsIPv6() const {
  return ip_address_.size() == kIPv6AddressSize;
}

bool IPAddress::IsValid() const {
  return IsIPv4() || IsIPv6();
}

bool IPAddress::IsReserved() const {
  return IsIPAddressReserved(ip_address_);
}

bool IPAddress::IsZero() const {
  for (auto x : ip_address_) {
    if (x != 0)
      return false;
  }

  return !empty();
}

bool IPAddress::IsIPv4MappedIPv6() const {
  return net::IsIPv4Mapped(ip_address_);
}

std::string IPAddress::ToString() const {
  return IPAddressToString(ip_address_);
}

// static
bool IPAddress::FromIPLiteral(const base::StringPiece& ip_literal,
                              IPAddress* ip_address) {
  std::vector<uint8_t> number;
  if (!ParseIPLiteralToNumber(ip_literal, &number))
    return false;

  std::swap(number, ip_address->ip_address_);
  return true;
}

bool IPAddress::operator==(const IPAddress& that) const {
  return ip_address_ == that.ip_address_;
}

bool IPAddress::operator!=(const IPAddress& that) const {
  return ip_address_ != that.ip_address_;
}

bool IPAddress::operator<(const IPAddress& that) const {
  // Sort IPv4 before IPv6.
  if (ip_address_.size() != that.ip_address_.size()) {
    return ip_address_.size() < that.ip_address_.size();
  }

  return ip_address_ < that.ip_address_;
}

std::string IPAddressToStringWithPort(const IPAddress& address, uint16_t port) {
  return IPAddressToStringWithPort(address.bytes(), port);
}

std::string IPAddressToPackedString(const IPAddress& address) {
  return IPAddressToPackedString(address.bytes());
}

IPAddress ConvertIPv4ToIPv4MappedIPv6(const IPAddress& address) {
  return IPAddress(ConvertIPv4NumberToIPv6Number(address.bytes()));
}

IPAddress ConvertIPv4MappedIPv6ToIPv4(const IPAddress& address) {
  return IPAddress(ConvertIPv4MappedToIPv4(address.bytes()));
}

}  // namespace net
