
# ウマウマクルーズ

![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss1.png)

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

予め、コンボボックスから育成ウマ娘を選択していないと、育成ウマ娘のイベントが表示されないので注意  
[能力詳細]ボタンから[ウマ娘詳細]を表示すると、自動的に育成ウマ娘が選択されます

![](https://raw.githubusercontent.com/amate/UmaUmaCruise/images/images/ss3.png)

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

icon
- ICON HOIHOI  
http://iconhoihoi.oops.jp/

## ■How to build
ビルドには、Visual Studio 2019が必要です  

$(SolutionDir)は、UmaCruise.sln があるフォルダのことです

・opencvの準備  
https://github.com/opencv/opencv/releases  
上記リンク先から opencv-4.x.x-vc14_vc15.exe をダウンロードして、適当なディレクトリにファイルを解凍してください

$(SolutionDir)opencv\x64\lib に  
opencv_world451.lib  
opencv_world451d.lib  

$(SolutionDir)opencv\include に  
opencv2 フォルダが存在する状態にしてください

・tesseractの準備  
https://qiita.com/lilac0011/items/9fe6e061c3a036689a36  
基本上記の記事通りで、vcpkgから導入します  

>vcpkg install tesseract:x64-windows-static

一部コマンドを変更して x64-windows-static と指定してください  
vcpkg installが終了すると  
vcpkg.exeがあるフォルダ\installed\x64-windows-static  フォルダ以下に、libやincludeディレクトリが生成されているはずです  

x64-windows-staticフォルダをコピーして $(SolutionDir) へペースト  
コピペしたフォルダ名を "tesseract" へ変更すれば完了です  
$(SolutionDir)tesseract\lib に各種ライブラリが入ってるかどうかを確認してください

・boostの準備  
boost::log filesystem timer を使っているので、それぞれビルドが必要です  
-Boostライブラリのビルド方法  
https://boostjp.github.io/howtobuild.html  
上記URLを参考にビルドしてください  

コマンドライン
>b2.exe install --prefix=lib64 toolset=msvc-14.2 runtime-link=static address-model=64 --with-filesystem --with-date_time --with-timer --with-log

ビルドが終了すると
boostフォルダ\lib64\lib にライブラリが生成されているはずです  
UmaCruiseプロパティページの VC++ ディレクトリにパスを設定します
インクルードディレクトリに  
boostフォルダ\lib64\include\boost-1_xx  
ライブラリディレクトリに  
boostフォルダ\lib64\lib  
を追加してください

・WTLの準備  
https://sourceforge.net/projects/wtl/files/latest/download  
上記URLからファイルをダウンロードして、適当なフォルダに解凍してください  
boostの時と同じように VC++ディレクトリのインクルードディレクトリに  
WTL10のフォルダ\Include  
のパスを追加してください

インクルードディレクトリやライブラリディレクトリはDebug/Releaseそれぞれ設定してください

以上で、ビルドの前準備は完了です  
あとはビルドが正常に終わることを祈りましょう


## ■イベント選択肢データ(UmaMusumeLibrary.json)について

‎Gamerch様運営の"ウマ娘攻略wiki"からイベントデータを加工して、自動生成しています  
URL:https://gamerch.com/umamusume/

## ■著作権表示
Copyright (C) 2021 amate

一連のソースコードを、個人的な利用以外に使用することを禁止します

## ■更新履歴

<pre>

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
