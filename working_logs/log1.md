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
## リンクができない
lldでgEfiLoadedImageProtocolGuidがリンクができないと表示されつまる．
Loader.infにProtocolを記述していなかったのが原因．
書籍に書かれていないことが原因だったため，しっかりとgitでdiffをとって，差分を確認しながら進めていく必要性を感じた．

# day3
