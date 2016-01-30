mFTP - minimized-data File Transfer Protocol

Protocol
========
A mFTP (Protocol) is like a HTTP (Protocol).
All control traffic are transmitted by plain text except data payload (body).
Filename or pathname should be encoded by percent-encoding (URL encoding).

Var = Variable Size

## Request
Request header should begin with `q`.

```
+-------------------------------------+-----------+
|  q  |       Command Type            | 0x0D 0x0A |
|     |          [Var]                |           |
+--------------+------+---------------+-----------+
| Header #1 |  |      |               |           |
+-----------+  | 0x3a | Header Value  | 0x0D 0x0A |
| Header Name  | [1b] |     [Var]     |    [2b]   |
|    [Var]     |      |               |           |
+--------------+------+---------------+-----------+
| Header #2 |  |      |               |           |
+-----------+  |      |               |           |
|              |      |               |           |
+--------------+------+---------------+-----------+
...
+--------------+------+---------------+-----------+
| Header #n |  |      |               |           |
+-----------+  |      |               |           |
|              |      |               |           |
+--------------+------+---------------+-----------+
| 0x0D 0x0A (\r\n is a terminator of head)        |
+-------------------------------------------------+
| Body (Binary/Text)                              |
|                                                 |
...                                               |
|                                                 |
+-------------------------------------------------+
```

## Response
```
+-------------------------------------+-----------+
|  s  |      Response Code            | 0x0D 0x0A |
|     |           [2b]                |           |
+--------------+------+---------------+-----------+
| Header #1 |  |      |               |           |
+-----------+  | 0x3a | Header Value  | 0x0D 0x0A |
| Header Name  | [1b] |     [Var]     |    [2b]   |
|    [Var]     |      |               |           |
+--------------+------+---------------+-----------+
| Header #2 |  |      |               |           |
+-----------+  |      |               |           |
|              |      |               |           |
+--------------+------+---------------+-----------+
...
+--------------+------+---------------+-----------+
| Header #n |  |      |               |           |
+-----------+  |      |               |           |
|              |      |               |           |
+--------------+------+---------------+-----------+
| 0x0D 0x0A (\r\n is a terminator of head)        |
+-------------------------------------------------+
| Body (Binary/Text)                              |
|                                                 |
...                                               |
|                                                 |
+-------------------------------------------------+
```

### Response Code
Response header should begin with `s`.

* s2x OK
* s4x NG


## GET
### Request `qget`
* Header
    * filename (Required)

## PUT
### Request `qput`
* Header
    * filename (Required)
* Payload
    * LZ4 Compressed Data

## DIR
### Request `qdir`
* Header
    * dirname (Required)
* Payload
    * 1 file per 1 line
    * Tab separated entry
    * Terminator is `\r\n`

## CD
### Request `qcd`
* Header
    * dirname (Required)


XFT Command Line Interface
==========================
* `get FILENAME`
    * Fetch 'FILENAME' from server
* `put FILENAME`
    * Send 'FILENAME' to server
* `ls [PATHNAME]` or `dir [PATHNAME]`
    * Remote ls
* `!ls [PATHNAME]` or `!dir [PATHNAME]`
    * Local ls
* `cd [PATHNAME]`
    * Change remote working directory
* `lcd [PATHNAME]`
    * Change local working directory
* `open HOST:PORT`
    * Connect to the server


XFT Server
==========
`./mftpd PORT`

