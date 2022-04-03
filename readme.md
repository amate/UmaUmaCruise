
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
Copyright (C) 2021 amate

一連のソースコードを、個人的な利用以外に使用することを禁止します

## ■開発支援
https://www.kiigo.jp/disp/CSfGoodsPage_001.jsp?GOODS_NO=9



<!----------------------------------------------------------------------------->

[更新履歴]: Documentation/Changelog.md