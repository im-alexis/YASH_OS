yash: yash.c yash.h
	gcc yash.c yash.h -lreadline -o yash
debug: yash.c yash.h
	gcc yash.c yash.h -g -lreadline 
clean:
	rm yash
cdebug:
	rm a.out yash.h.gch