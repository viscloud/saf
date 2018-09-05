#!/usr/bin/env bash

# Copyright 2018 The SAF Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Taken from https://devtalk.nvidia.com/default/topic/935300/jetson-tx1/deep-learning-inference-performance-validation-on-tx1/post/4876280/#4876280
# This is used to enable max performance on Tegra

echo "Enabling fan for safety..."
if [ ! -w /sys/kernel/debug/tegra_fan/target_pwm   ] ; then
  -1echo "Cannot set fan -- exiting..."
fi
echo 255 > /sys/kernel/debug/tegra_fan/target_pwm
echo "Set Tegra CPUs to max freq"
echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo userspace > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo userspace > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo userspace > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
cat /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
cat /sys/devices/system/cpu/cpu2/cpufreq/scaling_max_freq > /sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq
cat /sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq > /sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq
echo "Disable Tegra CPUs quite and set current gov to runnable"
echo 0 > /sys/devices/system/cpu/cpuquiet/tegra_cpuquiet/enable
echo runnable > /sys/devices/system/cpu/cpuquiet/current_governor
echo "Set Max GPU rate"
echo 844800000 > /sys/kernel/debug/clock/override.gbus/rate
echo 1 > /sys/kernel/debug/clock/override.gbus/state
# burst EMC freq to top
echo "Set Max EMC rate"
echo 1 > /sys/kernel/debug/clock/override.emc/state
cat /sys/kernel/debug/clock/emc/max > /sys/kernel/debug/clock/override.emc/rate
