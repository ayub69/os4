#!/bin/bash

# Small text file
echo "This is a small test file." > testfile.txt

# Large text file
for i in {1..10000}; do echo "This is a line in a large test file." >> largefile.txt; done

# Small image placeholder
echo "This is a dummy image file." > image.jpg

# Large image placeholder
for i in {1..10000}; do echo "This is a line in a large image file." >> largeimage.jpg; done

# Small video placeholder
echo "This is a dummy video file." > video.mp4

# Medium video placeholder
for i in {1..20000}; do echo "This is a line in a medium video file." >> mediumvideo.mp4; done

# Compressed zip placeholder
echo "This is a dummy compressed file." > archive.zip

# Corrupted binary file placeholder
echo -ne "\x00\xFF\xFE\xFD" > corruptedfile.bin

echo "Test files created successfully!"

