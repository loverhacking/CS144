This course is about computer network, and mainly cover the following topics:
* Internet and IP
  * How an application uses the Internet
  * The structure of the Internet:The 4 layer model
  * The Internet protocol(IP):What it is
  * Basic architectural ideas and principles
    - Packet switching
    - Layering
    - Encapsulatior
* Transport
  * Three Transport Layers:TCP,UDP,ICMP
  * How TCP works:Connections and Retransmissions
  * How UDP works
  * How ICMP works
  * The End to End Principle
* Packet Switching
  * Queueing delay and end-to-end delay
  * Why streaming applications use a playback buffer
  * A simple deterministic queue model
  * Rate guarantees
  * Delay guarantees
  * How packets are switched and forwarded
* Congestion Control
  * Principles
    * What network congestion is, what causes it, and what happens in routers
    * How we want the network to behave when congested: max-min fairness
    * AIMD: additive increase, multiplicative decrease
    * Behavior of multiple AIMD flows and TCP throughput equation
  * Practice
    * TCP Tahoe, Reno, and New Reno
    * Congestion window
    * Slow start, congestion avoidance
    * RTT estimation
    * Self-clocking
    * Fast retransmit, fast recovery, congestion window inflation
* Applications and NATS
  * NAT
  * DNS
  * Http
  * BitTorrent
* Routing
  * Approaches
    - Flooding
    - Source routing
    - Forwarding table
    - Spanning tree
  * Algorithms
    * Bellman‐Ford "distance vector" algorithm.
    Used by RIP.	
    * Dijkstra’s shortest path first “link state” algorithm. 
    Used by OSPF.
  * Internet routing
    * Hierarchical routing
    * BGP - path vector routing and local polices
    * Multicast routing
    * Spanning Tree Protocol(STP)
* Lower Layers
  * Ethernet and CSMA/CD
  * IP Fragmentation
  * Wireless networks
    - Why they are different
    - Sharing the channel
    - WiFi
  * Bit errors and coding
  * Error correcting codes
  * Shannon Capacity
* Security
  * Attacks
    * Eavesdropping, tampering, suppression
    * Spoofing, man-in‐the-middle
    * Redirection/hijacking at layers 2, 3 and 4	
    * Denial of service
  * Principles
    * Confidentiality
    * Integrity
    * Authenticity
  * Tools
    * Block ciphers(EBC mode, CBC mode)
    * Cryptographic hashes: collision resistance	
    * Message authentication codes
    * Public key encryption/decryption
    * Signatures and certificates