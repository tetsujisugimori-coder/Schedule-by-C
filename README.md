# Schedule-by-C

C言語とWin32 APIだけで作る、学習用の月間カレンダーGUIです。将来的にはカレンダーに予定を入力するアプリへ拡張しますが、v0.1では日付計算・自前描画・クリック選択に絞っています。

## 構成

- `src/calendar.c` / `src/calendar.h`: うるう年、月の日数、曜日、前月・翌月の計算
- `src/main.c`: Win32ウィンドウ、GDI描画、クリック処理
- `tests/calendar_tests.c`: 日付計算の確認用テスト

## ビルド方法

Visual Studioの「Developer PowerShell」など、CMakeとCコンパイラが使える環境で実行します。

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

MSVCだけで直接ビルドする場合は、次のようにも実行できます。

```powershell
cl /TC /W4 /utf-8 /DUNICODE /D_UNICODE src\main.c src\calendar.c /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib /OUT:ScheduleByC.exe
```

## 起動方法

```powershell
.\build\Debug\ScheduleByC.exe
```

Visual Studioの種類によって出力先が異なる場合は、生成された `ScheduleByC.exe` を起動してください。

## 操作

- タイトル左右の `<` と `>` をクリック: 前月・翌月へ移動
- 日付セルをクリック: 選択日を変更
- ウィンドウのサイズを変更: 次の再描画時にセル領域とクリック領域を再計算

月を切り替えると選択日も表示月へ移し、存在しない日付は月末日に調整します。たとえば31日を選んだ状態で4月へ移動すると、選択日は4月30日になります。

## v0.1で扱わないもの

予定入力、作業ログ、備考、保存、外部サービス連携、通知、繰り返し予定は未実装です。
