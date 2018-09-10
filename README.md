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

```
bst-cli checksignature 0eec311afa6e8253c984d9bd57dadedd72848065160a093dc66a776388cfda13 /path/to/file/myfile
PASS
```

```
bst-cli listtransactions
Among other parameters returns "datasize" field informing about the size of the data stored in the blockchain.
```

### Bets
The gambling games available are based on modulo operation. A user defines a modulo argument (by default it is 36). The last 4 bytes word of block hash is divided modulo by the modulo argument and the result is increased by 1. Along with the modulo argument the user provides its lucky number. If the modulo division result increased by 1 computed from the hash of a block containing a user bet transaction is equal to the lucky number included in this transaction, then he wins.

To gamble there are two kind of transactions: creating a bet - makebet, redeeming the bet - getbet.
By default makebet is configured to play a roulette. This means that the draw is made from numbers from 1 to 36.
To begin you call RPC makebet. 

For example:

```
makebet straight_2@0.1+street_3@0.05+even@0.7
```

The above transaction contains 3 bets:
1. straight with valaue 2 and amount 0.1 BST
2. street with valaue 3 and amount 0.05 BST
3. even with amount 0.7 BST
Each bet within a transaction creates one output. 

Bets outputs occupies outputs beginning from index 0. After bets output there is an op_return output containing a description of the bets included in this transaction.
The last output is for change purpose. The change takes into account a fee.

So, in this case we have following outputs:
0 - related to straight_2 with the amount 0.1 BST
1 - related to street_3 with the amount 0.05 BST
2 - related to even with the amount 0.7 BST
3 - op_return with string "00000024_straight_2+street_3+even" with the amount 0.0 BST
4 - change with the amount: prev_txs_outs_amount-0.1-0.05-0.7-fee. 
    For example if prev_txs_outs_amount=50 BST, fee=0.0004 BST then:
    change output amount = 50-0.1-0.05-0.7-0.0004 = 49.1496 BST

To make a bet you should have total input amount equal to sum of bet amounts plus fee. In this case you should have: 0.1+0.05+0.7+0.0004 BST. Where fee is computed automatically inside makebet RPC.

The op_return string begins with hexadecimal number defining a modulo argument (0x24 for roulette), followed with user bets. The modulo argument is separeted by underscore, bets are separated by plus. 

Calling makebet RPC returns transaction ids.
This transaction id is used as an input to getbet RPC.

For example:
```
makebet straight_2@0.1+street_3@0.05+even@0.7
123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e

getbet 123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB
```
where the first argument is transaction id, the second is an address where you want to sent a pontential reward.

In a case of complex bet (considered example is complex because it has 3 bets in one makebet transaction), the getbet RPC produces 3 transactions. Every of the transaction tries to redeem one bet output of the makebet transaction. If it redeems an output, a reward is sent to the provided address. This reward is reduced by a fee. The total reward is a sum of rewards from all successfuly redeemed outputs minus total fee.
A single reward is counted as output amount multiplied by reward ratio minus fee.

Here we could have:
1. 0.1*36-fee=3.6-fee1 BST
2. 0.05*12-fee=0.6-fee2 BST
3. 0.7*2-fee=1.4-fee3 BST
If all outputs win, we will have 5.6-fee1-fee2-fee3 BST

The getbet RPC returns transaction id for successfuly redeemd outputs plus errors for the rest of outputs.

For example:
```
getbet 123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB
Script failed an OP_EQUALVERIFY operation
Script failed an OP_EQUALVERIFY operation
84b53d691bf35eea08ad313d7632523842e04035bdd633e55019476467ab022d
```

The above getbet output means that: 
straight_2 - didn't win
street_3 - didn't win
even - won
so, the total reward is 0.7*2-fee=1.4-fee3 BST.


