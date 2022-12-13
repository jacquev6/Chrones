# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

from __future__ import annotations
import os

from typing import List, Optional, Tuple
import dataclasses
import logging
import resource
import subprocess
import time

import psutil

from .result import (
    RunResults, RunSettings,
    System, SystemInstantMetrics,
    MainProcess, MainProcessGlobalMetrics,
    Process, ProcessInstantMetrics, MemoryInstantMetrics, InputOutputoInstantMetrics, ContextSwitchInstantMetrics,
)


@dataclasses.dataclass
class InProgressProcessInstantMetrics:
    timestamp: float
    threads: int
    cpu_percent: float
    user_time: float
    system_time: float
    memory: psutil._pslinux.pfullmem
    open_files: int
    io: psutil._pslinux.pio
    context_switches: psutil._common.pctxsw
    gpu_percent: Optional[float]
    gpu_memory: Optional[float]


@dataclasses.dataclass
class InProgressProcess:
    psutil_process: psutil.Process
    pid: int
    command: List[str]
    started_between_timestamps: Tuple[float, float]
    terminated_between_timestamps: Optional[Tuple[float, float]]
    children: List[InProgressProcess]
    instant_metrics: List[InProgressProcessInstantMetrics]


class Runner:
    def __init__(self, *, logs_directory, monitor_gpu, monitoring_interval, allowed_missing_samples):
        self.__monitoring_interval = monitoring_interval
        self.__logs_directory = logs_directory
        self.__monitor_gpu = monitor_gpu
        self.__allowed_missing_samples = allowed_missing_samples

    def run(self, command):
        return self.__Run(
            command,
            monitoring_interval=self.__monitoring_interval,
            logs_directory=self.__logs_directory,
            monitor_gpu=self.__monitor_gpu,
            allowed_missing_samples=self.__allowed_missing_samples,
        )()

    class __Run:
        def __init__(self, command, *, monitoring_interval, logs_directory, monitor_gpu, allowed_missing_samples):
            self.__command = command
            self.__monitoring_interval = monitoring_interval
            self.__logs_directory = logs_directory
            self.__monitor_gpu = monitor_gpu
            self.__allowed_missing_samples = allowed_missing_samples

            self.__usage_before = resource.getrusage(resource.RUSAGE_CHILDREN)
            self.__monitored_processes = {}
            self.__system_instant_metrics = []
            self.__usage_after = None

        def __call__(self):
            env = dict(os.environ)
            os.makedirs(self.__logs_directory, exist_ok=True)
            env["CHRONES_LOGS_DIRECTORY"] = os.path.abspath(self.__logs_directory)
            main_process = psutil.Popen(
                self.__command,
                env=env,
            )
            self.__timestamp = time.time()
            self.__previous_timestamp = self.__timestamp
            spawn_time = self.__timestamp

            main_process = self.__start_monitoring_process(main_process)
            iteration = 0

            while main_process.psutil_process.returncode is None:
                missing_samples = 0
                while True:
                    iteration += 1
                    target_timestamp = spawn_time + iteration * self.__monitoring_interval
                    timeout = target_timestamp - time.time()
                    if timeout > 0:
                        break
                    else:
                        missing_samples += 1
                try:
                    main_process.psutil_process.communicate(timeout=timeout)
                except subprocess.TimeoutExpired:
                    self.__previous_timestamp = self.__timestamp
                    self.__timestamp = time.time()
                    # Main process is still running
                    if missing_samples > self.__allowed_missing_samples:
                        logging.warn(f"Monitoring is slow. {missing_samples} samples will be missing just before t={self.__timestamp:.3f}s.")
                    self.__run_monitoring_iteration()
                else:
                    self.__previous_timestamp = self.__timestamp
                    self.__timestamp = time.time()
                    # Main process has terminated
                    self.__terminate()

            return RunResults(
                run_settings=RunSettings(gpu_monitored=self.__monitor_gpu),
                system=System(instant_metrics=[
                    SystemInstantMetrics(
                        timestamp=m.timestamp,
                        host_to_device_transfer_rate=m.host_to_device_transfer_rate,
                        device_to_host_transfer_rate=m.device_to_host_transfer_rate,
                    )
                    for m in self.__system_instant_metrics
                ]),
                main_process=self.__return_main_process(main_process, main_process.psutil_process.returncode),
            )

        def __start_monitoring_process(self, psutil_process):
            psutil_process.cpu_percent()  # Ignore first, meaningless 0.0 returned, as per https://psutil.readthedocs.io/en/latest/#psutil.Process.cpu_percent
            child = InProgressProcess(
                psutil_process=psutil_process,
                pid=psutil_process.pid,
                command=psutil_process.cmdline(),
                started_between_timestamps=(self.__previous_timestamp, self.__timestamp),
                terminated_between_timestamps=None,
                children=[],  # We'll detect children in the next iteration
                instant_metrics=[],  # We'll gather the first instant metrics in the next iteration
            )
            self.__monitored_processes[psutil_process.pid] = child
            parent = psutil_process.ppid()
            if parent in self.__monitored_processes:
                self.__monitored_processes[parent].children.append(child)
            return child

        def __run_monitoring_iteration(self):
            if self.__monitor_gpu:
                # Start sub-processes as early as possible to give them time to run while we do other stuff
                nvidia_smi_dmon = subprocess.Popen(
                    ["nvidia-smi", "dmon", "-c", "1", "-s", "t"],
                    stdout=subprocess.PIPE,
                    universal_newlines=True,
                )
                nvidia_smi_pmon = subprocess.Popen(
                    ["nvidia-smi", "pmon", "-c", "1", "-s", "um"],
                    stdout=subprocess.PIPE,
                    universal_newlines=True,
                )

            for process in list(self.__monitored_processes.values()):
                try:
                    self.__gather_instant_metrics(process)
                    self.__gather_children(process)
                except psutil.NoSuchProcess:
                    self.__stop_monitoring_process(process)

            if self.__monitor_gpu:
                # Use sub-processes as late as possible
                self.__gather_gpu_metrics(nvidia_smi_dmon, nvidia_smi_pmon)

        def __gather_gpu_metrics(self, nvidia_smi_dmon, nvidia_smi_pmon):
            (nvidia_smi_pmon_stdout, _) = nvidia_smi_pmon.communicate()
            lines = nvidia_smi_pmon_stdout.splitlines()
            parts = lines[0].split()
            assert parts[0] == "#"
            parts = parts[1:]
            assert parts[1] == "pid"
            assert parts[3] == "sm"
            assert parts[7] == "fb"

            for line in lines[2:]:
                parts = line.split()
                pid = int(parts[1])
                process = self.__monitored_processes.get(pid)
                if process is not None and len(process.instant_metrics) != 0:
                    metrics = process.instant_metrics[-1]
                    if metrics.timestamp == self.__timestamp:
                        try:
                            metrics.gpu_percent = float(parts[3])
                        except ValueError:
                            metrics.gpu_percent = 0.
                        try:
                            metrics.gpu_memory = float(parts[7])
                        except ValueError:
                            metrics.gpu_memory = 0.

            (nvidia_smi_dmon_stdout, _) = nvidia_smi_dmon.communicate()

            lines = nvidia_smi_dmon_stdout.splitlines()
            parts = lines[0].split()
            assert parts[0] == "#"
            parts = parts[1:]
            assert parts[0] == "gpu"
            assert parts[1] == "rxpci"
            assert parts[2] == "txpci"

            assert len(lines) == 3  # Only a single device is supported
            parts = lines[2].split()

            self.__system_instant_metrics.append(SystemInstantMetrics(
                timestamp=self.__timestamp,
                host_to_device_transfer_rate=float(parts[1]),
                device_to_host_transfer_rate=float(parts[2]),
            ))

        def __gather_instant_metrics(self, process):
            try:
                with process.psutil_process.oneshot():
                    cpu_times = process.psutil_process.cpu_times()
                    process.instant_metrics.append(InProgressProcessInstantMetrics(
                        timestamp=self.__timestamp,
                        threads=process.psutil_process.num_threads(),
                        cpu_percent=process.psutil_process.cpu_percent(),
                        user_time=cpu_times.user,
                        system_time=cpu_times.system,
                        memory=process.psutil_process.memory_full_info(),
                        open_files=len(process.psutil_process.open_files()),
                        io=process.psutil_process.io_counters(),
                        context_switches=process.psutil_process.num_ctx_switches(),
                        gpu_percent=None,
                        gpu_memory=None,
                    ))
            except psutil.AccessDenied:
                logging.warn(f"Exception 'psutil.AccessDenied' occurred. Instant metrics for {process.command} will be missing at t={self.__timestamp}s.")

        def __gather_children(self, process):
            for child in process.psutil_process.children():
                if child.pid not in self.__monitored_processes:
                    self.__start_monitoring_process(child)

        def __stop_monitoring_process(self, process):
            process.terminated_between_timestamps = (self.__previous_timestamp, self.__timestamp)
            del self.__monitored_processes[process.pid]

        def __terminate(self):
            self.__usage_after = resource.getrusage(resource.RUSAGE_CHILDREN)
            for process in self.__monitored_processes.values():
                if process.terminated_between_timestamps is None:
                    process.terminated_between_timestamps = (self.__previous_timestamp, self.__timestamp)

        def __return_main_process(self, process, exit_code):
            return MainProcess(
                command_list=self.__command,
                pid=process.psutil_process.pid,
                started_between_timestamps=process.started_between_timestamps,
                terminated_between_timestamps=process.terminated_between_timestamps,
                instant_metrics=self.__return_instant_metrics(process),
                children=[self.__return_process(child) for child in process.children],
                exit_code=exit_code,
                global_metrics=MainProcessGlobalMetrics(
                    # According to https://manpages.debian.org/bullseye/manpages-dev/getrusage.2.en.html,
                    # we don't care about these fields:
                    #   ru_ixrss ru_idrss ru_isrss ru_nswap ru_msgsnd ru_msgrcv ru_nsignals
                    # And as 'subprocess' uses fork, ru_maxrss often measures the memory usage of the Python
                    # interpreter, so we don't care about that field either.
                    user_time=self.__usage_after.ru_utime - self.__usage_before.ru_utime,
                    system_time=self.__usage_after.ru_stime - self.__usage_before.ru_stime,
                    minor_page_faults=self.__usage_after.ru_minflt - self.__usage_before.ru_minflt,
                    major_page_faults=self.__usage_after.ru_majflt - self.__usage_before.ru_majflt,
                    input_blocks=self.__usage_after.ru_inblock - self.__usage_before.ru_inblock,
                    output_blocks=self.__usage_after.ru_oublock - self.__usage_before.ru_oublock,
                    voluntary_context_switches=self.__usage_after.ru_nvcsw - self.__usage_before.ru_nvcsw,
                    involuntary_context_switches=self.__usage_after.ru_nivcsw - self.__usage_before.ru_nivcsw,
                ),
            )

        def __return_instant_metrics(self, process):
            return [
                ProcessInstantMetrics(
                    timestamp=m.timestamp,
                    threads=m.threads,
                    cpu_percent=m.cpu_percent,
                    user_time=m.user_time,
                    system_time=m.system_time,
                    memory=MemoryInstantMetrics(rss=m.memory.rss),
                    open_files=m.open_files,
                    io=InputOutputoInstantMetrics(read_chars=m.io.read_chars, write_chars=m.io.write_chars),
                    context_switches=ContextSwitchInstantMetrics(voluntary=m.context_switches.voluntary, involuntary=m.context_switches.involuntary),
                    gpu_percent=m.gpu_percent,
                    gpu_memory=m.gpu_memory,
                )
                for m in process.instant_metrics
            ]

        def __return_process(self, process: InProgressProcess):
            return Process(
                command_list=process.command,
                pid=process.psutil_process.pid,
                started_between_timestamps=process.started_between_timestamps,
                terminated_between_timestamps=process.terminated_between_timestamps,
                instant_metrics=self.__return_instant_metrics(process),
                children=[self.__return_process(child) for child in process.children],
            )
