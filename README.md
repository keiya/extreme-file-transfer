mFTP - minimized-data File Transfer Protocol

Protocol
========
A mFTP (Protocol) is like a HTTP (Protocol).
All control traffic are transmitted by plain text except data payload (body).

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
| 0x0D 0x0A (start with \r\n means end of header) |
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
| 0x0D 0x0A (start with \r\n means end of header) |
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
### Request Header
* filename (Required)

### Response Header
* filename (Required)

## PUT
### Request Header
* filename (Required)

### Response Header
* filename (Required)
