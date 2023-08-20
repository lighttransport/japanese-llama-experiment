git clone https://github.com/shogo82148/TinySegmenterMaker

cd TinySegmenterMaker/

clang++ -DMULTITHREAD -O3 -o train train.cpp -lboost_thread -pthread
