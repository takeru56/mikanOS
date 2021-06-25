
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

global GetCS;
GetCS:
    xor eax, eax ; clear eax
    mov ax, cs ; 現在のコードセグメントのセレクタ値
    ret

global LoadIDT
LoadIDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di
    mov [rsp+2], rsi
    lidt [rsp]
    mov rsp, rbp
    pop rbp
    ret

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
    mov rsp, kernel_main_stack + 1024 * 1024
    call KernelMainNewStack
.fin:
    hlt
    jmp .fin

global LoadGDT
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 10
    mov [rsp], di ; limit
    mov [rsp + 2], rsi ; offset
    lgdt [rsp] ; 10byteの領域（gdtの大きさと場所）を渡してGDT Registerに登録
    mov rsp, rbp
    pop rbp
    ret

global SetCSSS
SetCSSS:
    push rbp
    mov rbp, rsp
    mov ss, si
    mov rax, .next
    push rdi
    push rax
    o64 retf
.next:
    mov rsp, rbp
    pop rbp
    ret

global SetDSALL
SetDSALL:
    mov ds, di
    mov es, di
    mov fs, di
    mov gs, di
    ret

global SetCR3
SetCR3:
    mov cr3, rdi
    ret
