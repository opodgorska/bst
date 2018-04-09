# Block STamp

Block STamp (BST) is a new digital currency intended to store user data in blockchain. It is based on Bitcon peer-to-peer technolgy to operate with no central authority.
To distinguish from Bitcoin, Block STamp uses hashes of blocks begining with the most significant bit set to one (0x80000000.... instead of 0x00000000...).


## Details

A Block STamp project is a clone of Bitcoin which is very well documented. Most of the Bitcoin documentation refers directly to Block STamp as well. 
Here you can find same useful links:
- https://en.bitcoin.it/wiki/Main_Page
- https://bitcoin.org/en/developer-documentation


## Current status

Block STamp project is under development and new features are continously added. Especially new RPC commands facilitating data manipulation are under development.
A working node you can connect with is available online.


## Installation

To start using Block STamp you shoud build the project and run a node with option -txindex to enable blockchain transaction queries.
The node default configuration contains build in addresses that let you connect to our working nodes.When you connect them, your node should start downloading blocks.
After downloading process is done you can start working with Block STamp.

Default settings for BST are following: 
- working directory:		~/.bst 
- config file:			bst.conf
- RPC port:			8445
- peer-to-peer network port:	8446
- executable names:		bst*

To store and retrieve data from the Block STamp blockchain RPC bitcoin commands should be used.
For now following RPC user data commands are availlable:

```
bstd-cli storemessage "user data string"
c04878b7bf26def16ee689863943da91f9e7dcce77250e1ca8b6390549356006
```

```
bstd-cli retrievemessage c04878b7bf26def16ee689863943da91f9e7dcce77250e1ca8b6390549356006
"user data string"
```

```
bstd-cli storefilehash /path/to/file/myfile
0eec311afa6e8253c984d9bd57dadedd72848065160a093dc66a776388cfda13
```

