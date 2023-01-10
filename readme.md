
# ウマウマクルーズ

![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss1.png)
![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss5.png)

![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss2.png)

## ■はじめに
このソフトは、自動でDMM版ウマ娘のウィンドウから、イベント選択肢の効果を知るために作られました

## ■動作環境
・Windows10 home 64bit バージョン 20H2  
※64bit版でしか動作しません

※注意  
UmaUmaCruise.exeが置かれるフォルダまでのパスにunicodeが含まれていると強制終了します  
例:  
 C:\🐎🐎Cruise\UmaUmaCruise.exe  
->unicode(絵文字🐎が含まれるので)ダメ  
 C:\UmaUmaCruise\UmaUmaCruise.exe  
-> OK!  
日本語なら多分大丈夫ですが、難しい漢字が含まれていると強制終了する可能性があります

## ■使い方
起動した後に、[スタート]ボタンを押せば、自動的にDMM版ウマ娘のウィンドウを探し出し、イベント画面ならば選択肢の効果を表示します

スタート実行後であれば、育成ウマ娘選択画面から自動的に、育成ウマ娘が選択されます  
予め、コンボボックスに育成ウマ娘が選択されていないと、育成ウマ娘のイベントが表示されないので注意  

![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss_ikusei_select_macikane.png)

スタートを押したときに[ウマ娘詳細]が表示されていれば、自動的に育成ウマ娘が選択されます

![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss3.png)

ウマ娘のウィンドウが表示されているのに、タイトルバーに"ウマ娘のウィンドウが見つかりません…"と表示される場合は、UmaUmaCruise.exeを管理者権限で起動させてみてください

### ・UmaMusumeLibrary.jsonについて

UmaLibraryフォルダ内の"UmaMusumeLibrary.json"には、イベント名と選択肢の名前/効果が記録されています  
キャラやサポートの追加によって選択肢が表示されない時には、[設定]->[UmaMusumeLibrary.jsonの更新確認]を実行してみてください


### ・URA予想機能について

>短距離: 1/4

と表示されるとき、  
"1"は実際にその距離で出場した回数  
"4"は現在の日付以降に予約されているレースの回数  
となります

### ・レース予約機能について

レースの予約は育成ウマ娘毎に保存されます  
予め育成ウマ娘を設定していないと保存されないので注意

## ■アンインストールの方法
レジストリも何もいじっていないので、UmaUmaCruiseフォルダ事削除すれば終わりです

## ■イベント誤認識/無認識の報告
もしイベントの誤認識を報告される場合は、本体の"スクリーンショット"機能を使って保存される画像を送ってもらえると助かります（できれば選択肢も写っているシーンで）  
スクリーンショットは"UmaUmaCruise/screenshot" フォルダに保存されます

<img src="https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss4.png" width=50%>

## ■免責
作者は、このソフトによって生じた如何なる損害にも、  
修正や更新も、責任を負わないこととします。  
使用にあたっては、自己責任でお願いします。  
 
