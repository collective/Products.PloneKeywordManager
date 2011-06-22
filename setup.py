import os
from setuptools import setup, find_packages


def read(*rnames):
    return open(os.path.join(os.path.dirname(__file__), *rnames)).read()

version = '1.9'

long_description = (
    read('README.txt')
    + '\n' +
    read('CHANGES.txt')
    #+ '\n' +
    #read('Products', 'PloneKeywordManager', 'README.txt')
    #+ '\n' +
    #read('CONTRIBUTORS.txt')
    )


setup(name='Products.PloneKeywordManager',
      version=version,
      description="Plone Keyword Manager allows you to change, merge and delete \
          keywords in Plone and updates all corresponding objects automatically.\
          It uses a similiarity search to support you in identifying similar keywords.\
          Keywords can be cleaned up from time to time by a site manager to \
          create a consistent vocabulary.",
      long_description=long_description,
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

      [z3c.autoinclude.plugin]
      target = plone
      """,
      extras_require = {
          'Levenshtein': [
              'python-Levenshtein',
          ]
      }
      )
