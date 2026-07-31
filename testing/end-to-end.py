import json
import multiprocessing
import os
import subprocess
import sys
import time
from pathlib import Path

data_failed = False

def error(*args, **kwargs):
    print("Error:", *args, file=sys.stderr, **kwargs)

class Test:
    def __init__(self, name : str, data : dict[str, str]):
        global data_failed
        self.name : str = name
        try:
            self.path: str = data["testfile"]

            if data.__contains__("stdout"): self.stdout: str = data["stdout"]
            else:                           self.stdout: str = "/home/gabriel/CLionProjects/language/testing/tests/empty"

            if data.__contains__("stderr"): self.stderr : str = data["stderr"]
            else:                           self.stderr : str = "/home/gabriel/CLionProjects/language/testing/tests/empty"

            if not os.path.isfile(self.stdout):
                error(f"Test \"{name}\" has invalid stdout path: \"{self.stdout}\"")
                data_failed = True
            if not os.path.isfile(self.stderr):
                error(f"Test \"{name}\" has invalid stderr path: \"{self.stdout}\"")
                data_failed = True
        except KeyError as e:
            error(f"Test \"{name}\" was missing attribute {e}")
            data_failed = True

class Config:
    def __init__(self, data : dict):
        self.data = data.copy()
        try:
            self.interpreter : str = data["Instance"]
            self.data.pop("Instance")
        except KeyError:
            self.interpreter : str = "/home/gabriel/CLionProjects/language/cmake-build-debug/grtlng"

        self.log : str = self.data["log"]
        self.data.pop("log")

class Result:
    def __init__(self, success : bool, message : list[str]):
        self.success : bool = success
        self.message : str =  "".join(message)

def read_config(path : str) -> Config:
    pathfile: Path = Path(path)

    if not pathfile.is_file():
        error(f"Test configuration file {pathfile.absolute()} does not exist")
        exit(os.EX_DATAERR)

    tests: dict[str, dict[str, str]]

    with open(pathfile.absolute(), "r") as f:
        json_input = f.read()

        if len(json_input) == 0:
            error(f"Configuration file {pathfile.absolute()} was empty")
            exit(os.EX_DATAERR)

        try:
            tests = json.loads(json_input)
        except json.JSONDecodeError as e:
            error(f"Configuration file {pathfile.absolute()} was malformed on line {e.lineno}:\n  {e.msg}")
            exit(os.EX_DATAERR)

    return Config(tests)

def make_tests(inputs : Config) -> list[Test]:
    global data_failed
    tests = []

    for name, test in inputs.data["tests"].items():
        t = Test(name, test)
        if not data_failed: tests.append(t)
        data_failed = False

    return tests

def expected_got_format(expected : str, got : str) -> list[str]:
    returns = []

    exp = ""
    for _ in range(25):
        exp += " "
    exp += "Expected:\n"
    returns.append(exp)

    for line in expected.splitlines():
        l = ""
        for _ in range(27):
            l += " "
        l += line + "\n"

        returns.append(l)


    g = ""
    for _ in range(25):
        g += " "
    g += "Got:\n"
    returns.append(g)

    for line in got.splitlines():
        l = ""
        for _ in range(27):
            l += " "
        l += line + "\n"

        returns.append(l)

    return returns

def run_test(test : Test, config : Config) -> Result:

    with open(test.stdout, "r") as f:
        stdout = f.read().strip()

    with open(test.stderr, "r") as f:
        stderr = f.read().strip()

    time_start = time.localtime()

    finished_test = subprocess.run([config.interpreter, "-i", test.path], capture_output=True)

    stdout_cap = finished_test.stdout.decode("UTF-8").strip()
    stderr_cap = finished_test.stderr.decode("UTF-8").strip()

    if not (stderr == stderr_cap and stdout == stdout_cap):
        timestamp = time.strftime("%d:%m:%Y %H:%M:%S", time_start)

        err_string = f"{timestamp}: Failed test \"{test.name}\" ({test.path}):\n"

        errs = [err_string]

        if stdout != stdout_cap:
            errs += f"                       Mismatched stdout:\n"
            lines = expected_got_format(stdout, stdout_cap)
            errs.extend(lines)
        if stderr != stderr_cap:
            errs += f"                       Mismatched stderr:\n"
            lines = expected_got_format(stderr, stderr_cap)
            errs.extend(lines)

        #errs.append("\n")

        return Result(False, errs)

    return Result(True, [])

def init_worker(config, tests):
    global _tests, _config
    _config = config
    _tests = tests

def worker_run(index):
    global _tests, _config
    return run_test(_tests[index], _config)

def main():
    start = time.time_ns()

    if len(sys.argv) != 3:
        print("Improper usage: Do python end-to-end.py <config.json> <single | multi>")
        exit(os.EX_USAGE)

    config = read_config(sys.argv[1])
    tests = make_tests(config)


    indices = list(range(len(tests)))
    results : list[Result] = []

    match sys.argv[2]:
        case "single":
            print("Warning: single-threaded mode has been selected. Testing will be a lot slower than usual.")
            results = [run_test(i, config) for i in tests]
        case "multi":
            with multiprocessing.Pool(multiprocessing.cpu_count(), initializer=init_worker, initargs=(config, tests)) as pool:
                results = pool.map(worker_run, indices)
        case _:
            error(f"sys.argv[2] (single / multicore processing) was given unexpected value. Expected \"single\" or \"multi\", got {sys.argv[2]}")
            exit(os.EX_USAGE)


    errors = []
    failed_tests = 0

    for result in results:
        if not result.success:
            failed_tests += 1
            errors.append(result.message)

    with open(config.log, "a") as f:
        f.writelines(errors)

    if failed_tests == 0:
        print("All tests passed successfully!")
    else:
        print(f"{failed_tests} test(s) failed. See \"{config.log}\" for more info")

    end = time.time_ns()
    print("Test duration:", (end - start) / 1e9)
    print("Number of tests:", len(tests))

if __name__ == "__main__":
    main()