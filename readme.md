
# ウマウマクルーズ

このソフトは、自動でDMM版ウマ娘のウィンドウから、<br>
イベント選択肢の効果を知るために作られました

---

 **[❮ 使い方 ❯][使い方]**
 **[❮ Build ❯][Build]**
 **[❮ 更新履歴 ❯][更新履歴]**

---

![Preview 1]

![Preview 5]

![Preview 2]

---

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
 
何かあれば[メールフォーム]か、githubの[issues]にお願いします。  


## ■使用ライブラリ・素材

### Library

- **[OpenCV]**  

- **[Tesseract OCR]**  

- **[SimString]**

- **[Boost C++ Libraries][Boost]**

- **[JSON for Modern C++][JSON]**

- **[WTL]**

- **[Win32 Darkmode]**

### Icon

- **[Icon HoiHoi]**

## ■既知のバグ

- windows10以前のOSかつ高DPI環境だと動作しない  
実行ファイルのプロパティから 互換性->高DPI設定の変更  
高いDPIスケールの動作を上書きにチェック、アプリケーションを選択してください

- 画面取り込みに"Windows Graphics Capture"を設定した場合、ウマ娘のウィンドウに黄色く枠が現れる  
仕様です

## ■イベント選択肢データ(UmaMusumeLibrary.json)について

[Gamerch]様運営の"ウマ娘攻略wiki"からイベントデータを加工して、自動生成しています  

## ■著作権表示
Copyright (C) 2021 amate

一連のソースコードを、個人的な利用以外に使用することを禁止します

## ■開発支援
https://www.kiigo.jp/disp/CSfGoodsPage_001.jsp?GOODS_NO=9



<!----------------------------------------------------------------------------->

[更新履歴]: Documentation/Changelog.md
[Build]: Documentation/Build.md
[使い方]: Documentation/Usage.md
[Issues]: https://github.com/amate/UmaUmaCruise/issues


[メールフォーム]: https://ws.formzu.net/fgen/S37403840/
[Gamerch]: https://gamerch.com/umamusume/


[Preview 1]: https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss1.png
[Preview 2]: https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss2.png
[Preview 5]: https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss5.png

[OpenCV]: https://opencv.org/
[Tesseract OCR]: https://github.com/tesseract-ocr/tesseract
[SimString]: http://www.chokkan.org/software/simstring/index.html.ja
[Boost]: https://www.boost.org/
[JSON]: https://github.com/nlohmann/json
[WTL]: https://sourceforge.net/projects/wtl/
[Win32 Darkmode]: https://github.com/ysc3839/win32-darkmode

[Icon HoiHoi]: http://iconhoihoi.oops.jp/

