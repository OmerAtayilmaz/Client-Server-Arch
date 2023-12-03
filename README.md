# Tcp-Server-Client-Console-App
Multi-threaded chat server in C using the Winsock library on Windows. It is designed to accept multiple client connections and allow them to communicate with each other through the server.

The server listens for incoming client connections on a specific port and creates a new thread for each client that connects. The thread function threadvoid handles communication with the client.

## Send Message
To connect to the server, simply type the **username** and click enter.

## Send Message
When client connect server send online user list. User can send message to this users like this way.
```
x->hello
```
## Disconnect
If write console 'quit' client. Server delete user in online list and terminates the thread of the leaving user.
```
quit
```

## Error checking algorithms: Crc and Chekcsum

The crc_in it function initializes a lookup table for a cyclic redundancy check (CRC). The crc function uses this table to calculate the CRC of a given data array. The crc_stob function converts a string to a byte array and returns the CRC value of the array. The checksum function calculates the checksum of a given data array by summing the ASCII values of its elements.

## Save Logs
The addlogs function writes a message to a log file with the name date_username.txt. The log_file_name array holds the name of the log file.

## SS
![](https://raw.githubusercontent.com/sezer57/Tcp-Server-Client-Console-App/main/screenshot/client-server.jpg)
