# Benchmark Comparison

*Generated from 4 compiler/variant combinations*

## compiler_comparison_bench / DispatchBaselines

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| if_else | 1.7 | 0.6 | 0.8 | 0.9 |
| switch | 1.7 | 0.6 | 0.8 | 0.9 |
| fn_ptr | 1.8 | 9.4 | 8.2 | 9.3 |
| POET | 1.9 | 9.3 | 8.2 | 9.4 |

## compiler_comparison_bench / Vectorization

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| saxpy_plain | 2494.0 | 4156.0 | 3501.5 | 4045.1 |
| saxpy_aligned | 2530.0 | 4151.3 | 3498.9 | 4042.0 |
| saxpy_restrict | 2492.5 | 4157.1 | 3495.3 | 4040.4 |

## compiler_comparison_bench / Sweep

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1-acc_N=64 | 76.8 | 60.1 | 61.4 | 80.2 |
| tuned-acc_N=64 | 40.8 | 46.7 | 39.7 | 50.8 |
| dynamic_for_N=64 | 72.6 | 51.0 | 34.5 | 42.2 |
| 1-acc_N=512 | 624.8 | 486.3 | 495.0 | 641.0 |
| tuned-acc_N=512 | 312.2 | 348.8 | 292.8 | 373.7 |
| dynamic_for_N=512 | 580.0 | 385.2 | 251.6 | 296.6 |
| 1-acc_N=4096 | 4915.8 | 3860.0 | 3939.1 | 5249.2 |
| tuned-acc_N=4096 | 2467.2 | 2765.6 | 2292.1 | 2943.7 |
| dynamic_for_N=4096 | 4581.9 | 3058.7 | 1970.2 | 2336.2 |
| 1-acc_N=32768 | 39193.3 | 30874.2 | 31264.6 | 40939.7 |
| tuned-acc_N=32768 | 19728.2 | 22102.0 | 18260.6 | 23521.8 |
| dynamic_for_N=32768 | 36561.8 | 24451.0 | 15741.5 | 18636.0 |

## compiler_comparison_bench / Inline

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_loop_N=4 | 0.1 | 0.2 | 0.3 | 0.3 |
| static_for_N=4 | 0.1 | 0.2 | 0.3 | 0.3 |
| plain_loop_N=8 | 0.1 | 0.2 | 0.3 | 0.3 |
| static_for_N=8 | 0.1 | 0.2 | 0.3 | 0.3 |
| plain_loop_N=16 | 0.1 | 0.2 | 0.3 | 0.3 |
| static_for_N=16 | 0.1 | 0.2 | 0.3 | 0.3 |

## dispatch_bench / Dispatch

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| 1D_contiguous_hit | 2.0 | 9.3 | 8.2 | 9.4 |
| 1D_contiguous_miss | 1.7 | 0.9 | 0.8 | 0.9 |
| 1D_non-contiguous_hit | 5.1 | 4.4 | 4.2 | 4.7 |
| 1D_non-contiguous_miss | 2.1 | 2.7 | 2.7 | 3.1 |
| 2D_contiguous_hit | 3.5 | 10.3 | 9.1 | 9.6 |
| 2D_contiguous_miss | 1.7 | 0.9 | 1.1 | 1.3 |
| 2D_non-contiguous_hit | 8.5 | 7.7 | 8.0 | 8.8 |
| 2D_non-contiguous_miss | 6.4 | 5.6 | 5.6 | 6.6 |
| 5D_contiguous_hit | 1.9 | 2.6 | 2.0 | 2.6 |
| 5D_contiguous_miss | 2.1 | 0.3 | 1.6 | 0.6 |
| 5D_non-contiguous_hit | 4.7 | 9.0 | 5.1 | 6.0 |
| 5D_non-contiguous_miss | 2.1 | 0.6 | 1.9 | 1.9 |
| 1D_strided-magic_hit | 1.9 | 7.6 | 7.6 | 8.2 |
| 1D_strided-magic_miss | 1.7 | 1.0 | 0.8 | 0.9 |
| set_hit | 2.0 | 2.4 | 2.5 | 3.2 |
| set_miss | 3.4 | 1.9 | 1.4 | 1.6 |
| set_wide_hit | 1.8 | 1.6 | 2.1 | 3.1 |
| set_wide_miss | 3.5 | 2.5 | 2.2 | 2.5 |

