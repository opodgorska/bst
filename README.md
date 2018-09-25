# BlockStamp

BlockStamp (BST) is a new digital currency intended to store user data in blockchain. It is based on Bitcon peer-to-peer technology to operate with no central authority. The goal was to create a new fast and effective blockchain that could be used for trusted timestamping of documents. Proof of concept of the solution involves online gambling platform, where the BlockStamp blockchain is used as the core.


## Details

The BlockStamp currency is a clone of Bitcoin which is very well documented.
Most of the Bitcoin documentation refers directly to BlockStamp as well. 
Here you can find same useful links:
- https://en.bitcoin.it/wiki/Main_Page
- https://bitcoin.org/en/developer-documentation

To distinguish from Bitcoin, BlockStamp uses hashes of blocks beginning with the most significant bit set to one (0x80000000.... instead of 0x00000000...). The blockchain will be used to offer transactions for timestamping of documents with minimal mining fee. The BST-client will enable the submission of the hash of a document or the whole document in a transaction to be recorded on the blockchain. 

The timestamping is already being tested on the online gambling portal. Chaining the blocks, like with BTC blockchain, makes it impossible to modify each block without modifying all the subsequent blocks. This is due to the hash of previous block being included in the following block. Given that a block’s hash is generated based on its content, the block’s content modifications would alter the hash and all subsequent blocks either. This characteristics of a blockchain is used for ensuring that no transactions, as well as wins and loses, would be tampered with.

## Current status

BlockStamp project is under development and new features are continuously added. Currently, new RPC commands facilitating data manipulation are under development. There is already a working node available you can connect to. It is available online at blockstamp.info. 


## Installation

To start using BlockStamp you should build the project and run a node with option -txindex to enable blockchain transaction queries.
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
There are two kind of transactions related with betting: creating a bet - makebet, redeeming the bet - getbet.
By default makebet is configured to play a roulette. This means that the draw is made from numbers from 1 to 36.
To begin you call RPC makebet. 

There can be more bets in one transaction than one. For example:

```
makebet straight_3@0.1+street_5@0.05+high@0.7
```

Which is a compilation of three bets:
1. No 3 with 0.1BST bet amount
2. No 7,8,9 with 0.05BST bet amount
3. High numbers with 0.7BST bet amount

The details of the bet will be stored in op_return field in the transaction. In this case, it would look as follows:
Op_return: 00000024_straight_3+street_5+high

To make a bet a user should have total transaction input amount equal to sum of bet amounts plus fee. In this case: 0.1+0.05+0.7+0.000x BST. The fee is computed automatically inside makebet RPC and is the mirror of current BTC fee algorithm. This fee is a mining fee only, there are no other fees related with betting or winning. 
Calling makebet RPC returns transaction ids. This transaction ID is used then as an input to getbet RPC, which accepts two arguments - transaction ID and an address where the reward should be sent. For example: 

```
getbet 123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB
```

Getbet returns a transaction ID for a successfully redeemed wins or errors in case of loses. To properly redeem the win amount, Pay-To-Public-Key-Hash transaction keys must match. 

Making bets has following limitation, by design:
1. Maximum modulo argument (reward ratio) is 2^30
2. Maximum reward 2^20 BST
3. Minimum bet is 0.00000001BST

As for the actual transaction syntax, it looks as follows. Makebet transaction is signaled by 30’th bit in the transaction version field. Value 1 at this position (0x40000000) sets transaction type to makebet transaction. Bets outputs occupies outputs beginning from index 0. Next to bets output, there is an op_return output containing the description of the bets included in the transaction. The op_return string begins with hexadecimal number defining a modulo argument (e.g. 0x24 for roulette, 0x48 for lottery drawing 1 out of 72 numbers, and 0xB for double dice), followed with user bets. The modulo argument is separated by underscore, bets are separated by plus.

