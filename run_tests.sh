#!/bin/bash

# 测试默认调度器
echo "Testing default scheduler..."
make clean
make qemu-nox << EOF
scheduler_test
EOF

# 测试FIFO调度器
echo "Testing FIFO scheduler..."
make clean
make SCHEDULER=FIFO qemu-nox << EOF
scheduler_test
EOF

# 测试彩票调度器
echo "Testing Lottery scheduler..."
make clean
make SCHEDULER=LOTTERY qemu-nox << EOF
scheduler_test
EOF 