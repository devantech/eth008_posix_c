# ETH008 POSIX C example

This is a simple utility written in for POSIX C that can be used to connect to an ETH008, and then read or set the state of the relay outputs.

The ETH008 is a Ethernet connected module that provides 8 volt free contact relay outputs. It has a simple TCP/IP control interface, and MQTT support with TLS encryption. More details can be found [on our website.](https://www.robot-electronics.co.uk/eth008b.html)

## Compile
```
gcc -std=c99 -pedantic -o eth008 eth008.c
```

## Usage

In the following replace <port>, <password> and <ip> with the port number, password, and IP address of your module.

Toggle relay number 1.
```
eth008 -t 1 -P <password> -p <port> <ip> 
```


