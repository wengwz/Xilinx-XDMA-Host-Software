#/bin/bash

trans_size=2048
trans_num=16
total_size=$[$trans_size*$trans_num]
channel_idx=0

h2c_channel=/dev/xdma0_h2c_$channel_idx
c2h_channel=/dev/xdma0_c2h_$channel_idx

bin_path=./bin
data_path=./data

out_data_file=$data_path/output_data${total_size}.bin
in_data_file=$data_path/input_data${total_size}.bin

testError=0

echo "Info: Running PCIe DMA Streaming Loopback Test"
echo "      transaction size: $trans_size"
echo "      transaction num : $trans_num"
echo "      total size      : $total_size"

# Generate random data
make data
mkdir -p data
echo "Info: Generating Random Data of $total_size bytes"

$bin_path/gen_random $total_size $in_data_file


# Setup the DMA c2h channel to wait for incomming data from the h2c channels.
rm -f $out_data_file
echo "Info: DMA setup to read from $c2h_channel. Waiting on write data to $h2c_channel"
$bin_path/xdma_rw -r -d $c2h_channel -f $out_data_file -s $trans_size -c $trans_num &

# Wait to make sure the DMA is ready to receive data.
sleep 2s

# Setup the DMA to write to the h2c channels. Data will be push out the h2c channel
# and then read back through the c2h channel and written to the output data file.

echo "Info: Writing to $h2c_channel. This will also start reading data on $c2h_channel."
$bin_path/xdma_rw -w -d $h2c_channel -f $in_data_file -s $trans_size -c $trans_num &


echo "Info: Wait the for current transactions to complete."
wait

# Verify that the written data matches the read data.
echo "Info: Checking data integrity."
cmp $in_data_file $out_data_file
returnVal=$?
if [ ! $returnVal == 0 ]; then
    echo "Error: The data written did not match the data that was read."
    echo "       write data file: $in_data_file"
    echo "       read data file:  $out_data_file"
    testError=1
else
    echo "Info: Data check passed for c2h and h2c channel $channel_idx."
fi

# Exit with an error code if an error was found during testing
if [ $testError -eq 1 ]; then
  echo "Error: Test completed with Errors."
  exit 1
fi

# Report all tests passed and exit
echo "Info: PCIe DMA streaming loopback tests passed."
exit 0