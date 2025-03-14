#!/bin/bash

# Function to run tests with a specific scheduler
run_tests() {
    scheduler=$1
    output_file="results_${scheduler}.txt"
    
    echo "Running tests with ${scheduler} scheduler..."
    echo "Results will be saved to ${output_file}"
    
    # Compile with appropriate scheduler
    if [ "$scheduler" = "default" ]; then
        make clean && make
    elif [ "$scheduler" = "fifo" ]; then
        make clean && make SCHEDULER=FIFO
    elif [ "$scheduler" = "lottery" ]; then
        make clean && make SCHEDULER=LOTTERY
    fi
    
    # Start QEMU and run tests
    (echo "scheduler_test" ; sleep 30) | make qemu-nox > "$output_file" 2>&1
    
    echo "Tests completed for ${scheduler} scheduler"
    echo "----------------------------------------"
}

# Create results directory
mkdir -p test_results

# Run tests with each scheduler
run_tests "default"
run_tests "fifo"
run_tests "lottery"

# Process and compare results
echo "Generating comparison report..."
python3 analyze_results.py

echo "All tests completed!" 