
data:
	mkdir -p data bin
	gcc ./src/gen_random_data.c -o ./bin/gen_random

xdma:
	mkdir -p bin
	gcc ./src/xdma_rw.c -o ./bin/xdma_rw

clean:
	rm -rf data bin

.DEFAULT_GOAL := xdma