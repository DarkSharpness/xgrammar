#pragma once

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>

#include "logging.h"

namespace xgrammar {

inline rapidjson::Document& xgrammar_document() {
  static rapidjson::Document allocator;
  return allocator;
}

inline rapidjson::MemoryPoolAllocator<>& xgrammar_json_allocator() {
  return xgrammar_document().GetAllocator();
}

inline std::string serialize_json(const rapidjson::Value& obj) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  obj.Accept(writer);
  return std::string(buffer.GetString(), buffer.GetLength());
}

inline rapidjson::Value deserialize_string(const std::string& json) {
  rapidjson::Document doc;
  XGRAMMAR_CHECK(!doc.Parse(json).HasParseError()) << "JSON parse error: " << doc.GetParseError();
  return rapidjson::Value(doc, xgrammar_document().GetAllocator());
}

}  // namespace xgrammar
