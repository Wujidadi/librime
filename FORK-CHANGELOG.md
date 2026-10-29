# Fork 變更日誌

本檔記錄本 fork（Wujidadi/librime）相對上游 rime/librime 的所有變動，依 fork 版號分節。
上游自身的變更見 `CHANGELOG.md`。

## 1.17.0-wujidadi.3 — 2026-10-29

基於上游 master 1b3417d2。

### 行為變更（Shift 切換與 Enter 上屏）

以下以方案預設的 `Shift_L: inline_ascii`、`Shift_R: commit_text` 為準；`fluid_editor`（注音、地球拼音）與 `express_editor`（倉頡）分別註明。

- 臨時西文狀態下按 Return／空白，改為上屏編輯區的組字結果（`Context::GetCommitText`）：
  上游上屏整串輸入碼，右 Shift 確認的中文（`fluid_editor` 下確認而不上屏）因此退回原始按鍵，例如 `su3` 確認成「你」後按 Return 上屏 `su3`
- 左 Shift 離開臨時西文時一律不上屏：
  - 西文未修改：還原進入前的中文組字（與上游相同）
  - 西文已修改：西文定為已確認段留在編輯區，切回中文後可接著輸入，最後一併上屏（上游直接上屏西文）
  - 未經左 Shift 進入（如右 Shift 確認後）且未鍵入西文：只切回中文，已確認段保留（上游上屏整串輸入碼）
  - 右 Shift 確認後退格重開的中文段：雖仍處於西文模式，左 Shift 直接進入臨時西文、換上實體按鍵，再按一次還原中文組字
- 已確認段的西文改以 `Segment::kConfirmed` 直接標記，不經 `select_notifier`，`express_editor` 的自動上屏因此不會觸發
- 左 Shift 進入臨時西文時保留已確認段的輸入碼，只把未確認部分換成實體按鍵；上游以 `set_input(raw_keys_)` 覆蓋整串輸入，已確認段隨之消失
- 實體按鍵紀錄 `raw_keys_` 為空時（右 Shift 確認會清空，之後退格重開的段沒有紀錄），左 Shift 改從輸入碼取回未確認部分：上游此時以空字串覆蓋輸入，編輯區清空並切成全西文
- 編輯器把數字鍵盤 Enter（`KP_Enter`）比照主鍵盤 Return 處理，含 Shift、Control 組合：
  上游編輯器只綁定 `Return`，輸入中按數字鍵盤 Enter 不被 Rime 處理，編輯區原封不動、按鍵直送應用程式
- 右 Shift 在 `express_editor` 下照舊確認即上屏，刻意未改

## 1.17.0-wujidadi.2 — 2026-10-24

基於上游 master 1b3417d2。

### 上游同步

- 合入上游 master 1b3417d2（23 個 commit），含 boost 1.92.0、C++20 支援、依執行期條件啟用元件、`streaming_chord` 修正
- `test/user_db_test.cc` 衝突解法：fork 的合併測試與上游的 `RimeUserDbValueTest.Denormal` 並存

### 建置

- macOS 預設部署目標 `MACOSX_DEPLOYMENT_TARGET` 自 `10.15` 改為 `13.0`（與 Squirrel 專案一致）：
  Xcode／Command Line Tools 27 的 libc++ 拒絕 macOS 11 以下的部署目標，以 `-Werror` 編譯的 leveldb 等相依套件會直接失敗
- `user_db.h` 的 `UserDbValue` 移除 fork `9b46caec` 加在成員上的 `RIME_DLL`，改由上游 `d2ec5c18` 加在結構上的 `RIME_DLL` 統一匯出：
  兩者並存時 MSVC 報 C2487（DLL 介面類別的成員不可再以 DLL 介面宣告），macOS 不受影響

## 1.17.0-wujidadi.1 — 2026-09-23

基於上游 master ef1a16aa。

### 版號

- fork 版號調升為 `1.17.0-wujidadi.1`：僅改 `rime_fork_version`，`rime_version` 維持 `1.17.0`

### 行為變更（詞條老化）

