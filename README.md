# netpipe
Universal TCP forwarder

This program was written by me at the XMAS of 2003 (ancient!).
It solves a problem I had: I needed to be able to connect from the ouside to the inside of a firewall.
I could init a connection from inside and rebuild it periodically and use PPP over SSH as a VPN solution.

## history

The scenario was like this:

Me | Firewall | Computer2

The firewall let the Computer2 to connect to Me (outgoing NAT).
If I started an SSH session for example from Computer2 to Me, it worked.
It did not work the other way, the firewall did not let me travel through to the Inside.

So, I had to do a reverse port forward for an SSH connection, and even could start a PPP over SSH (poor man's VPN).

I set up a cron job, to try to start an outgoing connection every 2-3 hours, if the ppp interface was down.
On the Outside (@Me) I just set up a listener, and tunneled SSH back behind the firewall.

## usage

netpipe has 3 modes:

> netpipe i2i ListenIP1:SPort1:ListenIP2:SPort2

In this mode netpipe listens for a TCP connection on IP1:Port1 and after a successful connect it
starts to listen on IP2:Port2 then connects the two sockets.
For example ListenIP1:SPort1 is a public IP:Port waiting for the other netpipe, ListenIP2:Sport2 is
a localhost:highport waiting for my SSH -p client connection.

This is useful on the outside of the firewall.

> netpipe o2o DHost1:DPort1:Dhost2:DPort2

This is the mode, where netpipe connects to a host (i.e. ouside of FW through NAT), and when successful connects to the second host.
For example DHost1:DPort1 is the outside listening netpipe i2i, DHost2:DPort2 is localhost:ssh.

This is usefule in the inside of the firewall.

> netpipe i2o ListenIP:Sport:DHost:Dport

Simple port forwarder. Why not.
