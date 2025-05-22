![sac_introduce](https://ibb.co/hRKtvYYP)
# SAC - Simple Archive Creator.
Simple and fast archive maker
## Dependencies
clang
## Install guide
Clone the repo
```
git clone https://github.com/diabloblacno/sac.git
```
Compile sac-make and sac-unpack. It's ok if warnings appear
```
clang sac-make -o sac-make && clang sac-unpack -o sac-unpack
```
Copy fresh-compiled files to /usr/bin
```
cp sac-make sac-unpack /usr/bin
```
Now it's ready-to-go!
## Usage 
```
sac-make <archive-name.sac> <file1> <file2>. #Please, use .sac file extensions for sac archives
```

```
sac-unpack <archive-name>
```
