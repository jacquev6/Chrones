# Copyright 2022 Vincent Jacques

import logging
from typing import List, Dict, Optional

import dataclasses
import resource
import subprocess
import time

import psutil



@dataclasses.dataclass
class InProgressInstantRunMetrics:
    iteration: int
    threads: int
    cpu_percent: float
    user_time: float
    system_time: float
    memory: psutil._pslinux.pfullmem
    open_files: int
    io: psutil._pslinux.pio
    context_switches: psutil._common.pctxsw
    gpu_percent: Optional[float]
    gpu_memory: Optional[int]


@dataclasses.dataclass
class InProgressProcess:
    psutil_process: psutil.Process
    pid: int
    command: List[str]
    spawned_at_iteration: int
    children: List["InProgressProcess"]
    instant_metrics: List[InProgressInstantRunMetrics]
    terminated_at_iteration: Optional[int]


@dataclasses.dataclass
class InProgressSystemInstantMetrics:
    iteration: int
    host_to_device_transfer_rate: float
    device_to_host_transfer_rate: float


@dataclasses.dataclass
class InstantRunMetrics:
    timestamp: float
    threads: int
    cpu_percent: float
    user_time: float
    system_time: float
    memory: Dict
    open_files: int
    io: Dict
    context_switches: Dict
    gpu_percent: float
    gpu_memory: int


@dataclasses.dataclass
class GlobalMetrics:
    duration: float


@dataclasses.dataclass
class Process:
    command: List[str]
    spawned_at: float
    terminated_at: float
    instant_metrics: List[InstantRunMetrics]
    children: List["Process"]
    global_metrics: GlobalMetrics


@dataclasses.dataclass
class MainProcessGlobalMetrics(GlobalMetrics):
    user_time: float
    system_time: float
    minor_page_faults: int
    major_page_faults: int
    input_blocks: int
    output_blocks: int
    voluntary_context_switches: int
    involuntary_context_switches: int


@dataclasses.dataclass
class MainProcess(Process):
    exit_code: int
    global_metrics: MainProcessGlobalMetrics


@dataclasses.dataclass
class SystemInstantMetrics:
    timestamp: float
    host_to_device_transfer_rate: float
    device_to_host_transfer_rate: float


@dataclasses.dataclass
class SystemMetrics:
    instant_metrics: List[SystemInstantMetrics]


@dataclasses.dataclass
class RunResults:
    system: SystemMetrics
    main_process: MainProcess
    stdout: Optional[str]
    stderr: Optional[str]


