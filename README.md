# NIOH
This is No Intention Of Harming. A suite to perform various attacks

## Installation
Run 
```
make
```
If this fails, please see the [Error Handling](#error-handling) section.

## OS Support

As of now NIOH was only tested on Ubuntu.

## Error Handling

### curses.h not found
NIOH requires the ncurses library, if you are seeing the following error
```
fatal error: curses.h: No such file or directory
```
then run the following command
```
sudo apt-get update
sudo apt-get install libncurses-dev
```

### pcap.h not found
NIOH requires the pcap library, if you are seeing the following error
```
fatal error: pcap/pcap.h: No such file or directory
```
then run the following command
```
sudo apt-get update
sudo apt-get install libpcap-dev
```
