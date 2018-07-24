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

To store and retrieve data from the blockchain following RPC commands are available:

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

To check if data stored in blockchain match user data following RPC commands are available:

```
bst-cli checkdata ad1e1c0736f366fdf6b2e9b63048c02a0aef83fdf4e705e78f89c3e654fa3323 /path/to/file/myfile
PASS
```

```
bst-cli checkmessage c04878b7bf26def16ee689863943da91f9e7dcce77250e1ca8b6390549356006 "user data string"
PASS
```

bst-cli checksignature 0eec311afa6e8253c984d9bd57dadedd72848065160a093dc66a776388cfda13 /path/to/file/myfile
PASS
```

```
bst-cli listtransactions
Among other parameters returns "datasize" field informing about the size of the data stored in the blockchain.
```

To play lottery following RPC commands are available:

```
makebet 0 0.1 2
15189c8466bac9680be23e2a3a4490e954fe59895f1c329ac9b008a7157caaa7
```

```
getbet 15189c8466bac9680be23e2a3a4490e954fe59895f1c329ac9b008a7157caaa7 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB
34bcf741f0899b33338893cc0be99550a28034a84f3aa91801aec1e4f2d17aef
```

To play lottery you need to create a bet using the makebet command which returns a transaction id. After the transaction is confirmed (is stored in the blockchain) you can check if you win
by calling the getbet command. If you win your reward is sent to the provided address.

The makebet command requires at least 3 arguments:
1. "number"                      (numeric, required) A number to be drown in range from 0 to 1023 
2. "amount"                      (numeric, required) Amount of money to be multiplied if you win or lose in other case. Max value of amount is half of block mining reward
3. "reward_mult_ratio"           (numeric, required) A ratio you want to multiply your amount by if you win. This value must be power of 2

The getbet command requires at least 2 arguments:
1. "txid"         (string, required) The transaction id returned by makebet
2. "address"      (string, required) The address to sent the reward

Along with RPC commands also Qt client was developed to provide the same data manipulation and lottery functionality. A 'Data' page embedded into bst-qt application
contains three tabs that enable data to store into, check and retrieve from the blockchain. A dialog with transaction details also informs about "Data size" for a
given transaction. Also a 'Lottery' page was added. This page handle two commands: make/get bet creating transactions that let you to make bets and transations that 
let you get rewards from winning bets.
