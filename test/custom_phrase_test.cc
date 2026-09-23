//
// Copyright RIME Developers
// Distributed under the BSD License
//
// custom_phrase.txt is a stabledb table: "phrase <Tab> code [ <Tab> weight ]".
// Entries without a weight column are packed as c=0 d=0 t=0. CreateDictEntry
// must still return them; otherwise table_translator@custom_phrase yields
// nothing (e.g. zdydu → 自定义短语).
//
#include <gtest/gtest.h>
#include <rime/common.h>
#include <rime/dict/user_dictionary.h>

using namespace rime;

// Unweighted table/stabledb rows pack as c=0 d=0 t=0. That is "never ticked",
// not a decayed userdb phrase, and must still produce a dict entry.
TEST(UserDictionary, UnweightedTableEntryIsNotDiscarded) {
  auto e = UserDictionary::CreateDictEntry("zdydu \t自定义短语", "c=0 d=0 t=0",
                                           /*present_tick=*/1);
  ASSERT_TRUE(e) << "unweighted custom_phrase row (dee=0, tick=0) was dropped";
  EXPECT_EQ("自定义短语", e->text);
}

// de21e7d4: truly decayed userdb entries stay suppressed.
TEST(UserDictionary, VeryOldDecayedEntryIsDiscarded) {
  auto e =
      UserDictionary::CreateDictEntry("foo \tbar", "c=1 d=1e-201 t=1000",
                                      /*present_tick=*/1000, 0.0, 0.0, nullptr,
                                      /*discard_threshold=*/1e-200);
  EXPECT_FALSE(e);
}

// 門檻為 0（fork 預設）時不遺忘：落後十萬 tick 的舊詞條衰減後 dee 約 2e-218，
// 仍建立 DictEntry
TEST(UserDictionary, AgedEntryIsKeptWhenThresholdIsZero) {
  auto e = UserDictionary::CreateDictEntry("qi \t妻", "c=22 d=0.303 t=1577323",
                                           /*present_tick=*/1677323);
  ASSERT_TRUE(e) << "aged userdb entry was dropped with threshold 0";
  EXPECT_EQ("妻", e->text);
  EXPECT_EQ(22, e->commit_count);
}

// 門檻設為上游常量 1e-200 時維持上游行為：同一舊詞條被丟棄
TEST(UserDictionary, AgedEntryIsDiscardedWhenThresholdIsUpstream) {
  auto e = UserDictionary::CreateDictEntry("qi \t妻", "c=22 d=0.303 t=1577323",
                                           /*present_tick=*/1677323, 0.0, 0.0,
                                           nullptr,
                                           /*discard_threshold=*/1e-200);
  EXPECT_FALSE(e);
}

// 門檻不影響尚未過期的詞條
TEST(UserDictionary, RecentEntryIsKeptWithUpstreamThreshold) {
  auto e = UserDictionary::CreateDictEntry("qi \t七", "c=5 d=0.5 t=1665357",
                                           /*present_tick=*/1677323, 0.0, 0.0,
                                           nullptr,
                                           /*discard_threshold=*/1e-200);
  ASSERT_TRUE(e);
  EXPECT_EQ("七", e->text);
}
