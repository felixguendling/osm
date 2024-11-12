#pragma once

#include <string>
#include <string_view>

#include "utl/verify.h"

#include "zlib.h"

namespace osm {

struct inflate {
  inflate() {
    z_.zalloc = Z_NULL;
    z_.zfree = Z_NULL;
    z_.opaque = Z_NULL;
    auto const ec = inflateInit(&z_);
    utl::verify(ec == Z_OK, "inflate init failed: {}", ec);
  }

  void decompress(std::string_view in, std::string& out) {
    z_.next_in = const_cast<unsigned char*>(
        reinterpret_cast<unsigned char const*>(in.data()));
    z_.avail_in = in.size();
    z_.next_out = reinterpret_cast<unsigned char*>(out.data());
    z_.avail_out = out.size();

    auto const ec = ::inflate(&z_, Z_FINISH);
    utl_verify(ec == Z_STREAM_END, "inflate failed: {}", ec);
    out.resize(z_.total_out);
    inflateReset(&z_);
  }

  z_stream z_;
};

}  // namespace osm