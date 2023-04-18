#! /usr/bin/env python3

import os
import fs_util
import sys
import time
'''
This is ClientA. 
'''

'''
Testing rename scenario. 
Client A - open,write,close
Client B - rename
Client A - open 
Expected outcome in Client A - File Not Found error
'''

cs739_env_vars = [
    'CS739_CLIENT_A', 'CS739_CLIENT_B', 'CS739_SERVER', 'CS739_MOUNT_POINT'
]
ENV_VARS = {var_name: os.environ.get(var_name) for var_name in cs739_env_vars}
for env_var in ENV_VARS.items():
    print(env_var)
    assert env_var is not None
TEST_DATA_DIR = ENV_VARS['CS739_MOUNT_POINT'] + '/test_consistency'
FNAME = f'{TEST_DATA_DIR}/case2'
print(TEST_DATA_DIR)
TEST_CASE_NO = 2


def run_test():
    host_b = ENV_VARS['CS739_CLIENT_B']
    assert fs_util.test_ssh_access(host_b)
    
    # print(signal_name_gen.gi_yieldfrom)
    if not fs_util.path_exists(TEST_DATA_DIR):
        fs_util.mkdir(TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(FNAME):
        fs_util.create_file(FNAME)

    init_str = fs_util.gen_str_by_repeat('0', 32768)
    fd = fs_util.open_file(FNAME)
    fs_util.write_file(fd, init_str)
    fs_util.close_file(fd)

    # time for client_b to work, host_b should read the all-zero file
    signal_name_gen = fs_util.get_fs_signal_name()
    cur_signal_name = next(signal_name_gen)
    fs_util.start_another_client(host_b, 2, 'B', cur_signal_name)

    # wait until client_b finish
    while True:
        removed = fs_util.poll_signal_remove(host_b, cur_signal_name)
        if removed:
            break
        time.sleep(1000)
    print('Clientb finished')


    # read the content written by client_b
    x = 0
    try:
        fd = fs_util.open_file(FNAME)
    except FileNotFoundError:
        x = 1
    
    # print(x)
    assert x==1


    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.send_signal(host_b, cur_signal_name)

    # done
    fs_util.record_test_result(TEST_CASE_NO, 'A', 'OK')


if __name__ == '__main__':
    run_test()
