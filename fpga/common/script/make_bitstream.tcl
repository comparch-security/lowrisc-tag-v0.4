set origin_dir "."
set project_name [lindex $argv 0]
set orig_proj_dir [file normalize $origin_dir/$project_name]

# open project
open_project $orig_proj_dir/$project_name.xpr

# suppress some not very useful messages
# IP flow regeneration
set_msg_config -id "\[IP_Flow 19-3664\]" -suppress

# reset runs
reset_run synth_1
reset_run impl_1

# run syntesis
launch_runs synth_1
wait_on_run synth_1

# run imp
set_property strategy Performance_ExtraTimingOpt [get_runs impl_1]
set_property STEPS.POST_ROUTE_PHYS_OPT_DESIGN.IS_ENABLED true [get_runs impl_1]
set_property STEPS.POST_ROUTE_PHYS_OPT_DESIGN.ARGS.DIRECTIVE Explore [get_runs impl_1]
launch_runs impl_1 -to_step write_bitstream
wait_on_run impl_1

