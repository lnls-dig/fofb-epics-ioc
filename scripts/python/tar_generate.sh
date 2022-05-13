#!/bin/bash

mkdir Results/tar
cd Results

for file in *1.3; do
	tar -cvzf tar/$file.tar.gz $file
  done