何かあれば[メールフォーム](https://ws.formzu.net/fgen/S37403840/)か、githubの[issues](https://github.com/amate/UmaUmaCruise/issues)にお願いします。  


## ■使用ライブラリ・素材

library

- opencv  
https://opencv.org/

- Tesseract OCR  
https://github.com/tesseract-ocr/tesseract

- SimString  
http://www.chokkan.org/software/simstring/index.html.ja

- Boost C++ Libraries  
https://www.boost.org/

- JSON for Modern C++  
https://github.com/nlohmann/json

- WTL  
https://sourceforge.net/projects/wtl/

- win32-darkmode  
https://github.com/ysc3839/win32-darkmode

icon
- ICON HOIHOI  
http://iconhoihoi.oops.jp/

## ■How to build
実行ファイルの生成には、Visual Studio 2019が必要です  

ビルドに必要なライブラリは、[vcpkg](https://github.com/microsoft/vcpkg)によって自動的にダウンロード、ライブラリ(.lib)の生成が行われるようになっています

vcpkgがVisual Studio 2019の英語の言語パックを要求するので、Visual Studioのインストール時に追加しておいてください  

https://github.com/microsoft/vcpkg/archive/refs/tags/2021.04.30.zip  
まず上記URLから"vcpkg-2021.04.30.zip"をダウンロードして、適当なフォルダに解凍します

解凍したフォルダ内にある "bootstrap-vcpkg.bat" を実行すれば、同じフォルダ内に "vcpkg.exe"が生成されます

コマンドプロンプトを管理者権限で起動し

>cd vcpkg.exeのあるフォルダまでのパス

を入力しEnter、コマンドプロンプトのカレントディレクトリを変更します

次に
>vcpkg integrate install

と入力しEnter

>Applied user-wide integration for this vcpkg root.

と表示されたら成功です

Visual Studio 2019で "UmaCruise.sln"を開き、  
デバッグ->デバッグの開始 を実行すれば、vcpkgがライブラリの準備をした後、実行ファイルが生成されます

実行ファイルを動作させるために、事前に "tessdata"と"UmaLibrary"フォルダを"x64\Debug"にコピーしてください


## ■既知のバグ

- windows10以前のOSかつ高DPI環境だと動作しない  
実行ファイルのプロパティから 互換性->高DPI設定の変更  
高いDPIスケールの動作を上書きにチェック、アプリケーションを選択してください

- 画面取り込みに"Windows Graphics Capture"を設定した場合、ウマ娘のウィンドウに黄色く枠が現れる  
仕様です

## ■イベント選択肢データ(UmaMusumeLibrary.json)について

Gamerch様運営の"ウマ娘攻略wiki"からイベントデータを加工して、自動生成しています  
URL:https://gamerch.com/umamusume/

## ■著作権表示
Copyright (C) 2021-2022 amate

一連のソースコードを、個人的な利用以外に使用することを禁止します

## ■開発支援
https://www.kiigo.jp/disp/CSfGoodsPage_001.jsp?GOODS_NO=9

## ■更新履歴

<pre>

============================================
・[update]UmaMusumeLibrary.json更新　2023-01-11
- キャラ
【スター・ライト・シップ】ホッコータルマエ

============================================
・[update]UmaMusumeLibrary.json更新　2022-12-30
- キャラ
【真打・慶鶴之志】キタサンブラック
【花形・弥栄之翠】サトノダイヤモンド
- サポート
［ハネ退け魔を退け願い込め］スペシャルウィーク
［気まぐれ絶佳］スイープトウショウ
［遥か流るる冬夏青青］グラスワンダー

============================================
・[update]UmaMusumeLibrary.json更新　2022-12-21
- キャラ
【餓狼】ナリタブライアン
- サポート
［誰も知らない風へ］ヤマニンゼファー
［偉大なるフォトグラフィーア］テイエムオペラオー


v1.21
・[fix] 育成ウマ娘名の自動認識が機能していなかったのを修正 #116

============================================
・[update]UmaMusumeLibrary.json更新　2022-12-14
- キャラ
【Heroic Author】ゼンノロブロイ

[fix] イベント修正  #116 #117 #118 #119

============================================
・[update]UmaMusumeLibrary.json更新　2022-11-29
- キャラ
【不凍のアクア・ウィエタ】ウオッカ
【緋色のニュイ・エトワレ】ダイワスカーレット
- サポート
［Are you merry？］エアグルーヴ
［ここからはDon’t stop！］サクラローレル
［GLAD TIDINGS］ナリタブライアン

============================================
・[update]UmaMusumeLibrary.json更新　2022-11-19
- キャラ
【Butterfly Sting】ワンダーアキュート
- サポート
［Danke schön］エイシンフラッシュ
［ストップ、委員長！］ナリタトップロード

その他、選択肢効果の抜けている物を追加

・[change] ConvertUmaMusumeLibrary.py で、UpdateEventのイベント名は部分一致でも置換に変更

============================================
v1.20
・[add] 川崎/船橋/盛岡レース場を追加
・[add] SkillLibrary.jsonにスキル追加
・[add] UmaMusumeLibraryMainStory.json にグランドライブのイベントを追加
・[add] ConvertUmaMusumeLibrary.py - 大成功 / 成功 (/ 失敗) のみの選択肢をイベントリストから削除する処理を追加
・[update]UmaMusumeLibrary.json更新　2022-11-14

============================================
・[update]UmaMusumeLibrary.json更新　2022-06-21
- キャラ
【プラタナス・ウィッチ】スイープトウショウ
- サポート
［Hands up,crook!］タイキシャトル
［好きなんだ、ご飯が］オグリキャップ

============================================
・[update]UmaMusumeLibrary.json更新　2022-06-12
- キャラ
【稲荷所縁江戸紫】イナリワン

============================================
・[update]UmaMusumeLibrary.json更新
ウォッカのイベント"究極の2択"が抜けていたのを修正 #111

============================================
・[update]UmaMusumeLibrary.json更新 2022-06-01
- キャラ
【Titania】ファインモーション
【朔日のマ・シェリ】カレンチャン
- サポート
［あこがれの景色］ライスシャワー
［うららか・ぱっしょん♪］ハルウララ
［愛すべき金蘭のつどい］イクノディクタス

============================================
・[update]UmaMusumeLibrary.json更新 2022-05-21
- キャラ
【Line Breakthrough】メジロパーマー

============================================
・[update] "【ティアード・ペタル】ニシノフラワー"の抜けイベント追加
・[add] ConvertUmaMusumeLibrary.pyで、出力するjsonにキャラ名やサポート名でソートする処理を追加 (これからの差分が見やすくなるかもしれない)

============================================
・[update]UmaMusumeLibrary.json更新 2022-05-11
- キャラ
【オールタイム・フィーバー】アイネスフウジン
- サポート
［Dear Mr. C.B.］ミスターシービー
［Step! Smile! Wink!］トウカイテイオー

============================================
・[update]UmaMusumeLibrary.json更新 2022-04-29
- キャラ
【RUN&WIN】ナイスネイチャ
【白く気高き激励の装】キングヘイロー
- サポート
［フレフレ☆プリンセス！］カワカミプリンセス
［届け、このオモイ！］バンブーメモリー
［そのエールは世界を変えた］シーキングザパール

============================================
・[update]UmaMusumeLibrary.json更新 2022-04-20
- キャラ
【四白流星の襲】ヤエノムテキ

============================================
・[update]UmaMusumeLibrary.json更新 2022-04-12
- キャラ
【ティアード・ペタル】ニシノフラワー
- サポート
［Outfit as No.1］ダイワスカーレット
［おてんば魔女、修行中。］スイープトウショウ
［ギリギリ!?UMA♡DOLL］スマートファルコン

============================================
・[update]SkillLibrary.json更新
"二の矢"の効果内容変更
"前のめり"のスキル追加

・[update]UmaMusumeLibrary.json更新
【キセキの白星】オグリキャップ に "オグリ、決意する"のイベント追加

============================================
・[update]UmaMusumeLibrary.json更新 2022-03-30
- キャラ
【ソワレ・ド・シャトン】セイウンスカイ
【シュクセ・エトワーレ】フジキセキ
- サポート
［Escorte etoile］シリウスシンボリ
［Enchainement］シンボリルドルフ
［mag!c number］エアシャカール

============================================
・[update]UmaMusumeLibrary.json更新 2022-03-20
- キャラ
【ブリュニサージュ・ライン】メジロブライト
- サポート
［紡がれてゆく想い］チーム＜シリウス＞

============================================
・[update]UmaMusumeLibrary.json更新 2022-03-17
アップデートされたイベント効果を更新

============================================
・[update]UmaMusumeLibrary.json更新 2022-03-09
- キャラ
【Natural Brilliance】サトノダイヤモンド

- サポート
［世界にもっと☆マーベラス］マーベラスサンデー
［ほっと♪きゅーとメモリー］カレンチャン

v1.19
・[add] 育成ウマ娘をコンボボックスから検索できるようにした
・[add] 開催レース一覧で、"URA/アオハル"と"MNT"に予約レース表示の切り替えをできるようにした
・[update] "UmaMusumeLibraryMainStory.json" に、"アオハル杯" / "Make a new track!!" のイベントを追加
・[update] UmaMusumeLibrary.jsonの生成に関するPull requestをマージ #98

============================================
・[update]UmaMusumeLibrary.json更新 2022-02-25
- キャラ
☆3
【錦上・大判神輿】キタサンブラック
☆2
【ぱんぱかティルトット】マチカネタンホイザ

- サポート
SSR
［桃色のバックショット］ナリタトップロード
［リトル・バイ・リトル］メジロブライト
［SPECIAL DREAMERS］サトノダイヤモンド
SR
［響け！模範的あいさつ！］サクラバクシンオー
［鼓動速まるクールダウン］メジロライアン
［1日の終わりに］スペシャルウィーク
［一等星を目指して］アドマイヤベガ
R
［トレセン学園］ナリタトップロード


============================================
・[update]UmaMusumeLibrary.json更新 2022-02-17
- キャラ
【Starry Nocturne】アドマイヤベガ

============================================
・[update]UmaMusumeLibrary.json更新 2022-02-09
- キャラ
【クリノクロア・ライン】メジロアルダン
- サポート
［そこに“いる”幸せ］アグネスデジタル
［頑張るキミに！］アイネスフウジン

============================================
・[update]UmaMusumeLibrary.json更新 2022-01-30
- キャラ
【CODE:グラサージュ】ミホノブルボン
【コレクト・ショコラティエ】エイシンフラッシュ
- サポート
［小さなカップに想いをこめて］ニシノフラワー
［爆速！最速！花あらし！］サクラバクシンオー
［In my way］トーセンジョーダン

・[change] ConvertUmaMusumeLibrary.pyで UpdateEventはキャラ名にマッチする複数キャラのイベントを更新するようにした

============================================

・[update]UmaMusumeLibrary.json更新 2022-01-26
- キャラ
【ツイステッド・ライン】メジロドーベル
抜けているイベントを追加

============================================
・[update]UmaMusumeLibrary.json更新 2022-01-21
- キャラ
【日下開山・花あかり】サクラチヨノオー

============================================
・[update]UmaMusumeLibrary.json更新 2022-01-14
- サポート
［ブスッといっとく？］安心沢刺々美
［これがウチらのいか焼きや！］タマモクロス
［笹針師］安心沢刺々美

============================================
・[update]UmaMusumeLibrary.json更新 2022-01-01
- キャラ
【初晴・青き絢爛】テイエムオペラオー
【初うらら♪さくさくら】ハルウララ
- サポート
［袖振り合えば福となる♪］マチカネフクキタル
［夜に暁、空に瑞星］アドマイヤベガ
［飛びも出でぬべき心地すれ］メイショウドトウ
============================================

v1.18
・[fix] windows7で起動しない問題をたぶん修正 (WindowsGraphicsCapture関連の処理を WindowsGraphicsCapture.dllに移動させて、動的にロードするようにした)
・[change] ウマ娘ウィンドウが横画面の時は、TextRecognizerを実行しないようにした (CPU負荷対策)
・[fix] 選択肢の候補が複数ある時、より類似度の高い方を選ぶようにした #90
・[fix] イベント名末尾にある"①"や"（イベント進行①）"などの文字列を取り除いた (選択肢検出の精度を上げるため)
・[fix] 画面取り込み方式に Windows Graphics Captureを選択した時に、画面取り込み実行時に古いフレームを取り込んでしまうのを修正

・[update]UmaMusumeLibrary.json更新 2021-12-23
- キャラ
【疾風迅雷】タマモクロス
- サポート
［まだ見ぬ景色を求めて］サイレンススズカ

・[update]UmaMusumeLibrary.json更新 2021-12-16
- キャラ
【Noble Seamair】ファインモーション
- サポート
［独奏・螺旋追走曲］マンハッタンカフェ
［泥濘一笑、真っつぐに］イナリワン

・[update] SkillLibrary.jsonに「真っ向勝負」を追加


・[update]UmaMusumeLibrary.json更新 2021-11-30
- キャラ
【キセキの白星】オグリキャップ
【ノエルージュ・キャロル】ビワハヤヒデ
- サポート
［フォーメーション：PARTY］マヤノトップガン
［いじっぱりのマルクト］ナリタタイシン
［スノウクリスタル・デイ］マーベラスサンデー

・[fix] 表示されるイベント名と選択肢の内容が異なるバグを修正
・[update] 開発環境をVisual Studio 2019 から Visual Studio 2022 へ変更

・[update]UmaMusumeLibrary.json更新 2021-11-20
- キャラ
【ツイステッド・ライン】メジロドーベル
- サポート
[おどれ・さわげ・フェスれ！]ダイタクヘリオス
［うるさい監視役］ウオッカ

・[update]UmaMusumeLibrary.json更新 2021-11-09
- キャラ
【ポップス☆ジョーカー】トーセンジョーダン

v1.17
・[fix] 設定でスリーショット保存先のフォルダ選択ダイアログを開くと応答停止状態になるバグを修正 #85 
・[fix] イベント名の認識に利用する画像にゴミが混ざり認識できないことがあるのを修正 #86 


・[update]UmaMusumeLibrary.json更新 2021-10-28
- キャラ
【皓月の弓取り】シンボリルドルフ
【秋桜ダンツァトリーチェ】ゴールドシチー
- サポート
［かなし君、うつくし君］カレンチャン
［天嗤う鏑矢］ナリタブライアン
［宵照らす奉納舞］ユキノビジン

v1.16
・[add] 画面取り込みに Windows Graphics Capture APIを使ったものを追加
・[change] 画面取り込み方式のデフォルトをGDIに変更した
・[fix] スタートとストップを繰り返すとメモリリークするバグを修正

・[update]UmaMusumeLibrary.json更新 2021-10-23
- キャラ
【Creeping Black】マンハッタンカフェ
- サポート
［43、8、1］ナカヤマフェスタ
［一等星は揺らがない］シリウスシンボリ

----
・[update]UmaMusumeLibrary.json更新 2021-10-12
- キャラ
【プリンセス・オブ・ピンク】カワカミプリンセス

・[update]UmaMusumeLibrary.json更新 2021-09-30
- キャラ
【Make up Vampire!】ライスシャワー
【シフォンリボンマミー】スーパークリーク
- サポート
［やったれハロウィンナイト！］タマモクロス
［魔力授かりし英雄］ゼンノロブロイ
［幽霊さんとハロウィンの魔法］ミホノブルボン

v1.15.3
・[fix] メインとは別のグラボに繋がれたモニターにウマ娘ウィンドウが存在するときに、画面取り込みができないバグを修正 #80

v1.15.2
・[fix] モニターが回転していると正常に画面取り込みできなかったバグを修正
・[fix] セイウンスカイ(サポート)の連続イベントで間違った選択肢が出るのを修正 #77

v1.15.1
・[fix] 高DPI環境で、スタートを押した瞬間ウィンドウの拡大スケーリングが解除される問題を修正

v1.15
・[change] ウマ娘ウィンドウの画面取り込みを GDIを使ったものから Desktop Duplication APIを使ったものに変更
・[change] レース予約の通知を、日付変更時ではなく、育成画面になったときに行うように変更 #78
・[fix] CurrentMenuBoundsで切り取りイメージをグレー化してからTextFromImageするように変更

・[update]UmaMusumeLibrary.json更新
- キャラ
【超特急！フルカラー特殊PP】アグネスデジタル
- サポート
[心と足元は温かく]イクノディクタス
[GRMAラーメン♪]ファインモーション

・[update] UmaMusumeLibrary.json更新
- キャラ
【ボーノ☆アラモーダ】ヒシアケボノ

v1.14
・[fix] アオハル杯では、現在の日付が取得できなかったのを修正
・[add] レース一覧に、予約レースまでのターン数表示を追加 #74
・[fix] フジキセキの二つ名のtypo修正 #75
・[update] UmaMusumeLibrary.json更新
- キャラ
【吉兆・初あらし】マチカネフクキタル
- サポート
［徹底管理主義］樫本理子
［幸せは曲がり角の向こう］ライスシャワー


・[update] UmaMusumeLibrary.json更新
- サポート
［WINNING DREAM］サイレンススズカ

・[update] UmaMusumeLibrary.json更新
- キャラ
【Meisterschaft】エイシンフラッシュ
- サポート
［願いまでは拭わない］ナイスネイチャ
［響き合うストレイン］ビワハヤヒデ
［nail on Turf］トーセンジョーダン

・[update] UmaMusumeLibrary.json更新
- キャラ
【ブルー/レイジング】メイショウドトウ

v1.13
・[fix] AmbiguousSearchEventで、イベント名と下部選択肢の類似度を比較して高い方のイベントを返すように変更 #66
・[add] スタート押したときに一度だけ能力詳細からウマ娘名を取得するようにした #70
・[fix] Releaseビルドが通らなかったのを修正
・[update] UmaMusumeLibrary.json更新

・[update] UmaMusumeLibrary.json更新
- キャラ
【オーセンティック/1928】ゴールドシチー
- サポート
［Two Pieces］ナリタブライアン
［集まってコンステレーション］ツインターボ

・[add] BuildUmaMusumeLibrary.py エラー発生時に中断する処理を追加
・[change] BuildUmaMusumeLibrary.py DeleteSuccessFailedOnlyとNomarizeEventSuccessFailedをAddCharactorEventとUpdateEventの後に移動させた
・[update] SkillLibrary.json更新
・[update] UmaMusumeLibrary.jsonの更新
- キャラ
【シューティンスタァ・ルヴゥ】フジキセキ
- サポート
［爽快!ウィニングショット!］メジロライアン
［その心に吹きすさぶ］メジロアルダン

v1.12
・[add] 設定からスクリーンショットの保存フォルダを設定できるようになった #65 (PullRequest thx!)
・[fix] スクリーンショットフォルダの設定の保存・復元時に、文字コードの変換を行っていなかったのを修正
・[change] スクリーンショットフォルダの選択ダイアログを、CShellFileOpenDialogを使ったものに変更
・[add] スクリーンショットボタン右クリックで、設定で指定したフォルダを開くようにした
・[del] TextRecognizerで、"育成ウマ娘名[能力詳細]"の文字認識を削除(誤爆するので)
・[del] Common.jsonから kUmaMusumeSubNameBounds、kUmaMusumeNameBoundsを削除
・[add]［祝福はフーガ］ミホノブルボン(ウマ箱2のサポートカード)のイベントを追加
・[update] UmaMusumeLibrary.jsonの更新

v1.11
・[add] 育成ウマ娘選択画面から、育成ウマ娘を取得するようにした (手動設定を不要に)
・[add] "スクリーンショット"ボタンを右クリックで、screenshotフォルダを開くようにした
・[add] 設定に、"メインウィンドウを最前面表示する"オプションを追加
・[add] 選択肢にイベント打ち切りがある場合、選択肢効果に表示するようにした #51
・[add] レース一覧のチェック状態を、育成ウマ娘毎に保存/復元するようにした
・[add] about画面のdebugに、IkuseiUmaMusumeSubNameBoundsとIkuseiUmaMusumeNameBoundsを追加
・[add] 設定で、"UmaMusumeLibrary.jsonの更新確認"を実行時にキャッシュを無視するようにした
・[add] 設定で、"UmaMusumeLibrary.jsonの更新確認"を実行した後に、再起動を不要にした
・[add] AnbigiousChangeIkuseImaMusumeで、育成ウマ娘名検索時の最小閾値をCommon.jsonの"UmaMusumeNameMinThreshold"から引っ張ってくるようにした(ウマ娘ウィンドウが小さい場合引っかからないので、検索閾値を引き下げた)
・[fix] TextRecognizerで、最下部のイベント選択肢名の取得時に、白背景化を行っていなかったのを修正 (スーパークリークの初詣イベントが出てこなかったのを修正)
・[fix] 旧バージョン(v1.9以下)用に、"https://raw.githubusercontent.com/amate/UmaUmaCruise/master/UmaLibrary/UmaMusumeLibrary.json"は4択選択肢を切り詰めたバージョンにし、
新バージョンは4択選択肢がある "https://raw.githubusercontent.com/amate/UmaUmaCruise/master/UmaLibrary/UmaMusumeLibrary_v2.json"から更新データを取得するようにした

v1.10
・[add] 選択肢で取得できるスキル効果を確認できるようになった
・[fix] 選択肢効果をポップアップ中にイベントが更新されると、ポップアップが消えた時に前の選択肢効果のテキストに巻き戻るバグを修正 #60
・[add] 選択肢効果のポップアップに背景色を付けた
・[fix] 金鯱賞の開催地が間違っていたのを修正 #50
・[fix] レース一覧のリストビューで、ダートがデフォルト状態だと見切れていたのを修正

v1.9
・[add] UmaMusumeLibrary.jsonに、追加キャラとウマ箱のサポートカードのイベントを追加
・[add] 4択選択肢に対応
・[add] 選択肢効果のテキストががエディットボックスをはみ出すときに、テキストを一度に確認できるようマウスオーバーでテキストをポップアップ表示させるようにした
・[add] 選択肢効果のステータス上昇下降に色をつけてわかりやすくした #6
・[fix] URA予想で出場距離を間違うことがあるのを修正
・[fix] 手動でダークモードに設定したとき、レース一覧のリストビューの背景が白いままだったのを修正
・[fix] BuildUmaMusumeLibrary.pyを最新のwikiに対応させた
・[fix] メジロライアンの"初詣"イベントを検出できなかったのを修正 #57
・[add] UmaMusumeLibraryURLの取得にキャッシュ回避処理を追加

v1.8
・[add] スマートファルコンのキャライベントを追加
・[fix] その他UmaMusumeLibrary.json更新
・[change] デバッグ時は、about画面での更新チェックを止めた
・[fix] about画面でのdebugで、HSVBoundsを読み込むようにした
・[add] 設定からテーマを選択できるようにした
・[fix] TextRecognizerで、イベント選択肢の候補にthresImageを追加した
・[add] UmaMusumeLibrary.json は、~Origin.json に ~Modify.jsonのパッチを当てて(BuildUmaMusumeLibrary.pyが)生成するようにした(wikiの変更点の確認を容易にするため)


v1.7
・[add] ダークモードに対応 (undocumentな方法を使っているので、将来のアップデートで使えなくなる可能性があります)
・[add] Common.jsonから色テーマを読み込むようにした
・[add] about画面にソフトの更新チェッカーを追加
・[add] "UmaMusumeLibrary.jsonの更新確認"実行時に古い方のjsonを残しておくようにした
・[change] AnbigiousChangeCurrentTurnで日付の検索にregex_matchではなく、regex_searchを使うようにした
・[change] TextRecognizerで、イベント選択肢の検出は文字色抜き出しに変更
・[fix] TextRecognizerで、イベント選択肢の検出前にテキストを囲って切り出すようにした (文字数が少ないと検出されないことがあるため)
・[fix] "サクラバクシンオー"の"レース敗北"の選択肢が上下逆なのを修正 #39
・[fix] "ゴールドシップの"夏合宿(3年目)終了"が抜けていたのを修正 (どっち選んでも一緒だが一応…) #40 
・[add] AUTHORS.txtにライセンステキストを追加
・[add] tessdataフォルダを追加

v1.6
・[add] vcpkg.json を追加 (各種ライブラリの取得/ビルドの自動化)
・[add] .editorconfig を追加、デフォルトのソースコードのエンコーディングをUTF8にした

・[change] readme.mdの How to buildを vcpkgを使ったビルド方法に変更
・[fix] resource.h と WinHTTPWrapper.h をUTF8に変換
・[fix] コンパイラオプションに "/source-charset:utf-8"を追加

・[add] about画面のdebugに、RaceDetailBoundsを追加
・[change]  Common.jsonのWindowNameとClassNameが空の時はFindoWindowにnullptrを渡すようにした #33
・[fix] ゲームから読み取った現在の日付の判定を厳格にした(もう日付の処理は大丈夫なはず)
・[add] レース一覧でshiftキーを押しながらチェックボックスをクリックすることによって、クリックしたチェックボックスが所属するグループのチェックボックスを一括でオン・オフできるようにした
・[change] TextRecognizerで、イベント選択肢のOCRを文字色を検出した時に変更(CPU負荷軽減)
・[change] TextRecognizerで、育成ウマ娘のOCRを文字色を検出した時に変更(CPU負荷軽減)
・[change] TextRecognizerで、現在の日付の検出を文字色抜き出しのみに限定(CPU負担軽減)
・[fix] レース予約完了ダイアログが表示されたときにも、URA予想の出走履歴に追加されてしまっていたのを修正
・[change] 全てのライブラリのCRTをstaticにできたので、実行ファイルにすべてのライブラリをスタティックリンクさせた(同伴させていた~.dllがすべて不要になった)

v1.5
・[add] レース一覧をウィンドウ化するオプションを追加
・[add] レース一覧でレース予約機能を追加 (レースをCtrl+クリック or 右クリックメニューから"レース予約を切り替え")
・[add] 出場レースの距離からURAファイナルズの距離を推定する機能を追加
・[add] 設定に、"現在の日付が予約レースの開催日になったときに、音とウィンドウの振動で通知する"機能の追加
・[add] 設定で、"UmaMusumeLibrary.jsonの更新確認"実行時にエラー発生した時、詳細なログを記録するようにした(info.logに記録されます)
・[add] メインウィンドウのタイトルバーにソフトのバージョンを表記するようにした
・[fix] プレビューウィンドウに画像をドロップした時に、画像ファイルをロックしないようにした
・[fix] 現在の日付が巻き戻ることがあるのを修正
・[change] サポートカードからイベント検索する時の優先度を R->SR->SSR　から SSR->SR->R へ変更
・[change] TextRecognizerで、イベント選択肢は、画像を2倍化するように変更
・[change] TextRecognizerで、育成ウマ娘名は、文字色抜き出しを利用するように変更
・[change] TextRecognizerで、現在の日付は、文字色抜き出しと2倍化を利用するように変更
・[add] TextRecognizerで、レース詳細からレース距離を抜き出す機能を追加
・[add] TextRecognizerで、イベント名とイベント選択肢に'?'が含まれていた場合、'？(全角)'も候補に追加するようにした
・[change] _IsEventNameIconで kBlueBackgroundThresholdの閾値を変更(緩い方へ)
・[change] kMinWhiteTextRatioThresholdの閾値を変更(緩い方へ)
・[change] RaceDataLibrary.json の　"スプリンターズステークス"と"フェブラリーステークス"の表記修正
・[fix] UmaMusumeLibrary.json の "レース入賞"を"レース入着"に変更
・[fix] UmaMusumeLibrary.json でメジロマックイーンの"エキサイティングお嬢様"のイベントが抜けていたのを修正
・[add] About画面でのdebugで、kCurrentTurnBoundsと右Winキー押しながらのOCRでは、文字色抜き出し化を行うようにした

v1.4
・[fix] 古いOSで起動に失敗するバグを多分修正(テストできないので直ったかどうかは分からない) #23
・[fix] 高DPI環境でスクリーンショットの取り込み範囲がずれるのを修正 #20
・[change] About画面でのdebugでのTextFromImageをAsync化した
・[change] 選択肢/効果エディットボックスの更新はイベント名変更時のみとした
・[change] 更新間隔に処理時間も含めるようにした
・[fix] "修正"ボタンで変更した選択肢効果の改行に\r\nが入るのを\nだけが入るようにした
・[add] Async用にTesseractWrapperにGetOCRFunctionを追加(キャッシュ化したTessBaseAPIを返す)
・[change] UmaMusumeLibraryRevision.jsonに入ってた"メインストーリーイベント"をUmaMusumeLibraryMainStory.jsonへ移動させた
(本体更新時UmaMusumeLibraryRevision.jsonの上書き対策)
・[change] TextRecognizerでイベント名関連の処理をAsync化した(CPUに余裕があれば処理速度が倍ほど早くなった)
・[fix] "ウマ娘詳細"画面で、"ナイスネイチャ"が認識されなかったのを修正
・[change] 現在の日付も白背景か確認するようにした
・[fix] グラスワンダー：「譲れないこと」の選択肢が逆なのを修正 #21
・[fix] ナイスネイチャの"初詣"イベントが抜けていたのを修正
・[add] readme.md に How to buildを追加
・[fix] 一部イベントの認識改善

v1.3
・古いCPUでスタートを押すと強制終了するバグを修正(tesseractを適切にビルドするようにした)
・イベント検索で、選択肢のテキストからも検索を行うようにした
・イベント名のアイコン検出で、サポートカードイベントでもアイコン検出を行っていたのを修正
・About画面でのdebugで 左エディットに1倍、右エディットに2倍リサイズ時の結果を表示するようにした
・About画面でのdebugで、kEventNameBoundsの時アイコン検出を行うようにした
・About画面でのdebugで、Altキーを押しながらのOCRは2倍拡大処理をスキップするようにした
・About画面でのdebugで、Direct指定が機能していなかったのを修正
・About画面でのdebugで、kEventNameIconBounds時はOCRではなく、アイコン検出を行うようにした
・～.json系をUmaLibraryフォルダへ移動させた
・ctrlを押しながらスクリーンショットボタンを押した場合、プレビュー画像からIRを行うようにした(デバッグ用)
・自動検出時に OnEventNameChangedが呼ばれていたのを修正(イベント名エディットボックスにフォーカスがある時のみ処理を行うようにした)
・windows7で起動に失敗するのをおそらく修正(環境がないのでテストできない)
・ウマ娘名検出をTestBoundsが白背景の時に限定(CPU負荷対策)
・イベント名の検出を "4 グレースケール反転 + 2倍"に限定(CPU負荷対策)
・イベント名の検出のために、イベント選択肢の検出を追加("5 黒背景白文字(グレー閾値) + 2倍")
・現在の日付検出を cutImage と thresImage に限定(CPU負荷対策)
・イベント名のアイコン検出でcv::thresholdに大津二値化からkEventNameIconThreshold指定のTHRES_BINARYへ変更
・リリースビルドではwarning以上のログのみ出力するようにした
・初回起動時に設定画面でOKを押しても設定が保存されないバグを修正


v1.2
・readme.mdとAbout画面にイベントデータの取得元の出典を追加
・std::ifstreamなどにwstringを渡すようにした
・高DPI環境で動作しないのを修正 #5
・UmaUmaCruise.exeが置かれるフォルダまでのパスにunicodeが含まれていると警告ダイアログを表示するようにした
->readme.mdに詳細を記載
・ptess->Initに tessdataフォルダへのパスを渡すようにした #5
・起動時に::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);を実行するようにした
->高DPI対応
・根岸ステークス/Sの表記ゆれを修正 #1

v1.1
・[add] 設定に"プレビューウィンドウの更新をトレーニング画面で止める"オプションを追加
・[add] レース一覧にレース場のカラムを追加
・[fix] レース一覧更新時にちらつくのを修正
・[fix] 起動時にイベント名の検索が機能していなかったのを修正
・[fix] 特定のイベント名の選択肢が出てこなかったのを修正

v1.0
・公開

</pre>
