WHICH=telnet

include ../vars.mk

ifeq ($(WHICH),telnet)
libcomm.so: telnetcomm.o
else
libcomm.so: ipcomm.o
endif
	gcc -shared $^ -o $@

all: libcomm.so

.PHONY: test
test:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DTELNET -fsanitize=address,undefined -fno-omit-frame-pointer test_ipcomm.c ../unix/wincomm.c -o test_ipcomm
	./test_ipcomm
	$(RM) test_ipcomm

install_libs install: libcomm.so
	cp -f $^ $(LIB)

telnetcomm.o: ipcomm.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -DTELNET -c $^ -o $@

clean:
	-rm libcomm.so *.o
