// IWYU pragma: private
#pragma once
#include <rapidjson/document.h>

#include <type_traits>

#include "../../support/logging.h"

namespace xgrammar::details {

template <typename, typename = void>
struct has_json_serialize_member : std::false_type {};

template <typename T>
struct has_json_serialize_member<T, std::void_t<decltype(std::declval<const T&>().JSONSerialize())>>
    : std::true_type {
  static_assert(
      std::is_same_v<decltype(std::declval<const T&>().JSONSerialize()), rapidjson::Value>,
      "JSONSerialize must be a const method returning picojson::value"
  );
};

template <typename, typename = void>
struct has_json_serialize_global : std::false_type {};

template <typename T>
struct has_json_serialize_global<T, std::void_t<decltype(JSONSerialize(std::declval<const T&>()))>>
    : std::true_type {
  static_assert(
      std::is_same_v<decltype(JSONSerialize(std::declval<const T&>())), rapidjson::Value>,
      "JSONSerialize must be a global function returning picojson::value"
  );
};

template <typename, typename = void>
struct has_json_deserialize_member : std::false_type {};

template <typename T>
struct has_json_deserialize_member<T, std::void_t<decltype(T::JSONDeserialize(rapidjson::Value{}))>>
    : std::true_type {
  static_assert(
      std::is_same_v<decltype(T::JSONDeserialize(rapidjson::Value{})), T>,
      "JSONDeserialize must be a static method returning T"
  );
};

template <typename T, typename = void>
struct has_json_deserialize_global : std::false_type {};

template <typename T>
struct has_json_deserialize_global<
    T,
    std::void_t<decltype(JSONDeserialize(std::declval<T&>(), rapidjson::Value{}))>>
    : std::true_type {
  static_assert(
      std::is_same_v<decltype(JSONDeserialize(std::declval<T&>(), rapidjson::Value{})), void>,
      "JSONDeserialize must be a global function returning void"
  );
  static_assert(
      std::is_default_constructible_v<T>,
      "global deserializer can only apply to a default constructible type"
  );
};

template <typename T>
inline const T& json_as(const rapidjson::Value& value) {
  XGRAMMAR_CHECK(value.Is<T>()) << "Wrong type in JSONDeserialize";
  return value.Get<T>();
}

inline const rapidjson::Value& json_member(
    rapidjson::Value::ConstObject value, const std::string& name
) {
  auto it = value.FindMember(name);
  XGRAMMAR_CHECK(it != value.MemberEnd()) << "Missing member in JSONDeserialize";
  return it->value;
}

}  // namespace xgrammar::details
