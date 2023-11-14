﻿# 概要
wavからDL頒布用の音声ファイルをエンコードする為のWindows用補助ツールです。

マスター音源を指定のディレクトリに出力して実行するだけで、勝手に他のコーデックにエンコードしてくれます。

mp3やm4aに一括で変換してくれるので、音源を差し替えたときもボタン一発で済んで楽です。

作業時間見積もりからパッケージ用意がほぼ消えて、ミックスに専念できるぞ！

![2023-11-15_01h59_29](https://github.com/MUkoutyan/EncodeUtility/assets/6138691/1c0d4efc-df37-4662-8ba1-e9c69c1fab7e)

# 対応コーデック
* AAC
* MP3
* Flac

# 必要環境
* ffmpeg ([https://ffmpeg.org/download.html](https://ffmpeg.org/download.html))

# 準備
1. ffmpegが無い場合、自動でダウンロードするため不要です。

# 使い方
1. 「出力フォルダ」に、エンコードしたファイルの書き出し先となるフォルダを指定します
2. エンコードしたいwavファイルをD&Dします
3. ジャケの画像をD&Dします (任意)
4. いい感じにアーティスト名とかを埋めます (任意)
5. Encodeボタンを押すと出来上がり

# 細かい使い方
## ファイル名からテーブルを埋める

※ファイル名を"_"で区切られた名前にすると、勝手に内容を埋めてくれます。
区切り文字は変更可能です。

ex.) 1_NiceMusic_Kunatsu_AwsomeAlbum_SoundTrack.wav

## テーブル入力
・選択したセルの内どれか1つに文字を入力することで、選択したセルを全て同じ内容で埋めることが出来ます。

## 「エンコードしたオーディオファイルにトラック番号を付ける」
・出力したファイル名の先頭に「区切り文字」「桁数」を元に文字を追加します。
```
　例）　元ファイル名 "Track_01.m4a"
　　　・区切り文字　 : -
　　　・桁数　　　　：2
　　　出力ファイル名 " 1-Track_01.m4a"
```
トラック番号が桁数に満たない場合、スペースが追加されます。


