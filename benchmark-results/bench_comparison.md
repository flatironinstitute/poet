# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 0.6 | 0.6 | 1.1 | 0.9 |
| switch | 0.6 | 0.6 | 1.1 | 0.9 |
| fn_ptr | 9.3 | 9.3 | 10.7 | 9.3 |
| POET | 9.3 | 9.3 | 10.6 | 9.6 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 4042.7 | 4060.5 | 4525.9 | 3915.9 |
| saxpy_aligned | 4045.9 | 4061.6 | 4531.0 | 3843.1 |
| saxpy_restrict | 4034.3 | 4061.7 | 4515.5 | 4011.8 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 78.7 | 60.1 | 79.2 | 77.2 |
| tuned-acc_N=64 | 54.6 | 46.7 | 50.3 | 47.4 |
| dynamic_for_N=64 | 75.3 | 46.5 | 49.9 | 43.8 |
| 1-acc_N=512 | 629.1 | 482.1 | 638.8 | 601.7 |
| tuned-acc_N=512 | 410.7 | 348.7 | 369.3 | 360.1 |
| dynamic_for_N=512 | 579.2 | 382.0 | 369.9 | 326.5 |
| 1-acc_N=4096 | 5014.7 | 3863.1 | 5056.9 | 4890.4 |
| tuned-acc_N=4096 | 3259.6 | 2764.9 | 2910.4 | 2752.4 |
| dynamic_for_N=4096 | 4613.0 | 3052.9 | 2931.8 | 2649.7 |
| 1-acc_N=32768 | 40094.0 | 31054.5 | 40465.6 | 38697.1 |
| tuned-acc_N=32768 | 25995.7 | 22096.0 | 23298.0 | 22411.3 |
| dynamic_for_N=32768 | 36917.1 | 24385.2 | 23449.4 | 21727.6 |

## compiler_comparison_bench / Inline

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_loop_N=4 | 0.2 | 0.2 | 0.4 | 0.3 |
| static_for_N=4 | 0.2 | 0.2 | 0.4 | 0.3 |
| plain_loop_N=8 | 0.2 | 0.2 | 0.4 | 0.3 |
| static_for_N=8 | 0.2 | 0.2 | 0.4 | 0.3 |
| plain_loop_N=16 | 0.2 | 0.2 | 0.4 | 0.3 |
| static_for_N=16 | 0.2 | 0.2 | 0.4 | 0.3 |

## dispatch_bench / Dispatch

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1D_contiguous_hit | 9.5 | 9.4 | 10.5 | 8.8 |
| 1D_contiguous_miss | 0.6 | 0.6 | 1.1 | 0.9 |
| 1D_non-contiguous_hit | 4.9 | 4.7 | 5.0 | 4.5 |
| 1D_non-contiguous_miss | 2.6 | 2.7 | 3.5 | 3.4 |
| 2D_contiguous_hit | 10.0 | 9.9 | 11.6 | 9.3 |
| 2D_contiguous_miss | 0.9 | 0.9 | 1.4 | 1.2 |
| 2D_non-contiguous_hit | 7.7 | 7.7 | 9.6 | 8.3 |
| 2D_non-contiguous_miss | 5.3 | 5.7 | 7.1 | 6.4 |
| 5D_contiguous_hit | 2.6 | 2.6 | 2.5 | 2.5 |
| 5D_contiguous_miss | 0.6 | 0.3 | 2.1 | 0.6 |
| 5D_non-contiguous_hit | 8.9 | 10.0 | 7.4 | 6.5 |
| 5D_non-contiguous_miss | 0.3 | 0.6 | 2.1 | 0.6 |

## dispatch_optimization_bench / Horner

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| N=4_runtime | 2.1 | 3.1 | 2.8 | 2.4 |
| N=4_dispatched | 0.9 | 0.8 | 0.9 | 0.8 |
| N=8_runtime | 3.0 | 3.3 | 3.6 | 3.2 |
| N=8_dispatched | 1.4 | 1.6 | 1.6 | 1.3 |
| N=16_runtime | 4.6 | 5.2 | 4.7 | 4.7 |
| N=16_dispatched | 3.1 | 3.1 | 3.4 | 2.8 |
| N=32_runtime | 12.3 | 12.1 | 10.2 | 9.9 |
| N=32_dispatched | 9.2 | 9.2 | 10.2 | 9.2 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12383.4 | 9419.8 | 12340.9 | 11699.5 |
| for_loop_optimal_accs | 11217.9 | 7496.8 | 7116.2 | 6270.4 |
| dynamic_for_1_acc | 12545.3 | 9424.1 | 12315.7 | 11800.4 |
| dynamic_for_optimal_accs | 7954.6 | 6738.4 | 7097.1 | 6425.0 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12230.0 | 9418.4 | 12309.6 | 11797.7 |
| dynamic_for_optimal | 7952.4 | 6743.2 | 7101.3 | 6379.5 |
| dynamic_for_spill | 6953.8 | 7018.2 | 5583.8 | 5497.7 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11218.5 | 7487.2 | 7122.3 | 6377.0 |
| computed-index | 11334.5 | 7483.7 | 7129.8 | 6642.7 |
| dynamic_for_lane_form | 11262.5 | 7482.6 | 7162.9 | 6433.2 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 8183.6 | 5802.1 | 3677.8 | 3815.1 |
| computed-index | 8146.0 | 5809.1 | 3678.3 | 4069.6 |
| dynamic_for_lane_form | 8184.1 | 5809.5 | 3676.2 | 3913.4 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5653.3 | 3730.6 | 3583.7 | 3204.1 |
| dynamic_for_RT_stride_2 | 5650.2 | 3735.6 | 3583.6 | 3121.7 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12292.2 | 9419.9 | 12319.2 | 11901.1 |
| dynamic_for_index_only_1_acc | 11073.4 | 11196.0 | 11446.6 | 11531.4 |
| dynamic_for_lane_form_optimal_accs | 11231.5 | 7482.1 | 7148.8 | 6502.2 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 5370.1 | 5327.3 | 4426.7 | 4229.4 |
| dynamic_for_index_only | 11554.7 | 9136.8 | 6063.5 | 5985.8 |
| dynamic_for_lane_form_unused_lane | 11557.7 | 9131.4 | 6041.0 | 5814.7 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.5 | 2.5 | 2.8 | 2.8 |
| dynamic_for_index_only_N=3 | 3.1 | 3.1 | 4.0 | 3.2 |
| dynamic_for_lane_form_N=3 | 2.8 | 3.1 | 4.0 | 3.1 |
| plain_for_N=7 | 7.4 | 7.4 | 7.7 | 7.6 |
| dynamic_for_index_only_N=7 | 6.7 | 6.7 | 8.7 | 8.0 |
| dynamic_for_lane_form_N=7 | 6.5 | 6.7 | 8.7 | 8.5 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 391.1 | 388.6 | 406.7 | 336.4 |
| static_for_tuned_BS | 352.9 | 352.3 | 405.5 | 343.0 |
| static_for_default_BS | 828.2 | 790.4 | 371.7 | 332.1 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 317.0 | 241.3 | 315.6 | 326.3 |
| static_for_tuned_BS | 140.2 | 140.9 | 589.2 | 161.0 |
| static_for_default_BS | 928.3 | 914.9 | 406.8 | 351.8 |

