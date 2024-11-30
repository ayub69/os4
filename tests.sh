#!/bin/bash
# tests.sh - Automated testing for server-client file transfer

# Start the server in the background
./server & 
SERVER_PID=$!
sleep 1  # Give server time to start

# Test 1: Small text file with 1 thread
echo "Test 1: Small text file with 1 thread"
./client testfile.txt 1 t1.txt

# Test 2: Small text file with 5 threads
echo "Test 2: Small text file with 5 threads"
./client testfile.txt 5 t2.txt

# Test 3: Large text file with 10 threads
echo "Test 3: Large text file with 10 threads"
./client largefile.txt 10 t3.txt

# Test 4: Image file with 5 threads
echo "Test 4: Image file with 5 threads"
./client image.jpg 5 timage.jpg

# Test 5: Large image file with 10 threads
echo "Test 5: Large image file with 10 threads"
./client largeimage.jpg 10 t5.txt

# Test 6: Small video file with 10 threads
echo "Test 6: Small video file with 10 threads"
./client video.mp4 10 t6.txt

# Test 7: Medium video file with 20 threads
echo "Test 7: Medium video file with 20 threads"
./client mediumvideo.mp4 20 t7.txt

# Test 8: Compressed zip file with 5 threads
echo "Test 8: Compressed zip file with 5 threads"
./client archive.zip 5 t8.txt

# Test 9: Corrupted file handling
echo "Test 9: Corrupted file handling"
./client corruptedfile.bin 5 t9.txt

# Stop the server
kill $SERVER_PID