# MiniTor
Implement a simplified Tor network using different processes to act as routers

Proj A:
+ Mostly set-up steps where we start to implements Proxy and routers using different processes (fork() function in C)
+ Ensure everything is working by sending some sample ICMP packets

Proj B:
+ Send traffic (ICMP packets) to external server through Tor network built by using processes to act as routers
+ Perform Tor-network operation such as create circuits, extend circuits, and relay data/returning data.
+ Perform all comunications with encyption to ensure security between hops (for simplicty, encryption method used in this project is very simple and NOT secure)

Proj C:
+ Extend to support TCP packets from proj B
+ Implement a simplified scheme to handle router failure and recovery