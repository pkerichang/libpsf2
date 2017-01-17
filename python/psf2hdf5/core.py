# -*- coding: utf-8 -*-

"""Core functionalities."""

from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
# noinspection PyCompatibility
from builtins import *

import os
import pprint

from .grammar import parse_psf_ascii


ARTIST_LOG = 'artistLogFile'
RUN_OBJ = 'runObjFile'


def parse_adexl_results(results_dir):
    """Read the given ADEXL result directory and return results."""

    dirname = os.path.abspath(results_dir)
    if not os.path.isdir(dirname):
        raise ValueError('%s is not a directory.' % dirname)

    # STEP 1: get global variables and default values
    # STEP 1A: read artistLogFile
    val_dict, prop_dict = parse_psf_ascii(os.path.join(dirname, ARTIST_LOG))
    pprint.pprint(val_dict)
    pprint.pprint(prop_dict)

    # STEP 1B: find variables file name
    var_file_name = ''
    for val in val_dict.values():
        if val['value']['analysisType'] == 'design_variables':
            var_file_name = val['value']['dataFile']
            break
    if not var_file_name:
        raise Exception('Cannot find design variable file')

    # STEP 1C: read variables file and create design variable dictionary.
    val_dict, prop_dict = parse_psf_ascii(os.path.join(dirname, var_file_name))
    dsn_vars = {var_name: var_val['value'] for var_name, var_val in val_dict.items()}
    pprint.pprint(dsn_vars)
    pprint.pprint(prop_dict)

    # STEP 2: get all simulation directories
    # STEP 2A: read runObjFile
    val_dict, prop_dict = parse_psf_ascii(os.path.join(dirname, RUN_OBJ))
    pprint.pprint(val_dict)
    pprint.pprint(prop_dict)


