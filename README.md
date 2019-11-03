# 概要
wavからDL頒布用の音声ファイルをエンコードする為のWindows用補助ツールです。

マスター音源を指定のディレクトリに出力して実行するだけで、勝手に他のコーデックにエンコードしてくれます。

# 必要環境
* Lame (mp3)
* qaac (m4a)
    - Apple Application Supportをインストールして下さい

# 準備
1. LameのバイナリをEncodeUtility.exeと同じ場所に置きます。
2. qaacのバイナリをEncodeUtility.exeと同じ場所に置きます。

# 使い方
1. 出力フォルダに書き出し先の空のフォルダを指定します
2. エンコードしたいwavファイルをD&Dします。
3. ジャケの画像をD&Dします (任意)
4. いい感じにアーティスト名とかを埋めます (任意)
5. Encodeボタンを押すと出来上がり

※ファイル名が "TrackNo_MusicTitle_Artist_AlbumTitle_Genre.wav"みたいに"_"で区切られてた名前だと勝手に内容を埋めてくれます。

ex.) 1_NiceMusic_Koutyan_AwsomeAlbum_SoundTrack.wav

# その他
* メタデータはiTunes準拠になっています。これを使う人はどうせiTunes入れてるだろうと思っています。
* 設定にはffmpegが使えそうな項目がありますが未対応です。