For the roulette following bets are available:
```
1. straight: straight_1, straight_2, ...,straight_36. 
Reward ratio=36.

2. split:
	split_1={1, 4},
	split_2={4, 7},
	split_3={7, 10},
	split_4={10, 13},
	split_5={13, 16},
	split_6={16, 19},
	split_7={19, 22},
	split_8={22, 25},
	split_9={25, 28},
	split_10={28, 31},
	split_11={31, 34},
	split_12={2, 5},
	split_13={5, 8},
	split_14={8, 11},
	split_15={11, 14},
	split_16={14, 17},
	split_17={17, 20},
	split_18={20, 23},
	split_19={23, 26},
	split_20={26, 29},
	split_21={29, 32},
	split_22={32, 35},
	split_23={3, 6},
	split_24={6, 9},
	split_25={9, 12},
	split_26={12, 15},
	split_27={15, 18},
	split_28={18, 21},
	split_29={21, 24},
	split_30={24, 27},
	split_31={27, 30},
	split_32={30, 33},
	split_33={33, 36}
Reward ratio=18.

3. street:
        street_1={1, 2, 3},
        street_2={4, 5, 6},
        street_3={7, 8, 9},
        street_4={10, 11, 12},
        street_5={13, 14, 15},
        street_6={16, 17, 18},
        street_7={19, 20, 21},
        street_8={22, 23, 24},
        street_9={25, 26, 27},
        street_10={28, 29, 30},
        street_11={31, 32, 33},
        street_12={34, 35, 36}
Reward ratio=12.

4. corner:
        corner_1={1, 2, 4, 5},
        corner_2={4, 5, 7, 8},
        corner_3={7, 8, 10, 11},
        corner_4={10, 11, 13, 14},
        corner_5={13, 14, 16, 17},
        corner_6={16, 17, 19, 20},
        corner_7={19, 20, 22, 23},
        corner_8={22, 23, 25, 26},
        corner_9={25, 26, 28, 29},
        corner_10={28, 29, 31, 32},
        corner_11={31, 32, 34, 35},
        corner_12={2, 3, 5, 6},
        corner_13={5, 6, 8, 9},
        corner_14={8, 9, 11, 12},
        corner_15={11, 12, 14, 15},
        corner_16={14, 15, 17, 18},
        corner_17={17, 18, 20, 21},
        corner_18={20, 21, 23, 24},
        corner_19={23, 24, 26, 27},
        corner_20={26, 27, 29, 30},
        corner_21={29, 30, 32, 33},
        corner_22={32, 33, 35, 36}
Reward ratio=9.

5. line:
        line_1={1, 2, 3, 4, 5, 6},
        line_2={4, 5, 6, 7, 8, 9},
        line_3={7, 8, 9, 10, 11, 12},
        line_4={10, 11, 12, 13, 14, 15},
        line_5={13, 14, 15, 16, 17, 18},
        line_6={16, 17, 18, 19, 20, 21},
        line_7={19, 20, 21, 22, 23, 24},
        line_8={22, 23, 24, 25, 26, 27},
        line_9={25, 26, 27, 28, 29, 30},
        line_10={28, 29, 30, 31, 32, 33},
        line_11={31, 32, 33, 34, 35, 36}
Reward ratio=6.

6. column:
        column_1={1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34},
        column_2={2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35},
        column_3={3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36}
Reward ratio=3.

7. dozen:
        dozen_1={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
        dozen_2={13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
        dozen_3={25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36}
Reward ratio=3.

8. low:
	low={1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18}
Reward ratio=2.

9. high:
	high={19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36}
Reward ratio=2.

10. even:
	even={2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36}
Reward ratio=2.

11. odd:
	odd={1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35}
Reward ratio=2.

12. red:
	red={2, 4, 6, 8, 10, 11, 13, 15, 17, 20, 22, 24, 26, 28, 29, 31, 33, 35}
Reward ratio=2.

13. black:
	black={1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36}
Reward ratio=2.
```

For games other than a roulette only stright bets are available. In this case reward ratio is equal modulo argument and in the op_return field shortened description is used, containing
only reward ratio and consecutive straight bets, excluding "straight" prefix.
For example:
```
makebet 2@0.1+3@0.05 72
op_return: 00000048_2+3
```