The last output field is reserved for change purpose, where change is transaction input minus bets amount minus fee, i.e.:
prev_txs_outs_amount-Bet1_amount-Bet2_amount-Bet3_amount-...-fee
In case then the transaction input is 50BST (which is the standard block reward), and 3 bets for 0.1BST, 0.05BST, and 0.7BST, and fee=0.0004 BST the change field would contain 49.1496 BST (50-0.1-0.05-0.7-0.0004 = 49.1496 BST).

The op_return string begins with hexadecimal number defining a modulo argument (0x24 for roulette), followed with user bets. The modulo argument is separated by underscore, bets are separated by plus. 

Calling makebet RPC returns transaction ids.
This transaction id is used as an input to getbet RPC.

For example:
```
makebet straight_2@0.1+street_3@0.05+even@0.7
123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e

getbet 123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB
```
where the first argument for getbet is transaction id, the second is an address where you want to sent a potential reward.

In a case of complex bet (considered example is complex because it has 3 bets in one makebet transaction), the getbet RPC produces 3 transactions. Every of the transaction tries to redeem one bet output of the makebet transaction. If it redeems an output, a reward is sent to the provided address. This reward is reduced by a fee. The total reward is a sum of rewards from all successfully redeemed outputs minus total fee.

A single reward is counted as output amount multiplied by reward ratio minus fee.

Here we could have:
1. 0.1*36-fee=3.6-fee1 BST
2. 0.05*12-fee=0.6-fee2 BST
3. 0.7*2-fee=1.4-fee3 BST
If all outputs win, we will have 5.6-fee1-fee2-fee3 BST

The getbet RPC returns transaction id for successfully redeemed outputs plus errors for the rest of outputs.

For example:
```
getbet 123d6c76257605431b644b43472ee3666c4f27cc665ec8fc48c2551a88f9906e 36TARZ3BhxUYaJcZ2EF5FCT32RnQPHSxYB
Script failed an OP_EQUALVERIFY operation
Script failed an OP_EQUALVERIFY operation
84b53d691bf35eea08ad313d7632523842e04035bdd633e55019476467ab022d
```

The above getbet output means that: 
straight_3 - didn't win
street_5 - didn't win
high - won
so, the total reward is 0.7*2-fee=1.4-fee3 BST.

Making bets has following limitation:
1. Maximum modulo argument (reward ratio) is 2^30
2. Maximum reward 2^20 BST
3. Minimal bet is 0.00000001 BST

### RNG and redeeming rewards
Each makebet RPC defines one or more numbers chosen by user as his/her bets. The numbers will then be compared to the random number drawn by the system. BlochStamp casino's approach is to use the hash of the block containing bet transaction as a base for RNG. 

When accepted by pool, a bet is put into a block. The last 4 bytes word of block hash is divided modulo by the modulo argument (the actual bet) and the result is increased by 1. The result is then taken and compared with the bet, and the bet is a winning one if the numbers are equal. 

For example, let’s imagine that a user bets on roulette result to be 12. The bet is put into a transaction and the transaction is part of a freshly mined block. The hash of the block is then truncated to obtain only the last 4 bytes, decimalised, and divided modulo 36. The result of the equation is increased by 1. If the increased value is equal 12 then the bet is a winning one.

To successfully redeem the reward by getbet two conditions must be met:
1. At least one of the numbers betted in makebet must match the last 4 bytes word of block hash divided modulo by the modulo argument and increased by 1
2. keys must match (as in the P2PKH transactions)

### Jackpot
By design, the jackpot/reward is limited to 2^20 BST, i.e. 1 048 576 BST. The jackpot is set to this amount from the beginning. No losing bets are required to fill it in. It is also designed so that a win of the maximum amount does not reduce future rewards. The reward coins will simply be mined as part of the block containing the winning transaction. The design was possible due to probability of a win of such amount and some constraints on the actual betting. If a sum of input amounts for winning makebet transactions included in a given block is greater or equal 0.9 of a current block subsidy (i.e. 50BST at the moment), and if the sum of payoffs for for this block is greater of the sum of inputs and the subsidy, then the block is rejected. This prevents huge bets with lowest modulo (i.e. 2) that can cause the system to collapse. 

