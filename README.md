PWoT Talker
===========

An ancient Talker system. The daemon runs on a Unix host, users then use telnet, netcat or some other simplistic client to connect.

For information on what a Talker is, see [Wikipedia](https://en.wikipedia.org/wiki/Talker)

Warning
=======

This is a historical relic written in 1999 as a learning exercise with no regard to resources, efficiency or security. **Do not to run this on an Internet-facing host!**

Installation
============

Surprisingly this still actually compiles on runs on Ubuntu 16.04. Make sure you have the ```libxml2-dev``` package installed, then:

1. Run ```./configure && make```

2. Run ```src/pwot```

3. Connect to localhost port 7000 with telnet and login as ```admin``` with the password ```password```


