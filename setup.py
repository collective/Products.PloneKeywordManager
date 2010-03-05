import os
from setuptools import setup, find_packages

_os_path = os.path.join("Products", "PloneKeywordManager")

version = open(os.path.join(_os_path, "version.txt")).read().strip()

setup(name='Products.PloneKeywordManager',
      version=version,
      description="Plone Keyword Manager allows you to change, merge and delete \
          keywords in Plone and updates all corresponding objects automatically.\
          It uses a similiarity search to support you in identifying similar keywords.\
          Keywords can be cleaned up from time to time by a site manager to \
          create a consistent vocabulary.",

      long_description=open(os.path.join(_os_path, "README.txt")).read() + "\n" +
                       open(os.path.join(_os_path, "CHANGES.txt")).read(),
      # Get more strings from http://www.python.org/pypi?%3Aaction=list_classifiers
      classifiers=[
        "Framework :: Plone",
        "Programming Language :: Python",
        "Topic :: Software Development :: Libraries :: Python Modules",
        ],
      keywords='Plone Keywords',
      author='Michael Dunlap',
      author_email='dunlapm@u.washington.edu',
      url='http://plone.org/products/plonekeywordmanager/',
      license='GPL',
      packages=find_packages(exclude=['ez_setup']),
      namespace_packages=['Products'],
      include_package_data=True,
      zip_safe=False,
      install_requires=[
          'setuptools',
          # -*- Extra requirements: -*-
      ],
      entry_points="""
      # -*- Entry points: -*-
      """,
      )
