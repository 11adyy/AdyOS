global TSS_flush ; TODO: Replace with tss.apl
TSS_flush:
    mov ax, 0x28
    ltr ax
    ret