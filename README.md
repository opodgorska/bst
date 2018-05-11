# BlockStamp

BlockStamp (BST) is a new digital currency intended to store user data in blockchain. It is based on Bitcon peer-to-peer technolgy to operate with no central authority.
To distinguish from Bitcoin, BlockStamp uses hashes of blocks begining with the most significant bit set to one (0x80000000.... instead of 0x00000000...).


## Details

The blockchain will be used to offer cheap transactions for time-stamping of documents. The BST-client will enable the submission of the hash of a document
or the whole document in a transaction to be recorded on the blockchain. A BlockStamp project is a clone of Bitcoin which is very well documented.
Most of the Bitcoin documentation refers directly to BlockStamp as well. 
Here you can find same useful links:
- https://en.bitcoin.it/wiki/Main_Page
- https://bitcoin.org/en/developer-documentation


## Current status

BlockStamp project is under development and new features are continously added. Especially new RPC commands facilitating data manipulation are under development.
A working node you can connect with is available online.


## Installation

To start using BlockStamp you shoud build the project and run a node with option -txindex to enable blockchain transaction queries.
The node default configuration let you connect to our working nodes. When you connect them, your node should start downloading blocks.
After downloading process is done you can start working with BlockStamp.

Default settings for BST are following: 
- working directory:		~/.bst 
- config file:			bst.conf
- RPC port:			8445
- peer-to-peer network port:	8446
- executable names:		bst*

To store and retrieve data from the blockchain following RPC commands are now availlable:

```
bst-cli storemessage "user data string"
c04878b7bf26def16ee689863943da91f9e7dcce77250e1ca8b6390549356006
```

```
bst-cli retrievemessage c04878b7bf26def16ee689863943da91f9e7dcce77250e1ca8b6390549356006
"user data string"
```

```
bst-cli storesignature /path/to/file/myfile
0eec311afa6e8253c984d9bd57dadedd72848065160a093dc66a776388cfda13
```

```
bst-cli storedata /path/to/file/myfile
ad1e1c0736f366fdf6b2e9b63048c02a0aef83fdf4e705e78f89c3e654fa3323
```

```
bst-cli retrievedata ad1e1c0736f366fdf6b2e9b63048c02a0aef83fdf4e705e78f89c3e654fa3323
"content of a myfile as a string"

bst-cli retrievedata ad1e1c0736f366fdf6b2e9b63048c02a0aef83fdf4e705e78f89c3e654fa3323 /path/to/file/outfile
```

```
bst-cli listtransactions
Among other parameters returns "datasize" field informing about the size of the data stored in the blockchain.
```

Along with RPC commands also Qt client was developed to provide the same data manipulation functionality. A 'Data' page embedded into bst-qt application
contains two tabs that enable data to store into or retrieve from the blockchain. A dialog with transaction details also informs about "Data size" for a
given transaction.
