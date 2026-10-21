# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.7 | 0.6 | 0.8 | 0.5 |
| switch | 0.6 | 0.6 | 0.8 | 0.7 |
| fn_ptr | 9.3 | 7.9 | 8.2 | 1.7 |
| POET | 9.3 | 8.2 | 8.2 | 1.7 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4080.7 | 3491.9 | 3511.4 | 2032.3 |
| saxpy_aligned | 4092.4 | 3489.0 | 3504.5 | 2336.8 |
| saxpy_restrict | 4084.0 | 3499.1 | 3498.9 | 2014.3 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.7 | 50.3 | 61.4 | 41.2 |
| tuned-acc_N=64 | 54.6 | 37.5 | 39.7 | 32.5 |
| dynamic_for_N=64 | 76.2 | 43.7 | 34.6 | 27.1 |
| 1-acc_N=512 | 629.2 | 415.3 | 495.7 | 340.0 |
| tuned-acc_N=512 | 410.8 | 281.6 | 292.4 | 240.4 |
| dynamic_for_N=512 | 581.2 | 330.9 | 250.6 | 207.6 |
| 1-acc_N=4096 | 5011.8 | 3347.0 | 3936.8 | 3935.0 |
| tuned-acc_N=4096 | 3260.3 | 2274.1 | 2300.8 | 1936.5 |
| dynamic_for_N=4096 | 4623.9 | 2633.7 | 2091.2 | 1541.2 |
| 1-acc_N=32768 | 40130.1 | 26813.5 | 31307.7 | 21359.6 |
| tuned-acc_N=32768 | 26000.7 | 17869.6 | 18272.1 | 15214.7 |
| dynamic_for_N=32768 | 36968.8 | 20992.5 | 15732.0 | 12290.7 |

## compiler_comparison_bench / Inline

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_loop_N=4 | 0.2 | 0.1 | 0.3 | 0.2 |
| static_for_N=4 | 0.2 | 0.1 | 0.3 | 0.3 |
| plain_loop_N=8 | 0.2 | 0.1 | 0.3 | 0.2 |
| static_for_N=8 | 0.2 | 0.1 | 0.3 | 0.2 |
| plain_loop_N=16 | 0.2 | 0.1 | 0.3 | 0.2 |
| static_for_N=16 | 0.2 | 0.1 | 0.3 | 0.2 |

## dispatch_bench / Dispatch

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1D_contiguous_hit | 9.3 | 8.2 | 8.2 | 1.4 |
| 1D_contiguous_miss | 0.6 | 0.8 | 0.8 | 0.5 |
| 1D_non-contiguous_hit | 5.1 | 3.6 | 4.1 | 3.3 |
| 1D_non-contiguous_miss | 2.7 | 1.9 | 2.7 | 2.6 |
| 2D_contiguous_hit | 10.0 | 6.9 | 9.0 | 1.6 |
| 2D_contiguous_miss | 0.9 | 0.8 | 1.1 | 0.7 |
| 2D_non-contiguous_hit | 8.5 | 6.5 | 8.1 | 6.6 |
| 2D_non-contiguous_miss | 6.5 | 4.6 | 5.6 | 5.1 |
| 5D_contiguous_hit | 2.6 | 2.0 | 2.0 | 1.7 |
| 5D_contiguous_miss | 0.3 | 0.3 | 1.6 | 0.3 |
| 5D_non-contiguous_hit | 10.6 | 5.7 | 5.2 | 3.7 |
| 5D_non-contiguous_miss | 0.3 | 0.5 | 1.7 | 1.2 |
| 1D_strided-magic_hit | 7.9 | 6.8 | 7.8 | 1.1 |
| 1D_strided-magic_miss | 1.0 | 0.6 | 0.8 | 0.5 |
| set_hit | 2.8 | 1.8 | 2.5 | 1.3 |
| set_miss | 1.9 | 1.1 | 1.4 | 1.2 |
| set_wide_hit | 1.5 | 1.4 | 2.1 | 1.2 |
| set_wide_miss | 2.2 | 1.9 | 2.2 | 1.2 |

