cd ../../../

# Get block hash from height
echo "===Get/BlockHash==="
./nexus ledger/get/blockhash height=2

#Get block data from height
echo "===Get/Block==="
./nexus ledger/get/block height=2

#Get block data from block hash
echo "===Get/Block==="
./nexus ledger/get/block hash=ca9f6d89d3ab2f071686cb368682409386b5ba2d9bdc6dfffa64f84cf2016bf1df0538749f8ead32319460f28495dd887b824678336c7de1cca7c8764390c02276eb128eec7787418a2a689a432314a636d1925a43d1e6d5092ca02117cc18e6f73a44a2af3d5080d1386091f0260d77d397f381f4a525d8ffcf8be524a6ac75

#List 100 blocks starting at height 1
echo "===List/Blocks==="
./nexus ledger/list/blocks height=1 limit=100

#List the second page of 10 blocks starting at height 1 (blocks 21-30)
echo "===List/Blocks==="
./nexus ledger/list/blocks height=1 limit=10 page=2

#Get transaction data by hash
echo "===Get/Transaction==="
./nexus ledger/get/transaction hash=32104b0faac8c15bc5d7645f356e138297e7e6d7f8eadc3e8263d85adb30f42ec0880ab10e15a7ea2ff223f6a23e9f97a00bf2c4d393e658dec2b287bc0794d6

#Get raw transaction binary data by hash
echo "===Get/Transaction==="
./nexus ledger/get/transaction format=raw hash=32104b0faac8c15bc5d7645f356e138297e7e6d7f8eadc3e8263d85adb30f42ec0880ab10e15a7ea2ff223f6a23e9f97a00bf2c4d393e658dec2b287bc0794d6