- 上游 de21e7d4（PR #1205）在 `UserDictionary::CreateDictEntry` 加入常量門檻 `1e-200`：查詢時先把 dee 依 `exp((t − present_tick)/200)` 衰減，`t != 0` 且衰減後 dee 不高於門檻的詞條整條丟棄，相當於落後庫級 tick 約 92103 即遺忘
- 舊版合併語義曾把整批詞條的 t 蓋成 `max_tick`，這些詞條的 t 同時落後庫級 tick，跨過門檻時會一次全部失效、候選退回字典表順序；2026-09-22 在鼠鬚管實機驗證 terra_pinyin userdb 因此丟失 839,655 條
- 門檻改為設定項 `<ns>/user_dict_forget_threshold`（double，與 `enable_user_dict` 同層），fork 預設 `0` 代表不遺忘；設為 `1e-200` 即恢復上游行為，不必重編
- `CreateDictEntry` 末尾新增參數 `discard_threshold`（預設 `0`），`UserDictionary` 新增成員 `discard_threshold_` 與存取器；只動 `user_dictionary.h`／`user_dictionary.cc`，不碰 `user_db.cc` 的 `Pack`／`Unpack`，避免與上游 PR #1215（門檻搬到 `user_db.h` 並在 `Pack` 時鉗為 0）交疊
- 上游測試 `VeryOldDecayedEntryIsDiscarded` 改為明確傳入 `1e-200`；新增 3 項 `UserDictionary` 測試（門檻 0 保留老化詞條、門檻 `1e-200` 丟棄、門檻 `1e-200` 不影響新詞條）

## 1.17.0-wujidadi — 2026-07-26

基於上游 master d4c324ca（1.17.0 之後的開發版）。首個 fork 版。

### 版號

- fork 版號標記 `1.17.0-wujidadi`：僅改 `RIME_VERSION` 顯示字串（rime_api、installation.yaml、userdb 中繼資料），dylib 的 `-current_version` 維持純數字 `1.17.0`（連結器限制）；藉此可直接分辨機器上跑的是官方或 fork 的 librime
- Windows 端 `rime.dll` 的資源字串 `ProductVersion` 改用 `rime_fork_version`（`src/rime.rc` 三處語系區塊），檔案內容的詳細資料頁可直接分辨 fork；`FILEVERSION`／`PRODUCTVERSION` 數值欄位維持純數字四段（RC 格式硬性限制），與 dylib `-current_version` 的顧慮對等

### 建置修正（Windows）

- MSVC 加上 `/utf-8` 編譯旗標：在雙位元組系統字碼頁（如台灣的 CP950）下，MSVC 預設以系統字碼頁解讀原始碼，中文註解的 UTF-8 尾位元組會與換行符誤組成雙位元組字元而吞掉下一行程式碼（如 `spelling.h` 的 `Compose` 宣告），導致大量詭異編譯錯誤；上游 CI 用英文單位元組字碼頁只會註解亂碼、不會吞行，故從未暴露
- `user_db.h` 補 `RIME_DLL` 匯出標記（`UserDbValue` 的字串建構式、`Pack`、`Unpack`，及 `UserDbMerger` 類別）：`RimeUserDbMergeTest` 連結共享版 `rime.dll` 時需要這些符號；macOS 的 dylib 預設全符號可見，Windows 需明確標記

### 新功能

- `rime_dict_manager` 新增 `-p|--purge <詞庫名>`：硬刪除 userdb 中 commits 為負值的墓碑詞條，並自動重建 sync 快照以免下次同步時被自己的舊快照合併回來；官方語義下墓碑永遠無法清除，過往只能以「編輯快照＋刪除 LevelDB 重建」繞過（2baa8b45）

### 行為變更（同步合併語義）

- `UserDbMerger::Put` 逐詞條保留事件 tick，不再整批蓋成 `max_tick`；庫級 tick 經 `CloseMerge` 取 max 做 Lamport 合流，同步過的裝置之間事件 tick 因此可比（1fa1ee09）
- commits 正負號改由事件較新的一方決定：較新的刪除會跨裝置傳播、較新的使用會復活；tick 平手時刪除勝作保底，絕對值取兩者較大以保留使用量歷史供復活延續（1fa1ee09）
- dee 一致衰減至該詞條的合併基準，對查詢權重是資訊保持變換，詞頻排序無回歸（1fa1ee09）
- 舊語義為「絕對值大者勝、平手本地勝」，刪除永不跨裝置傳播；新語義僅在自建版之間完整生效，與官方版混用時退化為近似舊行為
- 新增 5 項 `RimeUserDbMergeTest`（記帳等價、刪除傳播、復活、平手保底、快照採納）
