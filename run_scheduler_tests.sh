#!/bin/bash

# Function to run tests for a specific scheduler
run_scheduler_test() {
    scheduler=$1
    echo "Testing $scheduler scheduler..."
    
    # Clean and compile with specific scheduler
    make clean
    if [ "$scheduler" = "default" ]; then
        make qemu-nox &
    else
        make SCHEDULER=$scheduler qemu-nox &
    fi
    
    # Wait for QEMU to start
    sleep 2
    
    # Run the test program
    echo "scheduler_perf_test" > /dev/pts/0
    
    # Wait for test to complete and capture output
    sleep 10
    
    # Kill QEMU
    pkill qemu
    
    echo "Test completed for $scheduler scheduler"
    echo "----------------------------------------"
}

# Create results directory
mkdir -p results

# Run tests for each scheduler
run_scheduler_test "default"
run_scheduler_test "FIFO"
run_scheduler_test "LOTTERY"

echo "All tests completed!" 