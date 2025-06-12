#include "json_serializer.h"

#include <rapidjson/fwd.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "compiled_grammar_data_structure.h"
#include "grammar_data_structure.h"  // IWYU pragma: keep
#include "reflection/json.h"
#include "support/json_allocator.h"
#include "support/logging.h"
#include "tokenizer_info_impl.h"
#include "xgrammar/compiler.h"
#include "xgrammar/grammar.h"
#include "xgrammar/tokenizer_info.h"

namespace xgrammar {

static rapidjson::Value JSONSerialize(const CompiledGrammar::Impl& impl) {
  auto result = rapidjson::Value(rapidjson::kObjectType);
  auto& alloc = xgrammar_json_allocator();
  result.AddMember("grammar", AutoJSONSerialize(*impl.grammar), alloc);
  result.AddMember("tokenizer_metadata", AutoJSONSerialize(*impl.tokenizer_info), alloc);
  result.AddMember(
      "adaptive_token_mask_cache", AutoJSONSerialize(impl.adaptive_token_mask_cache), alloc
  );
  return result;
}

static void JSONDeserialize(CompiledGrammar::Impl& impl, const rapidjson::Value& v) {
  auto object = details::json_as<rapidjson::Value::ConstObject>(v);
  impl.grammar = Grammar{std::make_shared<Grammar::Impl>()};
  AutoJSONDeserialize(*impl.grammar, details::json_member(object, "grammar"));
  impl.tokenizer_info = TokenizerInfo{std::make_shared<TokenizerInfo::Impl>()};
  AutoJSONDeserialize(*impl.tokenizer_info, details::json_member(object, "tokenizer_metadata"));
  impl.adaptive_token_mask_cache.clear();
  AutoJSONDeserialize(
      impl.adaptive_token_mask_cache, details::json_member(object, "adaptive_token_mask_cache")
  );
}

std::string JSONSerializer::SerializeGrammar(const Grammar& grammar, bool prettify) {
  return serialize_json(AutoJSONSerialize(*grammar));
}

std::string JSONSerializer::SerializeTokenizerInfo(
    const TokenizerInfo& tokenizer_info, bool prettify
) {
  return serialize_json(AutoJSONSerialize(*tokenizer_info));
}

std::string JSONSerializer::SerializeCompiledGrammar(
    const CompiledGrammar& compiled_grammar, bool prettify
) {
  return serialize_json(JSONSerialize(*compiled_grammar));
}

Grammar JSONSerializer::DeserializeGrammar(const std::string& str) {
  auto grammar = Grammar{std::make_shared<Grammar::Impl>()};
  AutoJSONDeserialize(*grammar, deserialize_string(str));
  return grammar;
}

TokenizerInfo JSONSerializer::DeserializeTokenizerInfo(
    const std::string& str, const std::vector<std::string>& encoded_vocab
) {
  if (encoded_vocab.empty()) {
    // simply build a tokenizer info with only metadata
    auto tokenizer_info = TokenizerInfo{std::make_shared<TokenizerInfo::Impl>()};
    AutoJSONDeserialize(*tokenizer_info, deserialize_string(str));
    return tokenizer_info;
  } else {
    // rebuild a complete tokenizer info with vocab
    return TokenizerInfo::FromVocabAndMetadata(encoded_vocab, str);
  }
}

bool TokenizerInfo::Impl::operator==(const TokenizerInfo::Impl& other) const {
  static constexpr auto tie = [](const TokenizerInfo::Impl& impl) {
    return std::tie(
        impl.vocab_type_,
        impl.vocab_size_,
        impl.add_prefix_space_,
        impl.stop_token_ids_,
        impl.special_token_ids_
    );
  };
  return tie(*this) == tie(other);
}

CompiledGrammar JSONSerializer::DeserializeCompiledGrammar(
    const std::string& str, const TokenizerInfo& tokenizer_info
) {
  auto compiled_grammar = CompiledGrammar{std::make_shared<CompiledGrammar::Impl>()};
  AutoJSONDeserialize(*compiled_grammar, deserialize_string(str));
  // compare the tokenizer info metadata
  XGRAMMAR_CHECK(*compiled_grammar->tokenizer_info == *tokenizer_info)
      << "The tokenizer info in the compiled grammar does not match the provided one.";
  // set the tokenizer info to the real one
  compiled_grammar->tokenizer_info = tokenizer_info;
  return compiled_grammar;
}

}  // namespace xgrammar
