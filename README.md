# Schedule-by-C

C言語とWin32 APIで作る、学習用の月間カレンダーGUIです。Schedule by Cは予定の**表示・参照**を担当し、予定の入力・編集・削除は行いません。予定入力の正本は、将来「作業ログアプリ」または共通データ層が保持します。

```text
Worklog（作業ログアプリ）
  ↓ 予定を入力・保存
schedule.csv
  ↓ 読み込み専用
Schedule by C
  ↓ 将来連携
Memo Nexus / Google Calendar / Outlook
```

現在はネットワーク通信、Google/Microsoft認証、外部カレンダーへの登録は実装していません。

## 機能

- 7列×6行の月間カレンダー表示
- 前月・翌月への移動と日付選択
- `schedule.csv` の起動時読み込み
- 予定がある日への `*` 表示
- 選択日の予定を開始時刻順で下部領域へ表示
- CSVがない場合や一部の行が不正な場合も、カレンダーを継続表示

## 構成

- `src/main.c`: Win32ウィンドウ、GDI描画、クリック処理、各モジュールの調整
- `src/calendar.c` / `src/calendar.h`: うるう年、月の日数、曜日、年月移動、42セルへの日付割り当て
- `src/schedule.c` / `src/schedule.h`: `Schedule`構造体、日付検索、予定有無判定、開始時刻順の取得
- `src/storage.c` / `src/storage.h`: CSVファイルI/O、CSV解析、値の検証、`Schedule`への変換
- `tests/`: 日付計算、予定検索・並び替え、CSV読み込みのテスト
- `schedule.csv`: 作業ログアプリと共有する予定データのサンプル

カレンダー描画側はCSVの形式を知りません。将来CSVをSQLite、JSON、iCalendarなどへ置き換える場合も、`storage`層で`ScheduleCollection`を生成すれば、`calendar`側を大きく変更せずに済む構造です。

## schedule.csv

ファイルはUTF-8で保存し、先頭行を次のヘッダーにします。

```csv
id,date,start_time,end_time,title,note,status,source
```

| フィールド | 内容 | 例 |
|---|---|---|
| `id` | 一意ID。将来のiCalendar変換でも保持する文字列 | `1` |
| `date` | 日付（`YYYY-MM-DD`） | `2026-08-25` |
| `start_time` | 開始時刻（24時間制`HH:MM`） | `09:00` |
| `end_time` | 終了時刻（24時間制`HH:MM`） | `10:00` |
| `title` | 予定名 | `Power BI作業` |
| `note` | 備考。空でも可 | `月次資料作成` |
| `status` | 状態。現在は表示動作を変えず値だけ保持 | `planned` |
| `source` | 作成元。表示動作を変えず値だけ保持 | `worklog` |

想定する`source`は`worklog`、`manual`、`memo-nexus`、`google`、`outlook`です。将来値を増やせるよう、未知の`status`と`source`も有効な文字列として読み込みます。

### サンプル

```csv
id,date,start_time,end_time,title,note,status,source
1,2026-08-25,09:00,10:00,Power BI作業,月次資料作成,planned,worklog
2,2026-08-25,13:00,14:30,Memo Nexus修正,保存処理確認,planned,worklog
3,2026-08-26,18:00,19:00,C言語学習,Calendar module確認,planned,worklog
```

### CSVの対応範囲と制約

- カンマを含む値は`"Title, with comma"`のように二重引用符で囲めます。
- 値内の二重引用符は`"Quoted ""text"""`のように`""`で表します。
- 1レコードは1行です。引用符で囲んだ値であっても、フィールド内改行には対応しません。
- 空行は無視します。不完全な行、不正な日付・時刻、長すぎる行は、その行だけをスキップします。
- 最大件数は1,024件です。各最大長（UTF-8のバイト数）は、ID 64、時刻 5、タイトル128、備考256、状態32、作成元32です。
- `id`、日付、開始・終了時刻、タイトル、状態、作成元は必須です。備考だけは空にできます。

## データファイルの場所

CMakeビルドでは、サンプルの`schedule.csv`を実行ファイルと同じフォルダーへコピーします。アプリは実行ファイルと同じフォルダーの`schedule.csv`を起動時に一度読み込みます。

作業ログアプリと別の共通ファイルを使う場合は、環境変数`SCHEDULE_BY_C_DATA_FILE`に絶対パスを指定できます。

```powershell
$env:SCHEDULE_BY_C_DATA_FILE = 'C:\shared\schedule.csv'
.\build\ScheduleByC.exe
```

ファイルがない場合は「予定データなし」と表示し、通常のカレンダーとして動作します。

## ビルドと実行

Visual StudioのDeveloper PowerShell、またはMinGWなど、CMakeとCコンパイラが使える環境で実行します。

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

代表的な出力先は次のいずれかです。

```powershell
.\build\Debug\ScheduleByC.exe
.\build\ScheduleByC.exe
```

MSVCで直接ビルドする場合:

```powershell
cl /TC /W4 /utf-8 /DUNICODE /D_UNICODE `
  src\main.c src\calendar.c src\schedule.c src\storage.c `
  /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib /OUT:ScheduleByC.exe
```

## 操作

- タイトル左右の`<`と`>`をクリック: 前月・翌月へ移動
- 日付セルをクリック: 選択日を変更し、その日の予定を下部へ表示
- `*`が付いた日: 予定あり
- 月初前・月末後の空白セルをクリック: 選択日は変更しない
- ウィンドウを縦に広げる: 下部に表示できる予定件数が増える

月を切り替えると選択日も表示月へ移し、存在しない日付は月末日に調整します。たとえば31日を選んだ状態で4月へ移動すると、選択日は4月30日になります。

## テスト

```powershell
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

日付検索（0件・1件・複数件）、開始時刻順、正常CSV、ファイルなし、空ファイル、不正行、ヘッダーのみ、引用符付き値、月初・月末、うるう年、年跨ぎを確認します。

## 将来の連携

作業ログアプリ側では、入力値を上記の共通スキーマに変換し、安全に`schedule.csv`へ書き出す処理が必要です。特に一意IDの生成、UTF-8 CSVの引用符処理、一時ファイルからの置換による安全な保存、同時書き込み制御を作業ログ側で担当します。

その後は`Schedule`のID、日付、開始・終了時刻、タイトル、備考を保ったまま、Memo Nexus、iCalendar（`.ics`）、Google Calendar、Outlook向けの変換層を追加できます。

## 現在扱わないもの

Schedule by Cからの予定入力・編集・削除、作業実績入力、SQLite、GUIの全面変更、通知、繰り返し予定、外部API通信は今回の対象外です。
