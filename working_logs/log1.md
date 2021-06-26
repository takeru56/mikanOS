・qemuの起動
```
$ ~/osbook/devenv/run_qemu.sh ~/edk2/Build/MikanLoaderX64/DEBUG_CLANG38/X64/Loader.efi ./kernel.elf
```

# day1
##  環境構築
### mac 上でvirtualboxを試みる
→　windowを表示できるかわからずに挫折
### wsl上で試みる
次に，wsl上で実行を試みるも，環境変数DISPLAYの設定に手こずる．
https://github.com/microsoft/WSL/issues/4106
を参考にして，.profileに以下を追加することで解決．DNSの設定で8.8.8.8を設定していたため，書籍のとおりにはいけなかった．

```
export DISPLAY=$(route.exe print | grep 0.0.0.0 | head -1 | awk '{print $4}'):0.0
```

# day2
memory mapを取得してcsvに吐き出す．

## リンクができない
lldでgEfiLoadedImageProtocolGuidがリンクができないと表示されつまる．
Loader.infにProtocolを記述していなかったのが原因．
書籍に書かれていないことが原因だったため，しっかりとgitでdiffをとって，差分を確認しながら進めていく必要性を感じた．

# day3
## qemuによるデバッグ
・CPUのレジスタ値を確認
```
$ info registers
```
・addrを先頭とするメモリ領域の値を表示
fmt は[個数][フォーマット][サイズ]
個数：
何個分表示
フォーマット：
|フォーマット|説明|
|:--:|:--:|
|x|16進数|
|d|10進数|
|i|逆アセンブル|
サイズ：
|サイズ|説明|
|:--:|:--:|
|b|1バイト|
|h|2バイト|
|w|4バイト|
|g|8バイト|
```
何バイトを１単位とみなすか
$ x /fmt addr
```
ローダーからkernelをメモリ上に読み込み，実行する．


# ToDo
- USBドライバーのスクラッチ実装

# day6
PCIの詳細：http://archive.linux.or.jp/JF/JFdocs/The-Linux-Kernel-7.html

# day8
qemuを起動する際に
> Unable to init server: Could not connect: Connection refused
>QEMU 4.2.1 monitor - type 'help' for more information
のエラーが出て画面が出力されない．
=> なんかよくわからんけどwsl2を再起動したら直った．