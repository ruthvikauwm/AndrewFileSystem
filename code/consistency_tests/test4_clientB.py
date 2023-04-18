#! /usr/bin/env python3

import os
import fs_util
import sys
import test4_clientA as test4  # Simply do not want to do test1_common
'''
This is ClientB.
'''

'''
Testing delete scenario. 
Client A - open,write,close
Client B - unlink
Client A - open 
Expected outcome in Client A - File Not Found error
'''


def run_test():
    signal_name_gen = fs_util.get_fs_signal_name()

    cur_signal_name = next(signal_name_gen)
    fs_util.record_test_result(test4.TEST_CASE_NO, 'B',
                               f'START fname:{test4.FNAME}')
    fs_util.wait_for_signal(cur_signal_name)
    
    fs_util.delete_file(test4.FNAME)

    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)

    # done
    fs_util.record_test_result(test4.TEST_CASE_NO, 'B', 'OK')


if __name__ == '__main__':
    run_test()
