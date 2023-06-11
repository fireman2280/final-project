all : main_printf.c bootstrap.S
	riscv32-unknown-elf-g++ main_printf.c bootstrap.S -o main -nostartfiles -march=rv32ima -mabi=ilp32
	
sim: all
	/home/user/ee6470/docker-images/ee6470_risc-v/riscv-vp/vp/build/bin/tiny32-mc --intercept-syscalls main

dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
dump-comment: all
	objdump -s --section .comment main
	
clean:
	rm -f main
