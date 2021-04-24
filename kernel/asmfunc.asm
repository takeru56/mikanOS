
; IOアドレス空間にデータを読み書きする関数を作成

; System V AMD64 ABI
; RDI, RSI, RDX, RCX, R8, R9の順で引数をレジスタにわりあてる
; EAXに返り値

bits 64
section .txt

global IoOut32 ; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
    mov dx, di      ; dx = addr
    mov eax, esi   ; eax = data
    out dx, eax     ; dxのアドレスに対してeaxにセットされたdataを出力
    ret

global IoIn32 ; uint32_t IoIn32(uint16_t addr);
IoIn32:
    mov dx, di      ; dx = addr
    in eax, dx       ; eaxにdxのアドレスから32bitを入力して返す
    ret