class Runner:
    def __init__(self, *, interval, allowed_missing_samples=1, clear_io_caches=True):
        self.__interval = interval
        self.__allowed_missing_samples = allowed_missing_samples
        self.__clear_io_caches = clear_io_caches

    def run(self, *args, **kwds):
        if self.__clear_io_caches:
            # https://stackoverflow.com/a/25336215/905845
            subprocess.run("sync; echo 3 | sudo tee /proc/sys/vm/drop_caches", shell=True, check=True, capture_output=True)

        return self.__Run(self.__interval, self.__allowed_missing_samples, *args, **kwds)()

    class __Run:
        def __init__(self, interval, allowed_missing_samples, *args, capture_output=False, **kwds):
            self.__interval = interval
            self.__allowed_missing_samples = allowed_missing_samples
            self.__args = args
            self.__capture_output = capture_output
            self.__kwds = kwds

            self.__usage_before = resource.getrusage(resource.RUSAGE_CHILDREN)
            self.__iteration = 0
            self.__monitored_processes = {}
            self.__system_instant_metrics = []
            self.__usage_after = None

        def __call__(self):
            if self.__capture_output:
                stdout_arg = subprocess.PIPE
                stderr_arg = subprocess.PIPE
                stdout_list = []
                stderr_list = []
            else:
                stdout_arg = None
                stderr_arg = None
            main_process = psutil.Popen(
                *self.__args,
                stdout=stdout_arg, stderr=stderr_arg, universal_newlines=True,
                **self.__kwds)
            spawn_time = time.perf_counter()
            main_process = self.__start_monitoring_process(main_process)

            while main_process.psutil_process.returncode is None:
                iteration_before = self.__iteration
                while True:
                    self.__iteration += 1
                    timeout = spawn_time + self.__iteration * self.__interval - time.perf_counter()
                    if timeout > 0:
                        break
                missing_samples = self.__iteration - iteration_before - 1
                if missing_samples > self.__allowed_missing_samples:
                    logging.warn(f"Monitoring is slow. {missing_samples} samples will be missing between t={(iteration_before + 1) * self.__interval:.3f}s and t={(self.__iteration - 1) * self.__interval:.3f}s included.")
                try:
                    (stdout, stderr) = main_process.psutil_process.communicate(timeout=timeout)
                    if self.__capture_output:
                        stdout_list.append(stdout)
                        stderr_list.append(stderr)
                except subprocess.TimeoutExpired:
                    # Main process is still running
                    self.__run_monitoring_iteration()
                else:
                    # Main process has terminated
                    self.__terminate()

            if self.__capture_output:
                stdout = "".join(stdout_list)
                stderr = "".join(stderr_list)
            else:
                stdout = None
                stderr = None

            return RunResults(
                system=SystemMetrics(instant_metrics=[
                    SystemInstantMetrics(
                        timestamp=m.iteration * self.__interval,
                        host_to_device_transfer_rate=m.host_to_device_transfer_rate,
                        device_to_host_transfer_rate=m.device_to_host_transfer_rate,
                    )
                    for m in self.__system_instant_metrics
                ]),
                main_process=self.__return_main_process(main_process, main_process.psutil_process.returncode),
                stdout=stdout,
                stderr=stderr,
            )

        def __start_monitoring_process(self, psutil_process):
            psutil_process.cpu_percent()  # Ignore first, meaningless 0.0 returned, as per https://psutil.readthedocs.io/en/latest/#psutil.Process.cpu_percent
            child = InProgressProcess(
                psutil_process=psutil_process,
                pid=psutil_process.pid,
                command=psutil_process.cmdline(),
                spawned_at_iteration=self.__iteration,
                children=[],  # We'll detect children in the next iteration
                instant_metrics=[],  # We'll gather the first instant metrics in the next iteration
                terminated_at_iteration=None,
            )
            self.__monitored_processes[psutil_process.pid] = child
            parent = psutil_process.ppid()
            if parent in self.__monitored_processes:
                self.__monitored_processes[parent].children.append(child)
            return child

        def __run_monitoring_iteration(self):
            # Start sub-processes as early as possible to give them time to run while we do other stuff
            nvidia_smi_dmon = subprocess.Popen(["nvidia-smi", "dmon", "-c", "1", "-s", "t"], stdout=subprocess.PIPE, universal_newlines=True)
            nvidia_smi_pmon = subprocess.Popen(["nvidia-smi", "pmon", "-c", "1", "-s", "um"], stdout=subprocess.PIPE, universal_newlines=True)

            for process in list(self.__monitored_processes.values()):
                try:
                    self.__gather_instant_metrics(process)
                    self.__gather_children(process)
                except psutil.NoSuchProcess:
                    self.__stop_monitoring_process(process)

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
                    if metrics.iteration == self.__iteration:
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

            self.__system_instant_metrics.append(InProgressSystemInstantMetrics(
                iteration=self.__iteration,
                host_to_device_transfer_rate=float(parts[1]),
                device_to_host_transfer_rate=float(parts[2]),
            ))


        def __gather_instant_metrics(self, process):
            try:
                with process.psutil_process.oneshot():
                    cpu_times = process.psutil_process.cpu_times()
                    process.instant_metrics.append(InProgressInstantRunMetrics(
                        iteration=self.__iteration,
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
                logging.warn(f"Exception 'psutil.AccessDenied' occurred. Instant metrics for {process.command} will be missing at t={self.__iteration * self.__interval}s.")

        def __gather_children(self, process):
            for child in process.psutil_process.children():
                if child.pid not in self.__monitored_processes:
                    self.__start_monitoring_process(child)

        def __stop_monitoring_process(self, process):
            process.terminated_at_iteration = self.__iteration
            del self.__monitored_processes[process.pid]

        def __terminate(self):
            self.__usage_after = resource.getrusage(resource.RUSAGE_CHILDREN)
            for process in self.__monitored_processes.values():
                if process.terminated_at_iteration is None:
                    process.terminated_at_iteration = self.__iteration

        def __return_main_process(self, process, exit_code):
            spawned_at = process.spawned_at_iteration * self.__interval
            terminated_at = process.terminated_at_iteration * self.__interval
            return MainProcess(
                command=process.command,
                spawned_at=spawned_at,
                terminated_at=terminated_at,
                instant_metrics=self.__return_instant_metrics(process),
                children=[self.__return_process(child) for child in process.children],
                exit_code=exit_code,
                global_metrics=MainProcessGlobalMetrics(
                    duration=terminated_at - spawned_at,
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
                InstantRunMetrics(
                    timestamp=m.iteration * self.__interval,
                    threads=m.threads,
                    cpu_percent=m.cpu_percent,
                    user_time=m.user_time,
                    system_time=m.system_time,
                    memory=m.memory._asdict(),
                    open_files=m.open_files,
                    io=m.io._asdict(),
                    context_switches=m.context_switches._asdict(),
                    gpu_percent=m.gpu_percent,
                    gpu_memory=m.gpu_memory,
                )
                for m in process.instant_metrics
            ]

        def __return_process(self, process):
            spawned_at = process.spawned_at_iteration * self.__interval
            terminated_at = process.terminated_at_iteration * self.__interval
            return Process(
                command=process.command,
                spawned_at=spawned_at,
                terminated_at=terminated_at,
                instant_metrics=self.__return_instant_metrics(process),
                children=[self.__return_process(child) for child in process.children],
                global_metrics=GlobalMetrics(
                    duration=terminated_at - spawned_at,
                )
            )
