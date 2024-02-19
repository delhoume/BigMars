ls -a *.tif  | parallel ../bin/check_full {}  && touch ../check_result/{}
