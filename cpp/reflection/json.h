#pragma once
#include <rapidjson/document.h>

#include <cstddef>
#include <type_traits>

#include "../support/json_allocator.h"
#include "../support/logging.h"
#include "details/json.h"  // IWYU pragma: export
#include "reflection.h"
#include "utils.h"

namespace xgrammar {

template <typename T>
inline rapidjson::Value AutoJSONSerialize(const T& value);

template <typename T>
inline void AutoJSONDeserialize(T& result, const rapidjson::Value& value);

template <typename T>
inline rapidjson::Value TraitJSONSerialize(const T& value);

template <typename T>
inline void TraitJSONDeserialize(T& result, const rapidjson::Value& value);

template <typename T>
inline rapidjson::Value TraitJSONSerialize(const T& value) {
  using Functor = details::member_functor<T>;
  auto& alloc = xgrammar_json_allocator();
  if constexpr (Functor::value == member_type::kConfig) {
    if constexpr (Functor::has_names) {
      // normal named struct
      rapidjson::Value obj(rapidjson::kObjectType);
      obj.MemberReserve(Functor::member_count, alloc);
      // obj.reserve(Functor::member_count);
      details::visit_config<T>([&](auto ptr, const char* name, std::size_t idx) {
        obj[name] = AutoJSONSerialize(value.*ptr);
      });
      return obj;
    } else if constexpr (Functor::member_count == 1) {
      // optimize for single member unnamed structs
      constexpr auto member_ptr = std::get<0>(Functor::members);
      return AutoJSONSerialize(value.*member_ptr);
    } else {
      // normal unnamed struct
      rapidjson::Value arr(rapidjson::kArrayType);
      arr.Reserve(Functor::member_count, alloc);
      details::visit_config<T>([&](auto ptr, const char* name, std::size_t idx) {
        arr.PushBack(AutoJSONSerialize(value.*ptr), alloc);
      });
      return arr;
    }
  } else if constexpr (Functor::value == member_type::kDelegate) {
    // just cast to the delegate type
    return AutoJSONSerialize(Functor::into(value));
  } else {
    // should give an error in this case
    static_assert(details::false_v<T>, "Invalid trait type");
    return rapidjson::Value{};
  }
}

template <typename T>
inline void TraitJSONDeserialize(T& result, const rapidjson::Value& value) {
  using Functor = details::member_functor<T>;
  if constexpr (Functor::value == member_type::kConfig) {
    if constexpr (Functor::has_names) {
      // normal named struct
      XGRAMMAR_CHECK(value.IsObject()) << "Expected an object in JSONDeserialize";
      auto obj = details::json_as<rapidjson::Value::ConstObject>(value);
      XGRAMMAR_CHECK(obj.MemberCount() == Functor::member_count)
          << "Wrong number of members in object in JSONDeserialize" << " expected "
          << Functor::member_count << " but got " << obj.MemberCount();
      details::visit_config<T>([&](auto ptr, const char* name, std::size_t idx) {
        AutoJSONDeserialize(result.*ptr, details::json_member(obj, name));
      });
    } else if constexpr (Functor::member_count == 1) {
      // optimize for single member unnamed structs
      constexpr auto member_ptr = std::get<0>(Functor::members);
      AutoJSONDeserialize(result.*member_ptr, value);
    } else {
      // normal unnamed struct
      auto arr = details::json_as<rapidjson::Value::ConstArray>(value);
      XGRAMMAR_CHECK(arr.Size() == Functor::member_count)
          << "Wrong number of elements in array in JSONDeserialize" << " expected "
          << Functor::member_count << " but got " << arr.Size();
      details::visit_config<T>([&arr, &result](auto ptr, const char* name, size_t idx) {
        AutoJSONDeserialize(result.*ptr, arr[idx]);
      });
    }
  } else if constexpr (Functor::value == member_type::kDelegate) {
    // cast to the delegate type
    typename Functor::delegate_type value_delegate;
    AutoJSONDeserialize(value_delegate, value);
    result = Functor::from(value_delegate);
  } else {
    // should give an error in this case
    static_assert(details::false_v<T>, "Invalid trait type");
  }
}

template <typename T>
inline rapidjson::Value AutoJSONSerialize(const T& value) {
  // always prefer user-defined JSONSerialize
  if constexpr (details::has_json_serialize_member<T>::value) {
    return value.JSONSerialize();
  } else if constexpr (details::has_json_serialize_global<T>::value) {
    return JSONSerialize(value);
  } else if constexpr (std::is_same_v<T, bool>) {
    return rapidjson::Value(value);
  } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
    return rapidjson::Value(static_cast<int64_t>(value));
  } else if constexpr (std::is_floating_point_v<T>) {
    return rapidjson::Value(static_cast<double>(value));
  } else if constexpr (std::is_same_v<T, std::string>) {
    auto& alloc = xgrammar_json_allocator();
    return rapidjson::Value(value, alloc);
  } else if constexpr (details::is_optional<T>::value) {
    if (value.has_value()) {
      return AutoJSONSerialize(*value);
    } else {
      return rapidjson::Value();
    }
  } else if constexpr (details::is_vector<T>::value) {
    auto& alloc = xgrammar_json_allocator();
    rapidjson::Value arr(rapidjson::kArrayType);
    arr.Reserve(value.size(), alloc);
    for (const auto& item : value) {
      arr.PushBack(AutoJSONSerialize(item), alloc);
    }
    return arr;
  } else if constexpr (details::is_unordered_set<T>::value) {
    auto& alloc = xgrammar_json_allocator();
    rapidjson::Value arr(rapidjson::kArrayType);
    arr.Reserve(value.size(), alloc);
    for (const auto& item : value) {
      arr.PushBack(AutoJSONSerialize(item), alloc);
    }
    return arr;
  } else if constexpr (details::is_unordered_map<T>::value) {
    auto& alloc = xgrammar_json_allocator();
    rapidjson::Value arr(rapidjson::kArrayType);
    arr.Reserve(value.size() * 2, alloc);
    for (const auto& [key, item] : value) {
      arr.PushBack(AutoJSONSerialize(key), alloc);
      arr.PushBack(AutoJSONSerialize(item), alloc);
    }
    return arr;
  } else if constexpr (member_trait<T>::value != member_type::kNone) {
    return TraitJSONSerialize(value);
  } else {
    // should give an error in this case
    static_assert(details::false_v<T>, "Cannot serialize this type");
    return rapidjson::Value{};
  }
}

