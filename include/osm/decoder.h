#pragma once

#include <ranges>
#include <string_view>
#include <vector>

#include "protozero/pbf_message.hpp"

#include "geo/latlng.h"

#include "utl/verify.h"
#include "utl/zip.h"

#include "osm/tags.h"
#include "osm/varint.h"

namespace osm {

constexpr auto const kMaxStringLength = 256U * 4U;
constexpr auto const kNanoDegree = 1'000'000'000.0;

enum member_type : std::uint32_t { kNode, kWay, kRelation };

struct meta_data {
  geo::latlng to_latlng(std::int64_t const lat, std::int64_t const lon) const {
    return {(lat_offset_ + lat * granularity_) / kNanoDegree,
            (lon_offset_ + lon * granularity_) / kNanoDegree};
  }

  std::int32_t granularity_{100U};
  std::int64_t lat_offset_;
  std::int64_t lon_offset_;
};

void decode_string_table(std::string_view s,
                         std::vector<std::string_view>& strings) {
  auto pbf_string_table = protozero::pbf_message<string_table>{s};
  while (pbf_string_table.next(string_table::repeated_bytes_s,
                               protozero::pbf_wire_type::length_delimited)) {
    const auto str_view = pbf_string_table.get_view();
    utl_verify(str_view.length() < kMaxStringLength, "bad string {}", str_view);
    strings.emplace_back(str_view);
  }
}

meta_data decode_primitive_block_metadata(
    std::string_view s, std::vector<std::string_view>& strings) {
  auto m = meta_data{};
  auto pbf_primitive_block = protozero::pbf_message<primitive_block>{s};
  while (pbf_primitive_block.next()) {
    switch (pbf_primitive_block.tag_and_type()) {
      case protozero::tag_and_type(
          primitive_block::required_StringTable_stringtable,
          protozero::pbf_wire_type::length_delimited):
        decode_string_table(pbf_primitive_block.get_view(), strings);
        break;

      case protozero::tag_and_type(primitive_block::optional_int32_granularity,
                                   protozero::pbf_wire_type::varint):
        m.granularity_ = pbf_primitive_block.get_int32();
        break;

      case protozero::tag_and_type(primitive_block::optional_int64_lat_offset,
                                   protozero::pbf_wire_type::varint):
        m.lat_offset_ = pbf_primitive_block.get_int64();
        break;

      case protozero::tag_and_type(primitive_block::optional_int64_lon_offset,
                                   protozero::pbf_wire_type::varint):
        m.lon_offset_ = pbf_primitive_block.get_int64();
        break;

      default: pbf_primitive_block.skip();
    }
  }
  return m;
}

template <typename Fn>
void decode_dense_nodes(std::string_view s,
                        std::vector<std::string_view> const& strings,
                        meta_data const& meta,
                        Fn&& f) {
  auto ids = delta_varint<std::int64_t>{};
  auto lats = delta_varint<std::int64_t>{};
  auto lons = delta_varint<std::int64_t>{};
  auto tags = std::string_view{};

  auto pbf_dense_nodes = protozero::pbf_message<dense_nodes>{s};
  while (pbf_dense_nodes.next()) {
    switch (pbf_dense_nodes.tag_and_type()) {
      case protozero::tag_and_type(dense_nodes::packed_sint64_id,
                                   protozero::pbf_wire_type::length_delimited):
        ids = {pbf_dense_nodes.get_view()};
        break;

      case protozero::tag_and_type(dense_nodes::packed_sint64_lat,
                                   protozero::pbf_wire_type::length_delimited):
        lats = {pbf_dense_nodes.get_view()};
        break;

      case protozero::tag_and_type(dense_nodes::packed_sint64_lon,
                                   protozero::pbf_wire_type::length_delimited):
        lons = {pbf_dense_nodes.get_view()};
        break;

      case protozero::tag_and_type(dense_nodes::packed_int32_keys_vals,
                                   protozero::pbf_wire_type::length_delimited):
        tags = pbf_dense_nodes.get_view();
        break;

      default: pbf_dense_nodes.skip();
    }
  }

  for (auto const [id, lat, lon] : std::views::zip(ids, lats, lons)) {
    auto const separator_pos = tags.find('\0');
    auto const node_tags =
        varint<std::uint32_t>{separator_pos == std::string_view::npos
                                  ? std::string_view{}
                                  : tags.substr(0, separator_pos)} |
        std::views::chunk(2) | std::views::transform([&](auto&& y) {
          auto it = std::ranges::begin(y);
          auto const k = *it;
          auto const v = *++it;
          return std::tuple{strings.at(k), strings.at(v)};
        });
    tags = tags.substr(separator_pos + 1U);
    f(id, meta.to_latlng(lat, lon), node_tags);
  }
}

template <typename Fn>
void decode_node(std::string_view s,
                 meta_data const& m,
                 std::vector<std::string_view> const& strings,
                 Fn&& f) {
  auto keys = varint<std::uint32_t>{};
  auto values = varint<std::uint32_t>{};
  auto id = std::int64_t{};
  auto lon = std::numeric_limits<std::int64_t>::max();
  auto lat = std::numeric_limits<std::int64_t>::max();

  auto pbf_node = protozero::pbf_message<node>{s};
  while (pbf_node.next()) {
    switch (pbf_node.tag_and_type()) {
      case protozero::tag_and_type(node::required_sint64_id,
                                   protozero::pbf_wire_type::varint):
        id = pbf_node.get_sint64();
        break;

      case protozero::tag_and_type(node::packed_uint32_keys,
                                   protozero::pbf_wire_type::length_delimited):
        keys = {pbf_node.get_view()};
        break;

      case protozero::tag_and_type(node::packed_uint32_vals,
                                   protozero::pbf_wire_type::length_delimited):
        values = {pbf_node.get_view()};
        break;

      case protozero::tag_and_type(node::required_sint64_lat,
                                   protozero::pbf_wire_type::varint):
        lat = pbf_node.get_sint64();
        break;

      case protozero::tag_and_type(node::required_sint64_lon,
                                   protozero::pbf_wire_type::varint):
        lon = pbf_node.get_sint64();
        break;

      default: pbf_node.skip();
    }
  }

  using namespace std::views;
  auto const tags =
      zip(keys, values) | transform([&](auto&& x) {
        return std::tuple{strings.at(get<0>(x)), strings.at(get<1>(x))};
      });
  f(id, m.to_latlng(lat, lon), tags);
}

template <typename Fn>
void decode_way(std::string_view s,
                std::vector<std::string_view> const& strings,
                Fn&& f) {
  auto id = std::uint64_t{};
  auto keys = varint<std::uint32_t>{};
  auto values = varint<std::uint32_t>{};
  auto refs = varint<std::int64_t>{};

  protozero::pbf_message<way> pbf_way{s};
  while (pbf_way.next()) {
    switch (pbf_way.tag_and_type()) {
      case protozero::tag_and_type(way::required_int64_id,
                                   protozero::pbf_wire_type::varint):
        id = pbf_way.get_int64();
        break;

      case protozero::tag_and_type(way::packed_uint32_keys,
                                   protozero::pbf_wire_type::length_delimited):
        keys = {pbf_way.get_view()};
        break;

      case protozero::tag_and_type(way::packed_uint32_vals,
                                   protozero::pbf_wire_type::length_delimited):
        values = {pbf_way.get_view()};
        break;

      case protozero::tag_and_type(way::packed_sint64_refs,
                                   protozero::pbf_wire_type::length_delimited):
        refs = {pbf_way.get_view()};
        break;

      default: pbf_way.skip();
    }
  }

  using namespace std::views;
  auto const tags =
      zip(keys, values) | transform([&](auto&& x) {
        return std::tuple{strings.at(get<0>(x)), strings.at(get<1>(x))};
      });
  f(id, refs, tags);
}

template <typename Fn>
void decode_relation(std::string_view s,
                     std::vector<std::string_view> const& strings,
                     Fn&& f) {
  auto id = std::uint64_t{};
  auto keys = varint<std::uint32_t>{};
  auto values = varint<std::uint32_t>{};
  auto roles = varint<std::uint32_t>{};
  auto types = varint<std::uint32_t>{};
  auto refs = varint<std::int64_t>{};

  auto pbf_relation = protozero::pbf_message<relation>{s};
  while (pbf_relation.next()) {
    switch (pbf_relation.tag_and_type()) {
      case protozero::tag_and_type(relation::required_int64_id,
                                   protozero::pbf_wire_type::varint):
        id = pbf_relation.get_int64();
        break;

      case protozero::tag_and_type(relation::packed_uint32_keys,
                                   protozero::pbf_wire_type::length_delimited):
        keys = {pbf_relation.get_view()};
        break;

      case protozero::tag_and_type(relation::packed_uint32_vals,
                                   protozero::pbf_wire_type::length_delimited):
        values = {pbf_relation.get_view()};
        break;

      case protozero::tag_and_type(relation::packed_int32_roles_sid,
                                   protozero::pbf_wire_type::length_delimited):
        roles = {pbf_relation.get_view()};
        break;

      case protozero::tag_and_type(relation::packed_sint64_memids,
                                   protozero::pbf_wire_type::length_delimited):
        refs = {pbf_relation.get_view()};
        break;

      case protozero::tag_and_type(relation::packed_MemberType_types,
                                   protozero::pbf_wire_type::length_delimited):
        types = {pbf_relation.get_view()};
        break;

      default: pbf_relation.skip();
    }
  }

  using namespace std::views;
  auto const tags =
      zip(keys, values) | transform([&](auto&& x) {
        return std::tuple{strings.at(get<0>(x)), strings.at(get<1>(x))};
      });
  auto const members =
      zip(refs, roles, types) | transform([&](auto&& x) {
        auto const [ref, role, type] = x;
        return std::tuple{ref, strings.at(role), member_type{type}};
      });
  f(id, members, tags);
}

template <typename NodeFn, typename WayFn, typename RelFn>
void decode_primitive(std::string_view s,
                      std::vector<std::string_view>& strings,
                      bool const read_nodes,
                      bool const read_ways,
                      bool const read_relations,
                      NodeFn&& on_node,
                      WayFn&& on_way,
                      RelFn&& on_rel) {
  strings.clear();
  auto const meta = decode_primitive_block_metadata(s, strings);
  auto pbf_primitive_block = protozero::pbf_message<primitive_block>{s};
  while (pbf_primitive_block.next(
      primitive_block::repeated_PrimitiveGroup_primitivegroup,
      protozero::pbf_wire_type::length_delimited)) {
    auto pbf_primitive_group = protozero::pbf_message<primitive_group>{
        pbf_primitive_block.get_message()};
    while (pbf_primitive_group.next()) {
      switch (pbf_primitive_group.tag_and_type()) {
        case protozero::tag_and_type(
            primitive_group::repeated_Node_nodes,
            protozero::pbf_wire_type::length_delimited):
          if (read_nodes) {
            decode_node(pbf_primitive_group.get_view(), meta, strings, on_node);
          } else {
            pbf_primitive_group.skip();
          }
          break;

        case protozero::tag_and_type(
            primitive_group::optional_DenseNodes_dense,
            protozero::pbf_wire_type::length_delimited):
          if (read_nodes) {
            decode_dense_nodes(pbf_primitive_group.get_view(), strings, meta,
                               on_node);
          } else {
            pbf_primitive_group.skip();
          }
          break;

        case protozero::tag_and_type(
            primitive_group::repeated_Way_ways,
            protozero::pbf_wire_type::length_delimited):
          if (read_ways) {
            decode_way(pbf_primitive_group.get_view(), strings, on_way);
          } else {
            pbf_primitive_group.skip();
          }
          break;

        case protozero::tag_and_type(
            primitive_group::repeated_Relation_relations,
            protozero::pbf_wire_type::length_delimited):
          if (read_relations) {
            decode_relation(pbf_primitive_group.get_view(), strings, on_rel);
          } else {
            pbf_primitive_group.skip();
          }
          break;

        default: pbf_primitive_group.skip();
      }
    }
  }
}

}  // namespace osm