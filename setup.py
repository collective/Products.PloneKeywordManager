import os
from setuptools import setup, find_packages

version = '1.10dev'

long_description = open("README.txt").read() + "\n" + \
                   open(os.path.join("docs", "HISTORY.txt")).read()


setup(name='Products.PloneKeywordManager',
      version=version,
      description="Plone Keyword Manager allows you to change, merge and delete \
          keywords in Plone and updates all corresponding objects automatically.\
          It uses a similiarity search to support you in identifying similar keywords.\
          Keywords can be cleaned up from time to time by a site manager to \
          create a consistent vocabulary.",
      long_description=long_description,
      classifiers=[
        "Development Status :: 6 - Mature",
        "Environment :: Web Environment",
        "Framework :: Plone",
        "Framework :: Plone :: 4.0",
        "Framework :: Plone :: 4.1",
        "Framework :: Plone :: 4.2",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: System Administrators",
        "License :: OSI Approved :: GNU General Public License (GPL)",
        "Operating System :: OS Independent",
        "Programming Language :: Python",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
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
        ],
      extras_require={
        'Levenshtein': ['python-Levenshtein'],
        'test': [
          'plone.app.testing',
          'plone.app.dexterity',
          ],
        },
      entry_points="""
      [z3c.autoinclude.plugin]
      target = plone
      """,
      )