## dispatch_optimization_bench / Horner

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| N=4_runtime | 2.1 | 2.0 | 2.2 | 1.4 |
| N=4_dispatched | 0.9 | 0.7 | 0.7 | 0.6 |
| N=8_runtime | 3.0 | 3.1 | 2.8 | 1.5 |
| N=8_dispatched | 1.4 | 1.2 | 1.2 | 1.1 |
| N=16_runtime | 4.7 | 3.9 | 3.7 | 2.3 |
| N=16_dispatched | 3.1 | 2.6 | 2.6 | 2.1 |
| N=32_runtime | 12.3 | 9.9 | 7.9 | 6.4 |
| N=32_dispatched | 9.2 | 7.9 | 7.9 | 6.6 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12582.9 | 8175.2 | 9547.4 | 6545.6 |
| for_loop_optimal_accs | 11337.5 | 6446.8 | 5558.7 | 4491.6 |
| dynamic_for_1_acc | 12228.8 | 8194.6 | 9550.4 | 7591.8 |
| dynamic_for_optimal_accs | 7944.8 | 5626.0 | 4916.2 | 3774.3 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12560.6 | 8177.1 | 9558.0 | 6756.1 |
| dynamic_for_optimal | 7948.0 | 5630.9 | 4918.8 | 4029.3 |
| dynamic_for_spill | 7019.5 | 5592.7 | 4478.8 | 3352.7 |

## dynamic_for_bench / HotArgs

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| capture_wide_accs | 11272.1 | 6482.6 | 4822.3 | 4521.3 |
| hot_args_wide_accs | 11275.3 | 6396.6 | 4882.6 | 3842.8 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11371.7 | 6445.8 | 5535.2 | 4554.6 |
| computed-index | 11243.4 | 6434.5 | 5532.8 | 4511.1 |
| dynamic_for_lane_form | 11419.3 | 6401.8 | 4802.1 | 3775.2 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 7833.0 | 5260.9 | 2861.7 | 2421.5 |
| computed-index | 7806.9 | 5252.9 | 2858.2 | 2433.1 |
| dynamic_for_lane_form | 7900.5 | 5233.8 | 3602.9 | 2628.3 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5623.6 | 3205.3 | 2404.0 | 1870.1 |
| dynamic_for_RT_stride_2 | 5626.5 | 3202.5 | 2403.6 | 1892.5 |
| dynamic_for_RT_stride_3_opaque | 3815.4 | 1886.5 | 2372.6 | 1956.5 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12247.0 | 8176.2 | 10161.7 | 6629.9 |
| dynamic_for_index_only_1_acc | 11214.7 | 9338.2 | 9208.1 | 7203.0 |
| dynamic_for_lane_form_optimal_accs | 11267.9 | 6398.5 | 4807.5 | 3749.2 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5365.5 | 3927.4 | 3505.7 | 2977.9 |
| dynamic_for_index_only | 11686.0 | 6092.9 | 4779.5 | 4045.2 |
| dynamic_for_lane_form_unused_lane | 11691.3 | 6094.3 | 4780.4 | 4070.4 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.2 | 2.2 | 1.7 |
| dynamic_for_index_only_N=3 | 3.1 | 2.3 | 3.1 | 2.3 |
| dynamic_for_lane_form_N=3 | 2.8 | 2.3 | 3.1 | 2.3 |
| plain_for_N=7 | 7.4 | 5.7 | 5.9 | 5.1 |
| dynamic_for_index_only_N=7 | 6.9 | 5.6 | 6.8 | 5.6 |
| dynamic_for_lane_form_N=7 | 7.1 | 5.6 | 6.8 | 5.8 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 391.0 | 328.2 | 308.5 | 181.0 |
| static_for_tuned_BS | 352.3 | 290.9 | 306.8 | 184.5 |
| static_for_default_BS | 828.2 | 523.3 | 292.0 | 179.2 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 318.2 | 206.1 | 244.7 | 173.5 |
| static_for_tuned_BS | 140.1 | 119.9 | 133.6 | 98.7 |
| static_for_default_BS | 917.0 | 640.0 | 316.0 | 193.0 |

