http_server:http_server.c
	gcc -o $@ $^ -lpthread
.PHONY:clean
clean:
	rm http_server
