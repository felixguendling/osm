#pragma once

#include <protozero/types.hpp>

namespace osm {

enum class blob : protozero::pbf_tag_type {
  optional_bytes_raw = 1,
  optional_int32_raw_size = 2,
  optional_bytes_zlib_data = 3,
  optional_bytes_lzma_data = 4,
  optional_bytes_lz4_data = 6,
  optional_bytes_zstd_data = 7
};

enum class blob_header : protozero::pbf_tag_type {
  required_string_type = 1,
  optional_bytes_indexdata = 2,
  required_int32_datasize = 3
};

enum class header_block : protozero::pbf_tag_type {
  optional_HeaderBBox_bbox = 1,
  repeated_string_required_features = 4,
  repeated_string_optional_features = 5,
  optional_string_writingprogram = 16,
  optional_string_source = 17,
  optional_int64_osmosis_replication_timestamp = 32,
  optional_int64_osmosis_replication_sequence_number = 33,
  optional_string_osmosis_replication_base_url = 34
};

enum class header_bbox : protozero::pbf_tag_type {
  required_sint64_left = 1,
  required_sint64_right = 2,
  required_sint64_top = 3,
  required_sint64_bottom = 4
};

enum class primitive_block : protozero::pbf_tag_type {
  required_StringTable_stringtable = 1,
  repeated_PrimitiveGroup_primitivegroup = 2,
  optional_int32_granularity = 17,
  optional_int32_date_granularity = 18,
  optional_int64_lat_offset = 19,
  optional_int64_lon_offset = 20
};

enum class primitive_group : protozero::pbf_tag_type {
  unknown = 0,
  repeated_Node_nodes = 1,
  optional_DenseNodes_dense = 2,
  repeated_Way_ways = 3,
  repeated_Relation_relations = 4,
  repeated_ChangeSet_changesets = 5
};

enum class string_table : protozero::pbf_tag_type { repeated_bytes_s = 1 };

enum class info : protozero::pbf_tag_type {
  optional_int32_version = 1,
  optional_int64_timestamp = 2,
  optional_int64_changeset = 3,
  optional_int32_uid = 4,
  optional_uint32_user_sid = 5,
  optional_bool_visible = 6
};

enum class dense_info : protozero::pbf_tag_type {
  packed_int32_version = 1,
  packed_sint64_timestamp = 2,
  packed_sint64_changeset = 3,
  packed_sint32_uid = 4,
  packed_sint32_user_sid = 5,
  packed_bool_visible = 6
};

enum class node : protozero::pbf_tag_type {
  required_sint64_id = 1,
  packed_uint32_keys = 2,
  packed_uint32_vals = 3,
  optional_Info_info = 4,
  required_sint64_lat = 8,
  required_sint64_lon = 9
};

enum class dense_nodes : protozero::pbf_tag_type {
  packed_sint64_id = 1,
  optional_DenseInfo_denseinfo = 5,
  packed_sint64_lat = 8,
  packed_sint64_lon = 9,
  packed_int32_keys_vals = 10
};

enum class way : protozero::pbf_tag_type {
  required_int64_id = 1,
  packed_uint32_keys = 2,
  packed_uint32_vals = 3,
  optional_Info_info = 4,
  packed_sint64_refs = 8,
  packed_sint64_lat = 9,
  packed_sint64_lon = 10
};

enum class relation : protozero::pbf_tag_type {
  required_int64_id = 1,
  packed_uint32_keys = 2,
  packed_uint32_vals = 3,
  optional_Info_info = 4,
  packed_int32_roles_sid = 8,
  packed_sint64_memids = 9,
  packed_MemberType_types = 10
};

}  // namespace osm