template <typename T>
inline void AutoJSONDeserialize(T& result, const rapidjson::Value& value) {
  static_assert(!std::is_const_v<T>, "Cannot deserialize into a const type");
  if constexpr (details::has_json_deserialize_member<T>::value) {
    result = T::JSONDeserialize(value);
  } else if constexpr (details::has_json_deserialize_global<T>::value) {
    JSONDeserialize(result, value);
  } else if constexpr (std::is_same_v<T, bool>) {
    result = details::json_as<bool>(value);
  } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
    result = static_cast<T>(details::json_as<int64_t>(value));
  } else if constexpr (std::is_floating_point_v<T>) {
    result = static_cast<T>(details::json_as<double>(value));
  } else if constexpr (std::is_same_v<T, std::string>) {
    result = details::json_as<std::string>(value);
  } else if constexpr (details::is_optional<T>::value) {
    if (value.IsNull()) {
      result.reset();
    } else {
      AutoJSONDeserialize(result.emplace(), value);
    }
  } else if constexpr (details::is_vector<T>::value) {
    result.clear();
    const auto& arr = details::json_as<rapidjson::Value::ConstArray>(value);
    result.reserve(arr.Size());
    for (const auto& item : arr) {
      auto& item_value = result.emplace_back();
      AutoJSONDeserialize(item_value, item);
    }
  } else if constexpr (details::is_unordered_set<T>::value) {
    result.clear();
    const auto& arr = details::json_as<rapidjson::Value::ConstArray>(value);
    result.reserve(arr.Size());
    for (const auto& item : arr) {
      typename T::value_type item_value;
      AutoJSONDeserialize(item_value, item);
      result.emplace(std::move(item_value));
    }
  } else if constexpr (details::is_unordered_map<T>::value) {
    const auto& arr = details::json_as<rapidjson::Value::ConstArray>(value);
    XGRAMMAR_CHECK(arr.Size() % 2 == 0) << "Wrong number of elements in array in JSONDeserialize";
    result.clear();
    result.reserve(arr.Size() / 2);
    for (size_t i = 0; i < arr.Size(); i += 2) {
      // typename T::value_type item_value;
      typename T::key_type key_value;
      AutoJSONDeserialize(key_value, arr[i + 0]);
      typename T::mapped_type item_value;
      AutoJSONDeserialize(item_value, arr[i + 1]);
      result.emplace(std::move(key_value), std::move(item_value));
    }
  } else if constexpr (member_trait<T>::value != member_type::kNone) {
    return TraitJSONDeserialize(result, value);
  } else {
    // should give an error in this case
    static_assert(details::false_v<T>, "Cannot deserialize this type");
  }
}

}  // namespace xgrammar
