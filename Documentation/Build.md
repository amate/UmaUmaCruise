## ■How to build
実行ファイルの生成には、Visual Studio 2019が必要です  

ビルドに必要なライブラリは、[vcpkg]によって自動的にダウンロード、ライブラリ(.lib)の生成が行われるようになっています

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


<!----------------------------------------------------------------------------->

[vcpkg]: https://github.com/microsoft/vcpkg
