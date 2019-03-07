cp build/kamomil-helloworld_kvm-x86_64 iso/boot/helloworld_kvm-x86_64.elf 
grub-mkrescue -o bla.img iso
qemu-system-x86_64 -hda bla.img &
