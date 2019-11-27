CC:=gcc
OBJ:=*.c
exe:=main 


all:$(obj)
	
	$(CC) -o $(exe) $(OBJ) 
	@echo $(YELLOW)"========================Success========================"$(BLACK)

.PHONY:clean
clean:
	rm -rf $(obj) $(exe)