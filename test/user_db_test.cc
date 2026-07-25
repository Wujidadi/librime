//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-03 GONG Chen <chen.sst@gmail.com>
//
#include <cmath>
#include <gtest/gtest.h>
#include <rime/algo/syllabifier.h>
#include <rime/dict/text_db.h>
#include <rime/dict/user_db.h>

using namespace rime;

using TestDb = UserDbWrapper<TextDb>;

static string PackUserDbValue(int commits, double dee, TickCount tick) {
  UserDbValue v;
  v.commits = commits;
  v.dee = dee;
  v.tick = tick;
  return v.Pack();
}

TEST(RimeUserDbTest, AccessRecordByKey) {
  TestDb db(path{"user_db_test.txt"}, "user_db_test");
  if (db.Exists())
    db.Remove();
  ASSERT_FALSE(db.Exists());
  db.Open();
  EXPECT_TRUE(db.loaded());
  EXPECT_TRUE(db.Update("abc", "ZYX"));
  EXPECT_TRUE(db.Update("zyx", "CBA"));
  EXPECT_TRUE(db.Update("zyx", "ABC"));
  string value;
  EXPECT_TRUE(db.Fetch("abc", &value));
  EXPECT_EQ("ZYX", value);
  value.clear();
  EXPECT_TRUE(db.Fetch("zyx", &value));
  EXPECT_EQ("ABC", value);
  value.clear();
  EXPECT_FALSE(db.Fetch("wvu", &value));
  EXPECT_TRUE(value.empty());
  value.clear();
  EXPECT_TRUE(db.Erase("zyx"));
  EXPECT_FALSE(db.Fetch("zyx", &value));
  EXPECT_TRUE(value.empty());
  EXPECT_TRUE(db.Close());
  ASSERT_FALSE(db.loaded());
}

TEST(RimeUserDbTest, Query) {
  TestDb db(path{"user_db_test.txt"}, "user_db_test");
  if (db.Exists())
    db.Remove();
  ASSERT_FALSE(db.Exists());
  db.Open();
  EXPECT_TRUE(db.Update("abc", "ZYX"));
  EXPECT_TRUE(db.Update("abc\tdef", "ZYX WVU"));
  EXPECT_TRUE(db.Update("zyx", "ABC"));
  EXPECT_TRUE(db.Update("wvu", "DEF"));
  {
    an<DbAccessor> accessor = db.Query("abc");
    ASSERT_TRUE(bool(accessor));
    EXPECT_FALSE(accessor->exhausted());
    string key, value;
    EXPECT_TRUE(accessor->GetNextRecord(&key, &value));
    EXPECT_EQ("abc", key);
    EXPECT_EQ("ZYX", value);
    key.clear();
    value.clear();
    EXPECT_TRUE(accessor->GetNextRecord(&key, &value));
    EXPECT_EQ("abc\tdef", key);
    EXPECT_EQ("ZYX WVU", value);
    key.clear();
    value.clear();
    EXPECT_FALSE(accessor->GetNextRecord(&key, &value));
    // key, value contain invalid contents
    EXPECT_EQ("", key);
    EXPECT_EQ("", value);
  }
  {
    an<DbAccessor> accessor = db.Query("wvu\tt");
    ASSERT_TRUE(bool(accessor));
    EXPECT_TRUE(accessor->exhausted());
    string key, value;
    EXPECT_FALSE(accessor->GetNextRecord(&key, &value));
  }
  {
    an<DbAccessor> accessor = db.Query("z");
    ASSERT_TRUE(bool(accessor));
    EXPECT_FALSE(accessor->exhausted());
    string key, value;
    EXPECT_TRUE(accessor->GetNextRecord(&key, &value));
    EXPECT_EQ("zyx", key);
    EXPECT_EQ("ABC", value);
    EXPECT_FALSE(accessor->GetNextRecord(&key, &value));
  }
  db.Close();
}

