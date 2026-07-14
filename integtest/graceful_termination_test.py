# 08-July-2026, KAB: the goal of this test is to check whether the monitoring functionality
# in opmonlib gracefully shuts down as the DAQ processes are shutting down. An indication of
# an un-graceful shutdown is the presence of garbled strings in the metrics that are written
# to disk.
#
# Garbled strings were observed before the long delay in the ZmqSender destructor was removed
# and before the stop_monitoring() call was added to the appfwk::Application::run() method.
# If we want to force this integtest to fail (e.g. as a sanity check), we can comment out the
# stop_monitoring() call in Application::run().
#
# This integtest configures an artificial long delay in the ZmqSender destructor and checks
# for any garbled strings in the metrics on disk. It also specifies an ignored logfile string
# for the expected warning message from the inclusion of the artificial delay so that we
# don't see failures from a warning message that we know will be present in the logs.
#
# This integtest was created by copying the small_footprint_quick_test from the daqsystemtest
# repo and adding the extra configuration for the artificial delay, the extra checking of
# metric strings, etc.
#
import pytest
import urllib.request

import integrationtest.data_file_checks as data_file_checks
import integrationtest.log_file_checks as log_file_checks
import integrationtest.data_classes as data_classes
import integrationtest.resource_validation as resource_validation
import integrationtest.opmon_metric_checks as opmon_metric_checks
import integrationtest.utility_functions as utility_functions
from integrationtest.get_pytest_tmpdir import get_pytest_tmpdir
from integrationtest.verbosity_helper import IntegtestVerbosityLevels

import functools
print = functools.partial(print, flush=True)  # always flush print() output

pytest_plugins = "integrationtest.integrationtest_drunc"

# Values that help determine the running conditions
number_of_data_producers = 1
run_duration = 20  # seconds

# Default values for validation parameters
expected_number_of_data_files = 1
check_for_logfile_errors = True
expected_event_count = run_duration
expected_event_count_tolerance = 2
wibeth_frag_params = {
    "fragment_type_description": "WIBEth",
    "fragment_type": "WIBEth",
    "expected_fragment_count": number_of_data_producers,
    "min_size_bytes": 14472,
    "max_size_bytes": 21672,
}
triggercandidate_frag_params = {
    "fragment_type_description": "Trigger Candidate",
    "fragment_type": "Trigger_Candidate",
    "expected_fragment_count": 1,
    "min_size_bytes": 128,
    "max_size_bytes": 216,
}
hsi_frag_params = {
    "fragment_type_description": "HSI",
    "fragment_type": "Hardware_Signal",
    "expected_fragment_count": 1,
    "min_size_bytes": 100,
    "max_size_bytes": 100,
}
required_logfile_problems = {
    "df-01": [
        "An artificial delay of \\d+ usec is being introduced"
    ],
    "dfo-01": [
        "An artificial delay of \\d+ usec is being introduced"
    ],
    "mlt": [
        "An artificial delay of \\d+ usec is being introduced"
    ],
    "ru-det-conn-0": [
        "An artificial delay of \\d+ usec is being introduced"
    ]
}
ignored_logfile_problems = {
    "connectionservice": [
        "Searching for connections matching uid_regex<errored_frames_q> and data_type Unknown"
    ],
    "-controller": [
        "Worker with pid \\d+ was terminated due to signal 1",
        "Connection '.*' not found on the application registry",
    ],
    "connectivity-service": [
        "errorlog: -",
    ]
}

# Determine if this computer has enough resources for these tests
resource_validator = resource_validation.ResourceValidator()
resource_validator.cpu_count_needs(4, 8)  # 2 for each data source plus 2 more for everything else
resource_validator.free_memory_needs(4, 6)  # 33% more than what we observe being used ('free -h')
actual_output_path = get_pytest_tmpdir()
resource_validator.free_disk_space_needs(actual_output_path, 1)  # more than what we observe

# The arguments to pass to the config generator, excluding the json
# output directory (the test framework handles that)

conf_dict = data_classes.integtest_params_for_generated_dunedaq_config()
conf_dict.object_databases = ["config/daqsystemtest/integrationtest-objects.data.xml"]
conf_dict.dro_map_config.n_streams = number_of_data_producers
conf_dict.op_env = "integtest"
conf_dict.config_session_name = "gracefultermination"
conf_dict.tpg_enabled = False
conf_dict.fake_hsi_enabled = True
conf_dict.trace_debug_levels = {"fast path": {"DelayManager.hpp": 42}}

conf_dict.config_substitutions.append(
    data_classes.attribute_substitution(obj_class="LatencyBuffer", updates={"size": 50000})
)
conf_dict.config_substitutions.append(
    data_classes.attribute_substitution(
        obj_class="FakeHSIEventGeneratorConf",
        updates={"trigger_rate": 1.0},
    )
)
conf_dict.config_substitutions.append(
    data_classes.attribute_substitution(
        obj_class="DelaySpec",
        obj_id = "delay-spec-01",
        updates={
            "delay_name": "~ZmqSender",
            "delay_usec": "10000000",
        },
    )
)
conf_dict.config_substitutions.append(
    data_classes.list_element_addition(
        obj_class="DelayManagerConf",
        obj_id="delay-mgr",
        rel_name="delays",
        additional_object_class="DelaySpec",
        additional_object_id="delay-spec-01",
    )
)
conf_dict.config_substitutions.append(
    data_classes.relationship_substitution(
        obj_class="DFApplication",
        obj_id="df-01",
        rel_name="delay_manager_conf",
        replacement_object_class="DelayManagerConf",
        replacement_object_id="delay-mgr"
    )
)
conf_dict.config_substitutions.append(
    data_classes.relationship_substitution(
        obj_class="DFOApplication",
        obj_id="dfo-01",
        rel_name="delay_manager_conf",
        replacement_object_class="DelayManagerConf",
        replacement_object_id="delay-mgr"
    )
)
conf_dict.config_substitutions.append(
    data_classes.relationship_substitution(
        obj_class="MLTApplication",
        obj_id="mlt",
        rel_name="delay_manager_conf",
        replacement_object_class="DelayManagerConf",
        replacement_object_id="delay-mgr"
    )
)
conf_dict.config_substitutions.append(
    data_classes.relationship_substitution(
        obj_class="ReadoutApplication",
        obj_id="ru-det-conn-0",
        rel_name="delay_manager_conf",
        replacement_object_class="DelayManagerConf",
        replacement_object_id="delay-mgr"
    )
)