## dispatch_optimization_bench / Horner

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| N=4_runtime | 1.2 | 3.1 | 2.2 | 2.5 |
| N=4_dispatched | 0.6 | 0.8 | 0.7 | 0.8 |
| N=8_runtime | 1.7 | 3.4 | 2.9 | 3.3 |
| N=8_dispatched | 1.1 | 1.6 | 1.2 | 1.4 |
| N=16_runtime | 3.4 | 5.2 | 3.7 | 5.0 |
| N=16_dispatched | 2.4 | 3.1 | 2.6 | 3.0 |
| N=32_runtime | 8.4 | 12.2 | 7.9 | 10.1 |
| N=32_dispatched | 6.6 | 9.2 | 7.9 | 9.2 |

## dynamic_for_bench / Multi-acc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop_1_acc | 12206.8 | 9426.0 | 9558.3 | 12484.4 |
| for_loop_optimal_accs | 11152.2 | 7491.4 | 5532.9 | 6639.6 |
| dynamic_for_1_acc | 12024.9 | 9427.0 | 9549.0 | 12515.3 |
| dynamic_for_optimal_accs | 6031.7 | 6919.5 | 4917.8 | 5697.3 |

## dynamic_for_bench / Unroll

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12202.3 | 9424.8 | 9549.9 | 12463.0 |
| dynamic_for_optimal | 6032.4 | 6914.7 | 4917.5 | 5688.6 |
| dynamic_for_spill | 5529.9 | 7185.9 | 4474.2 | 5317.2 |

## dynamic_for_bench / HotArgs

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| capture_wide_accs | 11176.8 | 7480.9 | 4803.2 | 5720.6 |
| hot_args_wide_accs | 11161.1 | 7471.4 | 4803.9 | 5685.2 |

## dynamic_for_emission_bench / Heavy_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 11168.8 | 7490.6 | 5541.0 | 6645.2 |
| computed-index | 11130.9 | 7490.5 | 5536.9 | 6646.4 |
| dynamic_for_lane_form | 11158.7 | 7468.2 | 4806.2 | 5691.6 |

## dynamic_for_emission_bench / Light_body

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| carried-index | 6967.4 | 5810.0 | 2858.9 | 4070.1 |
| computed-index | 6994.1 | 5813.6 | 2861.7 | 4074.2 |
| dynamic_for_lane_form | 7007.4 | 5781.4 | 3591.4 | 4566.7 |

## dynamic_for_emission_bench / Stride

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| dynamic_for_CT_stride_2 | 5594.4 | 3741.2 | 2404.4 | 2845.7 |
| dynamic_for_RT_stride_2 | 5638.9 | 3739.6 | 2404.9 | 2846.6 |
| dynamic_for_RT_stride_3_opaque | 3836.8 | 2334.7 | 2374.2 | 2898.7 |

## dynamic_for_forms_bench / Accumulation

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_1_acc | 12107.1 | 9450.2 | 9559.3 | 12494.7 |
| dynamic_for_index_only_1_acc | 11157.9 | 11329.8 | 9202.5 | 11658.8 |
| dynamic_for_lane_form_optimal_accs | 11167.3 | 7470.2 | 4804.3 | 5689.3 |

## dynamic_for_forms_bench / Elementwise

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for | 4370.5 | 5328.4 | 3520.0 | 4452.7 |
| dynamic_for_index_only | 8030.1 | 9187.8 | 4804.3 | 6235.9 |
| dynamic_for_lane_form_unused_lane | 8027.8 | 9185.6 | 4792.8 | 6240.1 |

## dynamic_for_forms_bench / SmallN

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| plain_for_N=3 | 2.1 | 2.4 | 2.2 | 2.9 |
| dynamic_for_index_only_N=3 | 2.2 | 2.6 | 3.1 | 3.2 |
| dynamic_for_lane_form_N=3 | 2.3 | 2.7 | 3.1 | 3.2 |
| plain_for_N=7 | 5.4 | 7.3 | 5.9 | 8.0 |
| dynamic_for_index_only_N=7 | 5.2 | 6.8 | 6.8 | 8.5 |
| dynamic_for_lane_form_N=7 | 5.2 | 6.8 | 6.8 | 8.5 |

## static_for_bench / Map

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 267.3 | 388.4 | 313.8 | 373.3 |
| static_for_tuned_BS | 245.7 | 352.7 | 320.9 | 368.1 |
| static_for_default_BS | 421.6 | 788.9 | 291.8 | 347.5 |

## static_for_bench / MultiAcc

| Benchmark | gcc-14 default (ns) | gcc-15 default (ns) | clang-20 default (ns) | clang-21 default (ns) |
|:----------|--------:|--------:|--------:|--------:|
| for_loop | 319.6 | 241.5 | 244.9 | 324.9 |
| static_for_tuned_BS | 98.3 | 140.3 | 131.7 | 162.8 |
| static_for_default_BS | 453.3 | 914.7 | 317.0 | 376.9 |

