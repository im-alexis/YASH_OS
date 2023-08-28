yash: yash.c yash.h
	gcc yash.c yash.h -lreadline -o yash
clean:
	rm yash