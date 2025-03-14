#!/usr/bin/env python3

import re
import numpy as np
from tabulate import tabulate
import matplotlib.pyplot as plt

def parse_results(filename):
    results = {
        'single_cpu': [],
        'single_io': [],
        'single_mixed': [],
        'concurrent_cpu': [],
        'concurrent_io': [],
        'concurrent_mixed': []
    }
    
    try:
        with open(filename, 'r') as f:
            content = f.read()
            
            # Extract single process results
            single_tests = re.findall(r'Process \d+:\n\s+Runtime: (\d+) ticks\n\s+CPU ticks: (\d+)', content)
            if len(single_tests) >= 3:
                results['single_cpu'] = [int(single_tests[0][0]), int(single_tests[0][1])]
                results['single_io'] = [int(single_tests[1][0]), int(single_tests[1][1])]
                results['single_mixed'] = [int(single_tests[2][0]), int(single_tests[2][1])]
            
            # Extract concurrent test results
            concurrent_tests = re.findall(r'Total time for 5 processes: (\d+) ticks', content)
            if len(concurrent_tests) >= 3:
                results['concurrent_cpu'] = [int(concurrent_tests[0])]
                results['concurrent_io'] = [int(concurrent_tests[1])]
                results['concurrent_mixed'] = [int(concurrent_tests[2])]
    
    except FileNotFoundError:
        print(f"Warning: {filename} not found")
    
    return results

def generate_comparison_tables(default_results, fifo_results, lottery_results):
    # Single process comparison
    single_headers = ['Test Type', 'Default Runtime', 'FIFO Runtime', 'Lottery Runtime']
    single_data = [
        ['CPU-bound', default_results['single_cpu'][0], fifo_results['single_cpu'][0], lottery_results['single_cpu'][0]],
        ['I/O-bound', default_results['single_io'][0], fifo_results['single_io'][0], lottery_results['single_io'][0]],
        ['Mixed', default_results['single_mixed'][0], fifo_results['single_mixed'][0], lottery_results['single_mixed'][0]]
    ]
    
    # Concurrent process comparison
    concurrent_headers = ['Test Type', 'Default Runtime', 'FIFO Runtime', 'Lottery Runtime']
    concurrent_data = [
        ['CPU-bound', default_results['concurrent_cpu'][0], fifo_results['concurrent_cpu'][0], lottery_results['concurrent_cpu'][0]],
        ['I/O-bound', default_results['concurrent_io'][0], fifo_results['concurrent_io'][0], lottery_results['concurrent_io'][0]],
        ['Mixed', default_results['concurrent_mixed'][0], fifo_results['concurrent_mixed'][0], lottery_results['concurrent_mixed'][0]]
    ]
    
    return single_data, concurrent_data

def generate_report(default_results, fifo_results, lottery_results):
    single_data, concurrent_data = generate_comparison_tables(default_results, fifo_results, lottery_results)
    
    with open('scheduler_comparison.md', 'w') as f:
        f.write('# Scheduler Performance Comparison Report\n\n')
        
        f.write('## Single Process Performance\n\n')
        f.write(tabulate(single_data, headers=['Test Type', 'Default Runtime', 'FIFO Runtime', 'Lottery Runtime'], tablefmt='pipe'))
        f.write('\n\n')
        
        f.write('## Concurrent Process Performance\n\n')
        f.write(tabulate(concurrent_data, headers=['Test Type', 'Default Runtime', 'FIFO Runtime', 'Lottery Runtime'], tablefmt='pipe'))
        f.write('\n\n')
        
        # Analysis
        f.write('## Analysis\n\n')
        
        # Calculate averages
        def calc_avg(data):
            return np.mean([row[1:] for row in data], axis=0)
        
        single_avgs = calc_avg(single_data)
        concurrent_avgs = calc_avg(concurrent_data)
        
        f.write('### Single Process Performance Analysis\n\n')
        f.write(f'- Default Scheduler Average: {single_avgs[0]:.2f} ticks\n')
        f.write(f'- FIFO Scheduler Average: {single_avgs[1]:.2f} ticks\n')
        f.write(f'- Lottery Scheduler Average: {single_avgs[2]:.2f} ticks\n\n')
        
        f.write('### Concurrent Process Performance Analysis\n\n')
        f.write(f'- Default Scheduler Average: {concurrent_avgs[0]:.2f} ticks\n')
        f.write(f'- FIFO Scheduler Average: {concurrent_avgs[1]:.2f} ticks\n')
        f.write(f'- Lottery Scheduler Average: {concurrent_avgs[2]:.2f} ticks\n\n')
        
        # Observations
        f.write('### Key Observations\n\n')
        f.write('1. Single Process Performance:\n')
        best_single = ['Default', 'FIFO', 'Lottery'][np.argmin(single_avgs)]
        f.write(f'   - {best_single} scheduler performs best for single process workloads\n')
        f.write('   - Possible reasons: [Analysis based on actual results]\n\n')
        
        f.write('2. Concurrent Process Performance:\n')
        best_concurrent = ['Default', 'FIFO', 'Lottery'][np.argmin(concurrent_avgs)]
        f.write(f'   - {best_concurrent} scheduler performs best for concurrent workloads\n')
        f.write('   - Possible reasons: [Analysis based on actual results]\n\n')
        
        f.write('3. Workload-specific Performance:\n')
        f.write('   - CPU-bound tasks: [Analysis based on actual results]\n')
        f.write('   - I/O-bound tasks: [Analysis based on actual results]\n')
        f.write('   - Mixed workloads: [Analysis based on actual results]\n\n')

def main():
    # Parse results
    default_results = parse_results('results_default.txt')
    fifo_results = parse_results('results_fifo.txt')
    lottery_results = parse_results('results_lottery.txt')
    
    # Generate report
    generate_report(default_results, fifo_results, lottery_results)
    
    print("Report generated: scheduler_comparison.md")

if __name__ == "__main__":
    main() 