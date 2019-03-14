# -*- coding: utf-8 -*-
"""Installer for the collective.es.index package."""
from setuptools import setup


setup(
    # zest releaser does not change cfg file.
    version='3.0.0.dev0',

    # thanks to this bug
    # https://github.com/pypa/setuptools/issues/1136
    # we need one line in here:
    package_dir={'': 'src'},
)
