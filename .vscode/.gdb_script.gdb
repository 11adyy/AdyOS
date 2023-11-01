    symbol-file /home/11adyy/Desktop/AdyOS.EXMPL/build/i686_debug/kernel/kernel.elf
    set disassembly-flavor intel
    target remote | qemu-system-i386 -S -gdb stdio -m 32 -hda /home/11adyy/Desktop/AdyOS.EXMPL/./build/i686_debug/image.img
