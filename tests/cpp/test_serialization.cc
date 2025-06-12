#include <gtest/gtest.h>

#include "persistent_stack.h"
#include "reflection/json.h"

TEST(XGrammarReflectionTest, JSONSerialization) {
  using namespace xgrammar;
  StackElement element;
  auto v = AutoJSONSerialize(element);
  StackElement element2;
  AutoJSONDeserialize(element2, v);
  ASSERT_EQ(element2, element);
}
