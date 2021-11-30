#!/bin/bash

make

for i in $(seq 0 10000); do
	./tecnicofs inputs/test5.txt out.txt 50 |tail -1
done

make clean