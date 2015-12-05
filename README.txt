README

This is an implementation of the gossip protocol. It can be run from multiple machines but must be run from the same directory in a file system. To simulate failures, set the value of B to be non-zero. It will select a random node to fail every P seconds, with a total of B failures.

*COMPILATION*
make clean; make


*EXECUTION*
In order to execute the program, run the following command in the terminal:
./p4 N b c F B P S T
number of peer nodes N,
gossip parameter b,
gossip parameter c,
number of seconds after which a node is considered dead F,
number of bad nodes that should fail B,
number of seconds to wait between failures P,
the seed of the random number generator S,
the total number of seconds to run T.

Note: N must be greater than B + b. Otherwise the protocol will hang indefinitely as it will not be able to find enough active neighbours to send updates to.

*TEST RUN 1 -  ./p4 5 2 10 10 0 20 1 60 - No failures. *
Endpoints file:
152.14.104.100:10000
152.14.104.101:10000
152.14.104.105:10000
152.14.104.105:10001
152.14.104.107:10000

list0.txt:
OK
0 59
1 58
2 58
3 58
4 58

list1.txt:
OK
0 59
1 59
2 58
3 58
4 58

The other list files are also similar as there are no failures.

*TEST RUN 2 -  ./p4 5 2 10 10 2 20 1 60 - 2 failures. *
Node 3 failed at 20 seconds and node 1 at 40.

Endpoints file:
152.14.104.100:10000
152.14.104.101:10000
152.14.104.105:10000
152.14.104.105:10001
152.14.104.107:10000

list0.txt:
OK
0 59
1 39
2 58
3 19
4 58

list1.txt:
FAIL
0 39
1 39
2 39
3 19
4 39

list2.txt: 
OK
0 59
1 40
2 59
3 19
4 59

list3.txt:
FAIL
0 19
1 19
2 18
3 19
4 19

list4.txt:
OK
0 59
1 39
2 58
3 20
4 59