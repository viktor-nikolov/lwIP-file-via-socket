# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\MicroZed_projects\Zybo-Z7\DemoFileViaSocket\system\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\MicroZed_projects\Zybo-Z7\DemoFileViaSocket\system\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {system}\
-hw {C:\GitHub\lwIP-file-via-socket\demo_app_FreeRTOS_on_Zynq\project_files\Zybo-Z7_hw\system_wrapper.xsa}\
-proc {ps7_cortexa9_0} -os {freertos10_xilinx} -out {C:/MicroZed_projects/Zybo-Z7/DemoFileViaSocket}

platform write
platform generate -domains 
platform active {system}
domain active {zynq_fsbl}
bsp reload
bsp setlib -name lwip213 -ver 1.0
bsp config api_mode "SOCKET_API"
bsp config lwip_dhcp "true"
bsp config phy_link_speed "CONFIG_LINKSPEED1000"
bsp write
bsp reload
catch {bsp regenerate}
domain active {freertos10_xilinx_domain}
bsp reload
bsp reload
domain active {zynq_fsbl}
bsp removelib -name lwip213
bsp write
bsp reload
catch {bsp regenerate}
domain active {freertos10_xilinx_domain}
bsp setlib -name lwip213 -ver 1.0
bsp config api_mode "SOCKET_API"
bsp config lwip_dhcp "true"
bsp config phy_link_speed "CONFIG_LINKSPEED1000"
bsp write
bsp reload
catch {bsp regenerate}
platform generate
platform generate -domains freertos10_xilinx_domain 
platform clean
platform generate
platform generate -domains freertos10_xilinx_domain 
platform clean
platform generate
platform clean