confgen_arguments = {"GracefulTermination": conf_dict}

# The commands to run in dunerc, as a list
dunerc_command_list = (
    "boot conf start --run-number 101 wait 1 enable-triggers wait ".split()
    + [str(run_duration)]
    + "disable-triggers wait 2 drain-dataflow stop-trigger-sources stop wait 2 scrap terminate".split()
)

# The tests themselves


def test_dunerc_success(run_dunerc, caplog):
    # checks for run control success, problems during pytest setup, etc.
    utility_functions.basic_checks(run_dunerc, caplog, print_test_name=False)

    # check that the test took long enough that we can be confident that the
    # requested artificial delay(s) were actually run
    expected_min_time_sec = 65
    if run_dunerc.daq_session_overall_time < expected_min_time_sec:
        fail_msg = (f"The run control session took less time than expected. The overall "
                    f"run time was {round(run_dunerc.daq_session_overall_time,1)} sec, and "
                    f"the typical run time is greater than {expected_min_time_sec} sec. "
                    f"Check that the expected artificial delays in this test were actually run.")
        pytest.fail(fail_msg, pytrace=False)
    else:
        success_msg = (f"\N{WHITE HEAVY CHECK MARK} The run control session took the expected "
                       f"amount of time ({round(run_dunerc.daq_session_overall_time,1)}>="
                       f"{expected_min_time_sec} sec)")
        run_dunerc.verbosity_helper.lvl_print(IntegtestVerbosityLevels.drunc_transitions, success_msg)


def test_log_files(run_dunerc):
    if check_for_logfile_errors:
        # Check that there are no warnings or errors in the log files
        assert log_file_checks.logs_are_error_free(
            run_dunerc.log_files, True, True, ignored_logfile_problems,
            required_logfile_problems, verbosity_helper=run_dunerc.verbosity_helper
        )


def test_data_files(run_dunerc):
    # Run some tests on the output data file
    assert len(run_dunerc.data_files) == expected_number_of_data_files

    fragment_check_list = [triggercandidate_frag_params, hsi_frag_params]
    fragment_check_list.append(wibeth_frag_params)  # WIBEth

    all_ok = True
    for idx in range(len(run_dunerc.data_files)):
        data_file = data_file_checks.DataFile(run_dunerc.data_files[idx], run_dunerc.verbosity_helper)
        all_ok &= data_file_checks.sanity_check(data_file)
        all_ok &= data_file_checks.check_file_attributes(data_file)
        all_ok &= data_file_checks.check_event_count(
            data_file, expected_event_count, expected_event_count_tolerance
        )
        for jdx in range(len(fragment_check_list)):
            all_ok &= data_file_checks.check_fragment_count(
                data_file, fragment_check_list[jdx]
            )
            all_ok &= data_file_checks.check_fragment_sizes(
                data_file, fragment_check_list[jdx]
            )
    assert all_ok


def test_metric_files(run_dunerc):
    metric_data = opmon_metric_checks.collate_opmon_data_from_files(run_dunerc.opmon_files)

    all_ok = True
    metric_key_list = [run_dunerc.daq_session_name, "df-01", "appfwk.AppInfo", "state"]
    all_ok &= opmon_metric_checks.check_metric_value_string(metric_data, metric_key_list, r"^[a-zA-Z_]+$",
                                                            verbosity_helper=run_dunerc.verbosity_helper)
    metric_key_list = [run_dunerc.daq_session_name, "dfo-01", "appfwk.AppInfo", "state"]
    all_ok &= opmon_metric_checks.check_metric_value_string(metric_data, metric_key_list, r"^[a-zA-Z_]+$",
                                                            verbosity_helper=run_dunerc.verbosity_helper)
    metric_key_list = [run_dunerc.daq_session_name, "mlt", "appfwk.AppInfo", "state"]
    all_ok &= opmon_metric_checks.check_metric_value_string(metric_data, metric_key_list, r"^[a-zA-Z_]+$",
                                                            verbosity_helper=run_dunerc.verbosity_helper)
    metric_key_list = [run_dunerc.daq_session_name, "ru-det-conn-0", "appfwk.AppInfo", "state"]
    all_ok &= opmon_metric_checks.check_metric_value_string(metric_data, metric_key_list, r"^[a-zA-Z_]+$",
                                                            verbosity_helper=run_dunerc.verbosity_helper)
    metric_key_list = [run_dunerc.daq_session_name, "hsi-to-tc-app", "appfwk.AppInfo", "state"]
    all_ok &= opmon_metric_checks.check_metric_value_string(metric_data, metric_key_list, r"^[a-zA-Z_]+$",
                                                            verbosity_helper=run_dunerc.verbosity_helper)
    assert all_ok
