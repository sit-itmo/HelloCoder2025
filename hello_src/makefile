
BINS=hello.bin hello_none.bin hello32.bin hello64.bin
OUT= ../bin
rebuild: clean all


all: $(BINS)
	@echo Build done!

clean: 
	rm -f $(BINS)

hello_none.bin:
	gcc hello.c -nostdlib -nostdinc -fno-builtin -DNONE_VERSION -emain -o $(OUT)/$@

hello.bin:
	gcc hello.c -Os -o $(OUT)/$@

hello32.bin:
	gcc hello.c -m32 -o $(OUT)/$@

hello64.bin:
	gcc hello.c -o $(OUT)/$@