// 合併保留逐詞條事件 tick，dee 一致衰減至合併基準（資訊保持變換）
TEST(RimeUserDbMergeTest, PreservesPerEntryTickAndDee) {
  TestDb db(path{"user_db_merge_test.txt"}, "user_db_merge_test");
  if (db.Exists())
    db.Remove();
  db.Open();
  db.MetaUpdate("/tick", "1000");
  db.Update("jia \t家", PackUserDbValue(5, 0.8, 900));
  {
    UserDbMerger merger(&db);
    merger.MetaPut("/tick", "950");
    merger.Put("jia \t家", PackUserDbValue(3, 0.5, 700));
  }
  string value;
  ASSERT_TRUE(db.Fetch("jia \t家", &value));
  UserDbValue m(value);
  EXPECT_EQ(5, m.commits);
  EXPECT_EQ(900u, m.tick);  // 保留本地較新的事件 tick，而非蓋成 max_tick
  // v.dee 衰減至基準 900：0.5·exp((700−900)/200)≈0.1839 < 0.8，取本地值
  EXPECT_NEAR(0.8, m.dee, 1e-4);
  db.Close();
}

// 較新的刪除事件應傳播，且幅度取大以保留使用量歷史
TEST(RimeUserDbMergeTest, NewerDeletionPropagates) {
  TestDb db(path{"user_db_merge_test.txt"}, "user_db_merge_test");
  if (db.Exists())
    db.Remove();
  db.Open();
  db.MetaUpdate("/tick", "1000");
  db.Update("shan \t刪", PackUserDbValue(10, 0.6, 800));
  {
    UserDbMerger merger(&db);
    merger.MetaPut("/tick", "1200");
    merger.Put("shan \t刪", PackUserDbValue(-4, 0.3, 1100));
  }
  string value;
  ASSERT_TRUE(db.Fetch("shan \t刪", &value));
  UserDbValue m(value);
  EXPECT_EQ(-10, m.commits);  // 刪除傳播，幅度保留 10
  EXPECT_EQ(1100u, m.tick);
  db.Close();
}

// 較新的使用事件應勝過較舊的墓碑（復活）
TEST(RimeUserDbMergeTest, NewerCommitRevives) {
  TestDb db(path{"user_db_merge_test.txt"}, "user_db_merge_test");
  if (db.Exists())
    db.Remove();
  db.Open();
  db.MetaUpdate("/tick", "1000");
  db.Update("huo \t活", PackUserDbValue(-4, 0.2, 900));
  {
    UserDbMerger merger(&db);
    merger.MetaPut("/tick", "1200");
    merger.Put("huo \t活", PackUserDbValue(6, 0.7, 1150));
  }
  string value;
  ASSERT_TRUE(db.Fetch("huo \t活", &value));
  UserDbValue m(value);
  EXPECT_EQ(6, m.commits);
  EXPECT_EQ(1150u, m.tick);
  db.Close();
}

// tick 平手時刪除勝（保底規則）
TEST(RimeUserDbMergeTest, TieFavorsDeletion) {
  TestDb db(path{"user_db_merge_test.txt"}, "user_db_merge_test");
  if (db.Exists())
    db.Remove();
  db.Open();
  db.MetaUpdate("/tick", "1000");
  db.Update("ping \t平", PackUserDbValue(4, 0.5, 900));
  {
    UserDbMerger merger(&db);
    merger.MetaPut("/tick", "1000");
    merger.Put("ping \t平", PackUserDbValue(-4, 0.5, 900));
  }
  string value;
  ASSERT_TRUE(db.Fetch("ping \t平", &value));
  UserDbValue m(value);
  EXPECT_EQ(-4, m.commits);
  db.Close();
}

// 本地不存在的詞條：快照值全數採納（重建自快照的基礎行為）
TEST(RimeUserDbMergeTest, AdoptsSnapshotForMissingEntry) {
  TestDb db(path{"user_db_merge_test.txt"}, "user_db_merge_test");
  if (db.Exists())
    db.Remove();
  db.Open();
  db.MetaUpdate("/tick", "1000");
  {
    UserDbMerger merger(&db);
    merger.MetaPut("/tick", "800");
    merger.Put("xin \t新", PackUserDbValue(7, 0.4, 750));
    merger.Put("mu \t墓", PackUserDbValue(-2, 0.1, 600));
  }
  string value;
  ASSERT_TRUE(db.Fetch("xin \t新", &value));
  UserDbValue live(value);
  EXPECT_EQ(7, live.commits);
  EXPECT_EQ(750u, live.tick);
  EXPECT_NEAR(0.4, live.dee, 1e-4);
  ASSERT_TRUE(db.Fetch("mu \t墓", &value));
  UserDbValue tomb(value);
  EXPECT_EQ(-2, tomb.commits);
  db.Close();
}
