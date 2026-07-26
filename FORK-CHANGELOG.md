# Fork 變更日誌

本檔記錄本 fork（Wujidadi/librime）相對上游 rime/librime 的所有變動，依 fork 版號分節。
上游自身的變更見 `CHANGELOG.md`。

## 1.17.0-wujidadi — 2026-07-26

基於上游 master d4c324ca（1.17.0 之後的開發版）。首個 fork 版。

### 版號

- fork 版號標記 `1.17.0-wujidadi`：僅改 `RIME_VERSION` 顯示字串（rime_api、installation.yaml、userdb 中繼資料），dylib 的 `-current_version` 維持純數字 `1.17.0`（連結器限制）；藉此可直接分辨機器上跑的是官方或 fork 的 librime

### 新功能

- `rime_dict_manager` 新增 `-p|--purge <詞庫名>`：硬刪除 userdb 中 commits 為負值的墓碑詞條，並自動重建 sync 快照以免下次同步時被自己的舊快照合併回來；官方語義下墓碑永遠無法清除，過往只能以「編輯快照＋刪除 LevelDB 重建」繞過（2baa8b45）

### 行為變更（同步合併語義）

- `UserDbMerger::Put` 逐詞條保留事件 tick，不再整批蓋成 `max_tick`；庫級 tick 經 `CloseMerge` 取 max 做 Lamport 合流，同步過的裝置之間事件 tick 因此可比（1fa1ee09）
- commits 正負號改由事件較新的一方決定：較新的刪除會跨裝置傳播、較新的使用會復活；tick 平手時刪除勝作保底，絕對值取兩者較大以保留使用量歷史供復活延續（1fa1ee09）
- dee 一致衰減至該詞條的合併基準，對查詢權重是資訊保持變換，詞頻排序無回歸（1fa1ee09）
- 舊語義為「絕對值大者勝、平手本地勝」，刪除永不跨裝置傳播；新語義僅在自建版之間完整生效，與官方版混用時退化為近似舊行為
- 新增 5 項 `RimeUserDbMergeTest`（記帳等價、刪除傳播、復活、平手保底、快照採納）