### Mining constraints
Coinbase transaction in each of the blocks contains 50BST, that are unspendable for the next 1000 blocks. 
A mining policy exists. The rule is that:
1. If a sum of input amounts for winning makebet transactions included in a given block is greater or equal 0.9 of a current block subsidy (currently 50BST) and if the sum of payoffs for for this block is greater of the sum of inputs and the subsidy, then the block is rejected.
2. If a sum of payoffs for winning makebet transactions included in a given block is greater or equal than maximum reward  (2^20 BST, i.e. 1 048 576 BST), then the block is rejected.
The miners can select the transactions they want to put into a block, based on the pools rules (e.g. fee, amount of a bet, potential reward). 

### List of bet types
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
	split_33={33, 36},
	split_34={1, 2},
        split_35={2, 3},
        split_36={4, 5},
        split_37={5, 6},
        split_38={7, 8},
        split_39={8, 9},
        split_40={10, 11},
        split_41={11, 12},
        split_42={13, 14},
        split_43={14, 15},
        split_44={16, 17},
        split_45={17, 18},
        split_46={19, 20},
        split_47={20, 21},
        split_48={22, 23},
        split_49={23, 24},
        split_50={25, 26},
        split_51={26, 27},
        split_52={28, 29},
        split_53={29, 30},
        split_54={31, 32},
        split_55={32, 33},
        split_56={34, 35},
        split_57={35, 36}
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

### Lottery and other systems

As in case of roulette, there are identical two types of transactions - makebet and getbet, with the same syntax. What makes lottery and other games different is that the only bet type available is straight bet. Each makebet can define one number or more as separate bets from 1 up to 2^30 with modulo set to maximum 2^30. Makebet RPC can accept more than one straight bet, up to 10 bets. The modulo operator defined is tantamount to the number of options we draw from. For example:
```
makebet 2@0.1+3@0.05 72
```
would put into a transaction a bet for No 2 and No 3 with different incentives (0.1BST and 0.05BST accordingly). Having modulo set to 72 means that the 4 bytes of hash of the block in which the transaction will be put, shall be divided modulo 72 (as if we drawn from 72 numbers from 1 to 72). If the result of the hash modulo 72 operation plus one is equal to any of the numbers user betten on, the transaction will be a winning one. All bets in one makebet RPC are subject to the same rules and same modulo defined in the makebet. 

In this case reward ratio is equal modulo argument and the op_return field contains only reward ratio and consecutive straight bets, excluding bet name prefix (e.g. "straight"). For example, in case of the above bet op_return would look as follows:
op_return: 00000048_2+3

Getbet RPC works the same way as in the roulette model, i.e. it accepts transaction ID and an address to which a potential reward should be sent. Again, it verifies whether any of the betted numbers is equal with the hash modulo 72 +1 and returns a transaction hash in which the reward is sent.

### Your own game: double dice example

The BlockStamp platform can be treated as a hosting platform for user’s own game. Makebet and getbet transactions are available for this option and work as they do in the lottery game. User’s own game rules determine the bets and modulo, where modulo is the reward ratio at the same time. For example, if dice is to be implemented, the bets should accept numbers from 1 to 6 and modulo should be set to 6. If two dices are to be rolled together, numbers would be from 2 to 12 and modulo should be set to 11. To avoid 0, the result of hash modulo operation is plused one and only after that it is compared with the number user betted on. So the makebet could look as follows:

```
makebet 2@0.1+11@0.05 11
```

This means that a user bets on double dice rolling 2 (1+1) with 0.1BST and 11 (5+6) with 0.05BST. Modulo is set to 11, and the block of the hash which would contain the bet would be divided by 11 and then added 1.  
